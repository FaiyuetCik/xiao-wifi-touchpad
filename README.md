# XIAO Wi-Fi Touchpad

[![中文](https://img.shields.io/badge/中文-README-blue?style=for-the-badge)](README_CN.md)

Turn the Seeed Studio XIAO 1.47-inch touch display (ESP32-S3 Plus) into a Wi-Fi touchpad mouse for a Windows computer.

This repository contains the working mouse demo extracted from Pocket AI Terminal. Chat, Voice and Control are placeholder pages; AI and voice integration are not implemented yet.

## Hardware and software

- XIAO ESP32-S3 Plus with the 172 × 320 JD9853A / AXS5106L touch display board
- A-02 2.4 GHz antenna connected to the XIAO antenna connector
- Arduino ESP32 Board Package 3.3.11
- Seeed_GFX2 v1.0.0 using Config_Seeed_1inch47_Touch_JD9853A
- Python 3 on Windows

## Setup and run

1. Copy Pocket_AI_Terminal/wifi_config.h.example to Pocket_AI_Terminal/wifi_config.h.
2. Set WIFI_SSID, WIFI_PASSWORD and GATEWAY_HOST to your Wi-Fi credentials and the computer LAN IPv4 address. Run ipconfig on Windows to find the address.
3. Open Pocket_AI_Terminal/Pocket_AI_Terminal.ino in Arduino IDE, select XIAO_ESP32S3_PLUS and the board port, then upload.
4. Install dependencies:

    python -m pip install -r requirements.txt

5. Start the Wi-Fi gateway:

    python gateway/pocket_gateway.py --tcp 192.168.1.100 8765

Replace the example address with the computer LAN IP. Allow Python through the Windows private-network firewall if prompted. READY and repeated ping messages confirm the connection.

The local wifi_config.h is ignored by Git. Never upload real passwords or firmware binaries built with real credentials.

 ## Windows WiFi Gateway Setup

  ### 1. Clone the project

  Open PowerShell and run:

  powershell
  git clone https://github.com/FaiyuetCik/xiao-wifi-touchpad.git D:\xiao-wifi-touchpad

  ### 2. Enter the project directory

  cd D:\xiao-wifi-touchpad

  cd means changing to a directory.

  ### 3. Install dependencies

  python -m pip install -r requirements.txt

  ### 4. Start the WiFi gateway

  python gateway\pocket_gateway.py --tcp 0.0.0.0 8765

  When the following message appears, the gateway is running:

  Waiting for Pocket Terminal TCP connection on 0.0.0.0:8765

  ### 5. Configure the Arduino firmware

  Open:

  Pocket_AI_Terminal\wifi_config.h

  Set the WiFi information:

  #pragma once

  static constexpr char WIFI_SSID[] = "YOUR_WIFI_NAME";
  static constexpr char WIFI_PASSWORD[] = "YOUR_WIFI_PASSWORD";
  static constexpr char GATEWAY_HOST[] = "192.168.5.121";

  Save the file and upload the firmware again.

  The computer and XIAO board must be connected to the same WiFi network.

  > Replace 192.168.5.121 with your computer's local IPv4 address if it is different.
## Mouse gestures

| Function | Gesture |
| --- | --- |
| Move pointer | Slide in the central touchpad |
| Left click | Tap the touchpad |
| Double click | Tap twice quickly |
| Drag / select text | Tap once, then press nearby and slide; release to finish |
| Scroll | Slide up or down in the narrow right strip |
| Select all / copy / paste / undo | Tap the upper-left tools button |
| Return home | Press and release HOME |

There is currently no right-click touch binding. A stationary long press does not start dragging.

## USB serial mode

With the placeholder Wi-Fi settings, use:

    python gateway/pocket_gateway.py COM33

Close Arduino Serial Monitor before using this mode. The serial baud rate is 115200. Wi-Fi uses raw TCP with newline-delimited JSON rather than WebSocket.

## Validation

- Compiled and flashed successfully with ESP32 3.3.11 and Seeed_GFX2 v1.0.0.
- The board sends hello, receives READY and sends periodic heartbeats over Wi-Fi.
- Wi-Fi mouse movement, clicking, double-clicking, dragging, scrolling and shortcuts were verified on hardware.
- The gateway has no network authentication or encryption. Use it only on a trusted private LAN; do not expose port 8765 to the Internet.

The Seeed_GFX2 library and its license belong to the upstream repository and are not bundled here.
