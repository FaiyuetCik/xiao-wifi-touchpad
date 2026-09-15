# XIAO Wi-Fi Touchpad

Turn the Seeed Studio XIAO 1.47-inch touch display (ESP32-S3 Plus) into a Wi-Fi mouse controller for a Windows computer.

This is the working mouse demo extracted from Pocket AI Terminal. The launcher also contains Chat, Voice and Control placeholders; AI and voice integration are not implemented.

## Hardware and software

- XIAO ESP32-S3 Plus with the 172 x 320 JD9853A / AXS5106L touch display board.
- A-02 2.4 GHz antenna attached to the XIAO antenna connector.
- USB power, or a suitable battery connected to the board.
- Arduino ESP32 board package **3.3.11**.
- [Seeed_GFX2 **v1.0.0**](https://github.com/Seeed-Studio/Seeed_GFX2/tree/v1.0.0), installed in your Arduino sketchbook libraries directory. This sketch uses Config_Seeed_1inch47_Touch_JD9853A.
- Python 3 on Windows, with the dependencies below. Development tests used Python 3.14.

## Setup

1. Install the exact display library version above; avoid selecting another copy of Seeed_GFX2 during compilation.
2. Copy Pocket_AI_Terminal/wifi_config.h.example to Pocket_AI_Terminal/wifi_config.h.
3. Set WIFI_SSID and WIFI_PASSWORD to your 2.4 GHz network, and GATEWAY_HOST to your computer's LAN IPv4 address (shown by ipconfig). The computer and board must be able to reach each other on the LAN.
4. Open Pocket_AI_Terminal/Pocket_AI_Terminal.ino in Arduino IDE. Choose XIAO_ESP32S3_PLUS and the board's USB port, then upload.
5. Install the desktop dependencies from this repository's root:

```powershell
python -m pip install -r requirements.txt
```

6. Start the gateway, replacing the example address with your computer's LAN IPv4:

```powershell
python gateway/pocket_gateway.py --tcp 192.168.1.100 8765
```

Allow Python on the Windows private-network firewall if prompted. The board reconnects to the gateway when it becomes available. READY and repeated ping messages in the terminal confirm the connection. USB may remain connected for power.

The local wifi_config.h is ignored by Git. Never publish it or firmware binaries built with real credentials.

## Mouse gestures

Open the lower-right mouse tile on the launcher.

| Action | Gesture |
| --- | --- |
| Move pointer | Slide in the central pad |
| Left click | Tap once (320 ms double-tap detection delay) |
| Double click | Two quick taps without sliding |
| Drag / select text | Tap once, then press nearby within 320 ms and slide; lift to release |
| Scroll | Slide in the narrow right strip |
| Select all / copy / paste / undo | Tap the upper-left tools button |
| Return to launcher | Press and release inside HOME |

There is no right-click touch binding. A stationary long press does not start dragging.

## USB mode

With placeholder Wi-Fi settings, use the original serial gateway (replace COM33 with the detected port):

```powershell
python gateway/pocket_gateway.py COM33
```

Close Arduino Serial Monitor before using this mode. The serial baud rate is 115200. Wi-Fi uses raw TCP with newline-delimited JSON events, not WebSocket. Gateway replies are READY, PONG, REPLY:... and ERROR:... lines.

## Validation and limitations

- Compiled with ESP32 3.3.11 and Seeed_GFX2 v1.0.0, and flashed successfully on 2026-09-15.
- Real board Wi-Fi hello/READY and periodic heartbeats verified; the owner confirmed the wireless mouse demo works.
- Local gateway checks cover READY/PONG, idle timeout and connection EOF.
- The Windows wheel conversion follows the tested PyAutoGUI backend; other operating systems are not hardware-tested.
- The gateway releases tracked mouse buttons after five seconds without device data and on TCP disconnection. Stop the gateway with Ctrl+C when finished.
- This prototype has no network authentication or encryption and can control the desktop. Use only on a trusted private LAN; do not expose port 8765 to the Internet. PyAutoGUI's corner failsafe is disabled in this demo.
- The battery icon is decorative; the connection icon indicates gateway readiness, not measured signal strength.

Display library source and its license remain in the linked upstream repository; it is not bundled here.
