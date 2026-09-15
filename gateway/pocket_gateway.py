"""Desktop gateway for Pocket AI Terminal.

The same newline-delimited JSON protocol works over USB serial or a local
Wi-Fi TCP connection. Wi-Fi mode is intended for the ESP32-S3 board; serial
mode remains useful for provisioning and diagnostics.

Examples:
    python pocket_gateway.py COM33
    python pocket_gateway.py --tcp 0.0.0.0 8765
"""

import argparse
import atexit
import ctypes
import json
import socket
import sys
import time

import serial

try:
    import pyautogui
    pyautogui.FAILSAFE = False
    pyautogui.PAUSE = 0
except ImportError:
    pyautogui = None

held_buttons = set()


def release_buttons():
    if pyautogui is not None:
        for button in list(held_buttons):
            pyautogui.mouseUp(button=button)
            held_buttons.discard(button)


atexit.register(release_buttons)


class LineEndpoint:
    def __init__(self, reader, writer):
        self.reader = reader
        self.writer = writer
        self.buffer = bytearray()

    def send(self, text: str) -> None:
        payload = (text + "\n").encode("utf-8")
        if isinstance(self.writer, socket.socket):
            self.writer.sendall(payload)
        else:
            self.writer.write(payload)
            self.writer.flush()

    def readline(self):
        while True:
            marker = self.buffer.find(b"\n")
            if marker >= 0:
                raw = bytes(self.buffer[:marker])
                del self.buffer[:marker + 1]
                return raw
            try:
                chunk = (self.reader.recv(4096) if isinstance(self.reader, socket.socket)
                         else self.reader.read(4096))
            except socket.timeout:
                return b""
            if not chunk:
                return None if isinstance(self.reader, socket.socket) else b""
            self.buffer.extend(chunk)
            if len(self.buffer) > 16384:
                raise ConnectionError("Input line too long")


def handle_message(endpoint, message, state):
    now = time.monotonic()
    if message.get("type") == "hello":
        release_buttons()
        state["ready_sent"] = True
        endpoint.send("READY")
        print("gateway> READY", flush=True)
    elif message.get("type") == "ping":
        endpoint.send("PONG")
        if not state["ready_sent"]:
            endpoint.send("READY")
            state["ready_sent"] = True
            print("gateway> READY", flush=True)
    elif message.get("type") == "mouse_move":
        if pyautogui is None:
            endpoint.send("ERROR:install pyautogui")
            return
        before = pyautogui.position()
        dx, dy = int(message.get("dx", 0)), int(message.get("dy", 0))
        if sys.platform == "win32" and "left" in held_buttons:
            ctypes.windll.user32.mouse_event(0x0001, ctypes.c_long(dx),
                                               ctypes.c_long(dy), 0, 0)
        else:
            pyautogui.moveRel(dx, dy, duration=0)
        if now - state["last_mouse_log"] > 1:
            print(f"mouse> requested=({dx}, {dy}) position={before}->{pyautogui.position()}",
                  flush=True)
            state["last_mouse_log"] = now
    elif message.get("type") == "mouse_click":
        if pyautogui is None:
            endpoint.send("ERROR:install pyautogui")
        else:
            button = message.get("button", "left")
            if button in ("left", "right"):
                pyautogui.click(button=button)
    elif message.get("type") in ("mouse_down", "mouse_up"):
        button = message.get("button", "left")
        if pyautogui is not None and button in ("left", "right"):
            if message["type"] == "mouse_down":
                held_buttons.add(button)
                pyautogui.mouseDown(button=button)
            else:
                pyautogui.mouseUp(button=button)
                held_buttons.discard(button)
    elif message.get("type") == "mouse_scroll":
        if pyautogui is not None:
            steps = max(-20, min(20, int(message.get("steps", 0))))
            pyautogui.scroll(steps * 120 if sys.platform == "win32" else steps)
            print(f"wheel> steps={steps}", flush=True)
    elif message.get("type") == "shortcut":
        keys = {"select_all": "a", "copy": "c", "paste": "v", "undo": "z"}
        key = keys.get(message.get("action"))
        if pyautogui is not None and key:
            release_buttons()
            pyautogui.hotkey("ctrl", key)


def run_endpoint(endpoint, label):
    print(f"Pocket gateway on {label}", flush=True)
    state = {"ready_sent": False, "last_mouse_log": 0.0}
    last_received = time.monotonic()
    last_ping = time.monotonic()
    while True:
        raw = endpoint.readline()
        if raw is None:
            break
        if raw:
            last_received = time.monotonic()
            line = raw.decode("utf-8", errors="replace").strip()
            print(f"device> {line}")
            try:
                message = json.loads(line)
            except json.JSONDecodeError:
                continue
            if isinstance(message, dict):
                handle_message(endpoint, message, state)

        now = time.monotonic()
        if now - last_received > 5:
            release_buttons()
        if now - last_ping > 10:
            last_ping = now
            endpoint.send("REPLY:Wi-Fi link is working"
                          if label.startswith("TCP")
                          else "REPLY:Serial link is working")


def run_serial(port_name):
    with serial.Serial(port_name, 115200, timeout=0.2) as port:
        run_endpoint(LineEndpoint(port, port), port_name)


def run_tcp(host, port):
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as server:
        server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        server.bind((host, port))
        server.listen(1)
        server.settimeout(1.0)
        print(f"Waiting for Pocket Terminal TCP connection on {host}:{port}", flush=True)
        while True:
            try:
                client, address = server.accept()
            except socket.timeout:
                continue
            with client:
                # Block until the board sends its next newline-delimited
                # event. The board emits a ping every few seconds, so this
                # avoids mistaking an idle Wi-Fi link for a disconnect.
                client.settimeout(0.2)
                print(f"TCP device connected from {address[0]}:{address[1]}", flush=True)
                try:
                    run_endpoint(LineEndpoint(client, client),
                                 f"TCP {address[0]}:{address[1]}")
                except (OSError, ValueError, TypeError) as exc:
                    print(f"Connection ended: {exc}", flush=True)
                finally:
                    release_buttons()
                print("TCP device disconnected; waiting for reconnect", flush=True)


def main() -> int:
    parser = argparse.ArgumentParser(description="Pocket AI Terminal desktop gateway")
    parser.add_argument("port", nargs="?", default="COM33", help="USB serial port")
    parser.add_argument("--tcp", nargs=2, metavar=("HOST", "PORT"),
                        help="listen for Wi-Fi TCP")
    args = parser.parse_args()
    if args.tcp:
        run_tcp(args.tcp[0], int(args.tcp[1]))
    else:
        run_serial(args.port)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
