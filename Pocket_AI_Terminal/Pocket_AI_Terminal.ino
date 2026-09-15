/*
 * Pocket AI Tool Terminal - v0.1 hardware/UI baseline
 *
 * 1.47-inch XIAO ESP32-S3 Touch Display
 * Seeed_GFX2 display + AXS5106L touch
 *
 * This first milestone intentionally contains no Wi-Fi or AI transport.
 * It verifies the new application shell without modifying the old factory
 * Dashboard firmware.
 */

#include <Arduino.h>
#include <Seeed_GFX.h>
#include "board/boards/XIAO_LCD_Board.h"
#include "driver/tft/Driver_JD9853A.h"
#include "panel/Panel_TFT.h"
#include "touch/Touch_AXS5106L.h"
#include <Wire.h>
#include <WiFi.h>

Seeed_GFX display;

static constexpr int8_t LCD_RST_PIN = 13;
static constexpr int8_t LCD_BL_PIN = 12;
static constexpr int SCREEN_W = 172;
static constexpr int SCREEN_H = 320;

Touch_AXS5106L touch(-1, D7, Wire, SCREEN_W, SCREEN_H);

static constexpr uint16_t C_BG = 0x1082;
static constexpr uint16_t C_PANEL = 0x2104;
static constexpr uint16_t C_CYAN = 0x07FF;
static constexpr uint16_t C_GREEN = 0x07E0;
static constexpr uint16_t C_YELLOW = 0xFFE0;
static constexpr uint16_t C_WHITE = TFT_WHITE;
static constexpr uint16_t C_GREY = 0x8410;

// Wi-Fi transport configuration. Keep these as placeholders in the sketch;
// do not commit real credentials. The gateway listens on the same LAN.
#include "wifi_config.h"
static constexpr uint16_t GATEWAY_PORT = 8765;

enum class Screen : uint8_t {
  Home,
  Chat,
  Voice,
  Control,
  Tools,
  Status,
  Mouse,
};

Screen g_screen = Screen::Home;
bool g_wasTouching = false;
bool g_gatewayReady = false;
WiFiClient g_gatewayClient;
bool g_wifiTransport = false;
String g_lastReply = "No reply yet";
uint32_t g_lastHeartbeatMs = 0;
int32_t g_lastTouchX = 0;
int32_t g_lastTouchY = 0;
bool g_mouseHomeCandidate = false;
int32_t g_mouseHomeX = 0;
int32_t g_mouseHomeY = 0;
bool g_mouseGestureActive = false;
bool g_mouseGestureStartedInPad = false;
uint32_t g_mouseLastTouchMs = 0;

static void drawMicIcon(int cx, int cy, uint16_t color, uint16_t bg = C_BG);
static void drawMouseIcon(int cx, int cy, uint16_t color);
static void drawChatIcon(int cx, int cy, uint16_t color);
static void drawComputerIcon(int cx, int cy, uint16_t color);
static void drawMouseFooter();

static bool wifiConfigured() {
  return WIFI_SSID[0] != '\0' && strcmp(WIFI_SSID, "YOUR_WIFI_SSID") != 0;
}

static void sendGatewayLine(const String &line) {
  if (g_gatewayClient.connected()) {
    g_gatewayClient.print(line);
    g_gatewayClient.print('\n');
  } else {
    // USB serial remains a diagnostic/fallback transport during migration.
    Serial.println(line);
  }
}

static void drawHeader(const char *title) {
  display.fillRect(0, 0, SCREEN_W, 44, C_PANEL);
  display.setTextDatum(TL_DATUM);
  display.setTextColor(C_CYAN, C_BG);
  display.drawString("AI", 8, 7, 1);
  display.setTextColor(C_WHITE, C_PANEL);
  display.drawString(title, 8, 24, 2);
}

static void drawFooter() {
  display.fillRect(0, 278, SCREEN_W, 42, C_BG);
  display.drawRoundRect(52, 286, 68, 25, 5, C_CYAN);
  display.setTextDatum(MC_DATUM);
  display.setTextColor(C_CYAN, C_PANEL);
  display.drawString("HOME", SCREEN_W / 2, 299, 1);
}

static void drawMicIcon(int cx, int cy, uint16_t color, uint16_t bg) {
  display.drawRoundRect(cx - 7, cy - 15, 14, 23, 7, color);
  display.drawFastHLine(cx - 3, cy - 7, 6, color);
  display.drawFastHLine(cx - 3, cy - 2, 6, color);
  display.drawFastHLine(cx - 3, cy + 3, 6, color);
  display.drawRoundRect(cx - 13, cy - 2, 26, 19, 9, color);
  display.fillRect(cx - 10, cy - 2, 20, 9, bg);
  display.drawFastVLine(cx, cy + 16, 7, color);
  display.drawFastHLine(cx - 7, cy + 23, 14, color);
}

static void drawMouseIcon(int cx, int cy, uint16_t color) {
  display.drawRoundRect(cx - 13, cy - 18, 26, 36, 10, color);
  display.drawFastVLine(cx, cy - 17, 12, color);
  display.fillCircle(cx, cy - 8, 2, color);
}

static void drawChatIcon(int cx, int cy, uint16_t color) {
  display.drawRoundRect(cx - 18, cy - 13, 36, 25, 5, color);
  display.fillTriangle(cx - 9, cy + 12, cx - 2, cy + 12, cx - 9, cy + 19, color);
  display.fillCircle(cx - 8, cy, 2, color);
  display.fillCircle(cx, cy, 2, color);
  display.fillCircle(cx + 8, cy, 2, color);
}

static void drawComputerIcon(int cx, int cy, uint16_t color) {
  display.drawRoundRect(cx - 18, cy - 14, 36, 25, 4, color);
  display.drawFastHLine(cx - 12, cy + 16, 24, color);
  display.drawFastVLine(cx, cy + 11, 5, color);
  display.drawFastHLine(cx - 6, cy - 4, 4, color);
  display.drawFastHLine(cx - 6, cy + 2, 12, color);
}

static void drawBatteryIcon(int x, int y, uint16_t color) {
  display.drawRect(x, y, 24, 12, color);
  display.fillRect(x + 24, y + 3, 3, 6, color);
  display.fillRect(x + 3, y + 3, 14, 6, color);
}

static void drawWifiIcon(int cx, int cy, uint16_t color) {
  display.drawArc(cx, cy + 5, 18, 16, 225, 315, color, C_BG);
  display.drawArc(cx, cy + 5, 11, 9, 225, 315, color, C_BG);
  display.fillCircle(cx, cy + 6, 2, color);
}

static void drawHome() {
  display.fillScreen(C_BG);
  display.setTextDatum(TL_DATUM);
  display.setTextColor(C_CYAN, C_BG);
  display.drawString("AI", 8, 8, 1);
  drawWifiIcon(128, 13, g_gatewayReady ? C_GREEN : C_GREY);
  drawBatteryIcon(143, 8, C_YELLOW);

  // Four primary functions: chat, voice, computer control, mouse.
  const int xs[] = {48, 124, 48, 124};
  const int ys[] = {94, 94, 176, 176};
  for (int i = 0; i < 4; ++i) {
    display.drawRoundRect(xs[i] - 29, ys[i] - 29, 58, 58, 9,
                          i == 0 ? C_CYAN : C_GREY);
  }
  drawChatIcon(xs[0], ys[0], C_CYAN);
  drawMicIcon(xs[1], ys[1], C_YELLOW, C_BG);
  drawComputerIcon(xs[0], ys[2], C_GREEN);
  drawMouseIcon(xs[1], ys[2], C_WHITE);
  drawFooter();
}

static void drawTools() {
  display.fillScreen(C_BG);
  drawHeader("TOOLS");
  const int xs[] = {48, 124, 48, 124};
  const int ys[] = {88, 88, 178, 178};
  for (int i = 0; i < 4; ++i) {
    display.drawRoundRect(xs[i] - 29, ys[i] - 29, 58, 58, 9,
                          i == 0 ? C_CYAN : C_GREY);
  }
  drawMouseIcon(xs[0], ys[0], C_CYAN);
  drawChatIcon(xs[1], ys[1], C_WHITE);
  drawMicIcon(xs[0], ys[2], C_YELLOW);
  display.drawCircle(xs[1], ys[3], 16, C_GREEN);
  display.setTextDatum(MC_DATUM);
  display.setTextColor(C_GREEN, C_BG);
  display.drawString("i", xs[1], ys[3] - 1, 2);
  drawFooter();
}

static void drawChat() {
  display.fillScreen(C_BG);
  drawHeader("CHAT");
  drawChatIcon(SCREEN_W / 2, 100, C_CYAN);
  display.setTextDatum(MC_DATUM);
  display.setTextColor(C_WHITE, C_BG);
  display.drawString("ask", SCREEN_W / 2, 139, 2);
  display.setTextColor(C_GREY, C_BG);
  display.drawString("touch bottom mic to speak", SCREEN_W / 2, 190, 1);
  display.drawRoundRect(16, 220, 140, 34, 8, C_PANEL);
  display.setTextColor(C_GREY, C_PANEL);
  display.drawString("no messages", SCREEN_W / 2, 237, 1);
  drawFooter();
}

static void drawVoice() {
  display.fillScreen(C_BG);
  drawHeader("VOICE");
  display.drawRoundRect(25, 62, 122, 122, 22, C_PANEL);
  drawMicIcon(SCREEN_W / 2, 116, C_YELLOW, C_PANEL);
  display.setTextDatum(MC_DATUM);
  display.setTextColor(C_WHITE, C_PANEL);
  display.drawString("HOLD", SCREEN_W / 2, 157, 2);
  display.setTextColor(C_GREY, C_BG);
  display.drawString("release to send", SCREEN_W / 2, 220, 1);
  drawFooter();
}

static void drawControl() {
  display.fillScreen(C_BG);
  drawHeader("CONTROL");
  drawComputerIcon(SCREEN_W / 2, 96, C_GREEN);
  display.setTextDatum(MC_DATUM);
  display.setTextColor(C_WHITE, C_BG);
  display.drawString("computer", SCREEN_W / 2, 137, 2);
  display.setTextColor(C_GREY, C_BG);
  display.drawString("commands will appear here", SCREEN_W / 2, 188, 1);
  drawFooter();
}

static void drawStatus() {
  display.fillScreen(C_BG);
  drawHeader("INFO");
  display.setTextDatum(TL_DATUM);
  display.setTextColor(C_GREEN, C_BG);
  display.drawString("LCD", 12, 65, 1);
  display.setTextColor(C_WHITE, C_BG);
  display.drawString("Seeed_GFX2 / JD9853A", 12, 81, 1);
  display.setTextColor(C_GREEN, C_BG);
  display.drawString("TP", 12, 111, 1);
  display.setTextColor(C_WHITE, C_BG);
  display.drawString("AXS5106L / ready", 12, 127, 1);
  display.setTextColor(C_GREEN, C_BG);
  display.drawString("LINK", 12, 157, 1);
  display.setTextColor(C_WHITE, C_BG);
  display.drawString("gateway not started", 12, 173, 1);
  display.setTextColor(C_YELLOW, C_BG);
  display.drawString("v0.1", 12, 225, 1);
  drawFooter();
}

bool g_mouseTools = false;
bool g_mouseDragging = false;
int g_mouseZone = -1;
bool g_mouseMoved = false;
uint32_t g_mouseStartMs = 0;
int g_scrollRemainder = 0;
bool g_tapPending = false;
uint32_t g_tapReleasedMs = 0;
int32_t g_tapX = 0, g_tapY = 0;

static void mouseClick() {
  sendGatewayLine("{\"type\":\"mouse_click\",\"button\":\"left\"}");
}

static void drawMouse() {
  display.fillScreen(C_BG);
  drawWifiIcon(128, 13, g_gatewayReady ? C_GREEN : C_GREY);
  drawBatteryIcon(143, 8, C_YELLOW);
  display.drawRoundRect(8, 36, 36, 28, 4, C_CYAN);
  display.setTextDatum(MC_DATUM);
  display.setTextColor(C_CYAN, C_BG);
  display.drawString(g_mouseTools ? "X" : "...", 26, 50, 2);
  if (g_mouseTools) {
    const char *labels[] = {"ALL", "COPY", "PASTE", "UNDO"};
    for (int i = 0; i < 4; ++i) {
      int x = 12 + (i % 2) * 80, y = 82 + (i / 2) * 88;
      display.drawRoundRect(x, y, 68, 72, 6, C_CYAN);
      // Small line icons, with short labels to make shortcuts discoverable.
      if (i == 0) display.drawRect(x + 22, y + 12, 24, 22, C_WHITE);
      if (i == 1) {
        display.drawRect(x + 19, y + 10, 21, 21, C_WHITE);
        display.drawRect(x + 26, y + 17, 21, 21, C_WHITE);
      }
      if (i == 2) {
        display.drawRoundRect(x + 22, y + 12, 24, 27, 3, C_WHITE);
        display.drawRect(x + 28, y + 9, 12, 6, C_WHITE);
      }
      if (i == 3) {
        display.drawLine(x + 22, y + 23, x + 47, y + 23, C_WHITE);
        display.drawLine(x + 22, y + 23, x + 30, y + 15, C_WHITE);
        display.drawLine(x + 22, y + 23, x + 30, y + 31, C_WHITE);
      }
      display.drawString(labels[i], x + 34, y + 55, 1);
    }
  } else {
    display.drawFastVLine(145, 76, 194, C_PANEL);
    display.drawString("^", 158, 90, 2);
    display.drawString("v", 158, 256, 2);
    if (g_mouseDragging) display.drawString("DRAG", 88, 50, 1);
  }
  drawMouseFooter();
}

static void drawMouseFooter() {
  display.fillRect(0, 278, SCREEN_W, 42, C_BG);
  display.drawRoundRect(52, 286, 68, 25, 5, C_CYAN);
  display.setTextDatum(MC_DATUM);
  display.setTextColor(C_CYAN, C_BG);
  display.drawString("HOME", SCREEN_W / 2, 299, 1);
}

static void drawCurrentScreen() {
  if (g_screen == Screen::Home) drawHome();
  else if (g_screen == Screen::Chat) drawChat();
  else if (g_screen == Screen::Voice) drawVoice();
  else if (g_screen == Screen::Control) drawControl();
  else if (g_screen == Screen::Tools) drawTools();
  else if (g_screen == Screen::Status) drawStatus();
  else drawMouse();
}

static void sendHello() {
  sendGatewayLine("{\"type\":\"hello\",\"device\":\"pocket-terminal\",\"version\":\"0.2\",\"transport\":\"wifi\"}");
}

static void handleGatewayLine(const String &line) {
  if (line == "READY") {
    g_gatewayReady = true;
    g_lastReply = "Gateway connected";
    drawCurrentScreen();
  } else if (line == "PONG") {
    if (!g_gatewayReady) {
      g_gatewayReady = true;
      drawCurrentScreen();
    }
  } else if (line.startsWith("REPLY:")) {
    g_gatewayReady = true;
    g_lastReply = line.substring(6);
    if (g_lastReply.length() > 25) g_lastReply = g_lastReply.substring(0, 25);
    drawCurrentScreen();
  } else if (line.startsWith("ERROR:")) {
    g_lastReply = line.substring(6);
    if (g_lastReply.length() > 25) g_lastReply = g_lastReply.substring(0, 25);
    drawCurrentScreen();
  }
}

static void serviceGateway() {
  static String line;
  static uint32_t lastConnectMs = 0;
  if (!g_gatewayClient.connected() && g_wifiTransport) {
    g_gatewayClient.stop();
    g_gatewayReady = false;
    g_wifiTransport = false;
    line = "";
    g_mouseDragging = false;
    g_tapPending = false;
    g_mouseGestureActive = false;
    drawCurrentScreen();
  }
  if (wifiConfigured() && WiFi.status() == WL_CONNECTED &&
      !g_gatewayClient.connected() && millis() - lastConnectMs >= 3000) {
    lastConnectMs = millis();
    if (g_gatewayClient.connect(GATEWAY_HOST, GATEWAY_PORT, 100)) {
      g_gatewayClient.setNoDelay(true);
      g_wifiTransport = true;
      line = "";
      sendHello();
    }
  }
  Stream *input = g_wifiTransport ? static_cast<Stream *>(&g_gatewayClient) : static_cast<Stream *>(&Serial);
  while (input->available()) {
    char c = (char)input->read();
    if (c == '\n') {
      line.trim();
      if (line.length()) handleGatewayLine(line);
      line = "";
    } else if (c != '\r' && line.length() < 160) {
      line += c;
    }
  }

  uint32_t now = millis();
  if (now - g_lastHeartbeatMs >= 3000) {
    g_lastHeartbeatMs = now;
    sendGatewayLine("{\"type\":\"ping\"}");
  }
}

static void handleTouch(int32_t x, int32_t y) {
  // Mouse owns its entire page. Never fall through to launcher navigation.
  if (g_screen == Screen::Mouse) return;
  Screen previous = g_screen;
  if (x >= 52 && x < 120 && y >= 286 && y < 311) {
    g_screen = Screen::Home;
  } else if (g_screen == Screen::Home && y >= 65 && y < 130) {
    if (x < 86) g_screen = Screen::Chat;
    else g_screen = Screen::Voice;
  } else if (g_screen == Screen::Home && y >= 145 && y < 210) {
    if (x < 86) g_screen = Screen::Control;
    else g_screen = Screen::Mouse;
  } else if (g_screen == Screen::Tools && y >= 59 && y < 117) {
    g_screen = x < 86 ? Screen::Mouse : Screen::Chat;
  } else if (g_screen == Screen::Tools && y >= 149 && y < 207) {
    g_screen = x < 86 ? Screen::Voice : Screen::Status;
  } else {
    return;
  }
  if (previous == g_screen) return;
  g_mouseGestureActive = false;
  g_mouseGestureStartedInPad = false;
  g_mouseHomeCandidate = false;
  drawCurrentScreen();
  Serial.printf("[UI] touch=(%ld,%ld) screen=%d\n", (long)x, (long)y, (int)g_screen);
}

// A touch owns its starting zone until release; crossing HOME cannot exit.
static int mouseZone(int32_t x, int32_t y) {
  if (y >= 286 && y < 311) {
    if (x >= 52 && x < 120) return 2;
  }
  if (x >= 8 && x < 44 && y >= 36 && y < 64) return 4;
  if (g_mouseTools) {
    for (int i = 0; i < 4; ++i) {
      int bx = 12 + (i % 2) * 80, by = 82 + (i / 2) * 88;
      if (x >= bx && x < bx + 68 && y >= by && y < by + 72) return 10 + i;
    }
    return -1;
  }
  if (y >= 76 && y < 278) return x >= 145 ? 5 : 0;
  return -1;
}

static void releaseMouse() {
  sendGatewayLine("{\"type\":\"mouse_up\",\"button\":\"left\"}");
  g_mouseDragging = false;
}

static void serviceMouse(int32_t x, int32_t y, bool touching) {
  if (g_screen != Screen::Mouse) return;
  uint32_t now = millis();
  if (g_tapPending && now - g_tapReleasedMs > 320) {
    mouseClick();
    g_tapPending = false;
  }
  if (touching) {
    if (!g_mouseGestureActive) {
      if (g_wasTouching) return;
      g_mouseGestureActive = true;
      g_mouseZone = mouseZone(x, y);
      g_mouseStartMs = now;
      g_mouseMoved = false;
      g_mouseHomeX = g_lastTouchX = x;
      g_mouseHomeY = g_lastTouchY = y;
      g_scrollRemainder = 0;
      bool secondTap = g_tapPending && g_mouseZone == 0 &&
                       abs(x - g_tapX) <= 24 && abs(y - g_tapY) <= 24;
      if (g_tapPending && !secondTap) mouseClick();
      g_tapPending = false;
      if (secondTap) {
        sendGatewayLine("{\"type\":\"mouse_down\",\"button\":\"left\"}");
        g_mouseDragging = true;
        drawMouse();
      }
    }
    g_mouseLastTouchMs = now;
    int moveThreshold = g_mouseDragging ? 3 : 8;
    if (abs(x - g_mouseHomeX) > moveThreshold || abs(y - g_mouseHomeY) > moveThreshold)
      g_mouseMoved = true;
    int32_t dx = x - g_lastTouchX, dy = y - g_lastTouchY;
    if (g_mouseZone == 0 && y >= 76 && y < 278 && x < 145 &&
        g_wasTouching && g_mouseMoved && (dx || dy)) {
      String event = String("{\"type\":\"mouse_move\",\"dx\":") +
                     String((long)(dx * (g_mouseDragging ? 1 : 2))) +
                     String(",\"dy\":") +
                     String((long)(dy * (g_mouseDragging ? 1 : 2))) + "}";
      sendGatewayLine(event);
    }
    if (g_mouseZone == 5 && g_wasTouching) {
      g_scrollRemainder -= dy;
      int steps = g_scrollRemainder / 18;
      if (steps) {
        sendGatewayLine(String("{\"type\":\"mouse_scroll\",\"steps\":") + String(steps) + "}");
        g_scrollRemainder -= steps * 18;
      }
    }
    g_lastTouchX = x;
    g_lastTouchY = y;
    return;
  }
  // Short tap releases must be recognized before the next tap. Keep the
  // longer dropout guard for swipes so crossing HOME still cannot navigate.
  uint32_t releaseDelay = (g_mouseZone == 0 && !g_mouseMoved) ? 30 : 120;
  if (!g_mouseGestureActive || now - g_mouseLastTouchMs <= releaseDelay) return;
  bool wasDragging = g_mouseDragging;
  if (wasDragging) {
    releaseMouse();
    // Two quick taps without movement remain an ordinary double-click.
    if (!g_mouseMoved && g_mouseLastTouchMs - g_mouseStartMs <= 250) mouseClick();
    drawMouse();
  }
  if (!g_mouseMoved && !wasDragging &&
      mouseZone(g_lastTouchX, g_lastTouchY) == g_mouseZone) {
    if (g_mouseZone == 0 && g_mouseLastTouchMs - g_mouseStartMs <= 250) {
      g_tapPending = true;
      g_tapReleasedMs = g_mouseLastTouchMs;
      g_tapX = g_lastTouchX;
      g_tapY = g_lastTouchY;
    } else if (g_mouseZone == 2) {
      releaseMouse();
      g_mouseTools = false;
      g_screen = Screen::Home;
      drawCurrentScreen();
    } else if (g_mouseZone == 4) {
      g_mouseTools = !g_mouseTools;
      drawMouse();
    } else if (g_mouseZone >= 10 && g_mouseZone <= 13) {
      const char *actions[] = {"select_all", "copy", "paste", "undo"};
      sendGatewayLine(String("{\"type\":\"shortcut\",\"action\":\"") +
                      actions[g_mouseZone - 10] + "\"}");
      g_mouseTools = false;
      drawMouse();
    }
  }
  g_mouseGestureActive = false;
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n=== Pocket AI Tool Terminal v0.1 ===");

  if (wifiConfigured()) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    uint32_t started = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - started < 12000) delay(100);
    if (WiFi.status() == WL_CONNECTED) {
      g_wifiTransport = true;
      Serial.printf("[WiFi] connected: %s\n", WiFi.localIP().toString().c_str());
      if (g_gatewayClient.connect(GATEWAY_HOST, GATEWAY_PORT))
        Serial.printf("[WiFi] gateway connected: %s:%u\n", GATEWAY_HOST, GATEWAY_PORT);
      else
        Serial.printf("[WiFi] gateway unavailable: %s:%u\n", GATEWAY_HOST, GATEWAY_PORT);
    } else {
      Serial.println("[WiFi] connection timeout; using USB serial fallback");
    }
  } else {
    Serial.println("[WiFi] not configured; using USB serial fallback");
  }

  if (!display.begin<Board_XIAO_1inch47_Touch_Display<LCD_RST_PIN, LCD_BL_PIN>,
                     Config_Seeed_1inch47_Touch_JD9853A>()) {
    Serial.println(display.lastResult().message);
    return;
  }
  if (!display.attachTouch(touch, display.panel().driver().bus())) {
    Serial.println(display.lastResult().message);
    return;
  }
  drawCurrentScreen();
  sendHello();
  Serial.println("[OK] display + touch ready");
}

void loop() {
  serviceGateway();
  int32_t x = 0;
  int32_t y = 0;
  bool touching = display.getTouch(&x, &y);
  serviceMouse(x, y, touching);
  if (touching && !g_wasTouching) handleTouch(x, y);
  g_wasTouching = touching;
  delay(10);
}
