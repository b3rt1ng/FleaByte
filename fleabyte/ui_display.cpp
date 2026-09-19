#include "ui_display.h"
#include "config.h"

#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>

static constexpr uint16_t rgb(uint8_t r, uint8_t g, uint8_t b) {
#if TFT_SWAP_RED_BLUE
  return ((uint16_t)(b & 0xF8) << 8) | ((uint16_t)(g & 0xFC) << 3) | (r >> 3);
#else
  return ((uint16_t)(r & 0xF8) << 8) | ((uint16_t)(g & 0xFC) << 3) | (b >> 3);
#endif
}

static constexpr uint16_t C_BG      = rgb(0x00, 0x00, 0x00);
static constexpr uint16_t C_TEXT    = rgb(0xE6, 0xFB, 0xF6);
static constexpr uint16_t C_DIM     = rgb(0x3E, 0x6B, 0x63);
static constexpr uint16_t C_CYAN    = rgb(0x00, 0xE5, 0xD0);
static constexpr uint16_t C_MAGENTA = rgb(0xFF, 0x2D, 0x8A);
static constexpr uint16_t C_LIME    = rgb(0xB6, 0xFF, 0x3C);
static constexpr uint16_t C_RED     = rgb(0xFF, 0x3B, 0x30);
static constexpr uint16_t C_AMBER   = rgb(0xFF, 0xA5, 0x00);

static SPIClass tftSPI(FSPI);
static Adafruit_ST7735 tft(&tftSPI, TFT_CS, TFT_DC, TFT_RST);

static uint8_t s_rotation = TFT_ROTATION;
static bool s_screenOn = true;
static bool s_ledOn = true;
static uint8_t s_ledR = LED_DEFAULT_R, s_ledG = LED_DEFAULT_G, s_ledB = LED_DEFAULT_B;
static uint32_t s_wakeUntil = 0;
static bool s_backlightLit = true;
static bool s_fullRepaint = true;
static bool s_splash = false;

static bool isLandscape() { return s_rotation == 1 || s_rotation == 3; }
static int16_t screenW() { return isLandscape() ? 160 : 80; }
static int16_t screenH() { return isLandscape() ? 80 : 160; }

static void apa102Byte(uint8_t b) {
  for (int i = 7; i >= 0; i--) {
    digitalWrite(LED_DI_PIN, (b >> i) & 0x01);
    digitalWrite(LED_CI_PIN, HIGH);
    digitalWrite(LED_CI_PIN, LOW);
  }
}

void ledSet(uint8_t r, uint8_t g, uint8_t b) {
  for (int i = 0; i < 4; i++) apa102Byte(0x00);
  apa102Byte(0xE0 | (LED_BRIGHTNESS & 0x1F));
  apa102Byte(b);
  apa102Byte(g);
  apa102Byte(r);
  for (int i = 0; i < 4; i++) apa102Byte(0xFF);
}

static void ledShow(uint8_t r, uint8_t g, uint8_t b) {
  if (!s_ledOn) {
    ledSet(0, 0, 0);
    return;
  }
  ledSet(r, g, b);
}

static void applyBacklight() {
  bool lit = s_screenOn || (millis() < s_wakeUntil);
  if (lit == s_backlightLit) return;
  s_backlightLit = lit;

  ledcWrite(TFT_BL, lit ? TFT_BL_DUTY_ON : 255);
}

static void putText(int16_t x, int16_t y, const String &t, uint16_t color, uint8_t size) {
  tft.setTextSize(size);
  tft.setTextColor(color);
  tft.setCursor(x, y);
  tft.print(t);
}

static String truncate(const String &s, size_t maxChars) {
  if (s.length() <= maxChars) return s;
  return s.substring(0, maxChars);
}

static size_t charsPerLine() { return (screenW() - 8) / 6; }

enum FieldId : uint8_t { F_SSID, F_IP, F_CLIENTS, F_STATE, F_COUNT };

struct Field {
  int16_t x, y, w, h;
  uint8_t size;
  uint16_t color;
  String text;
  uint8_t decode;
};

static Field s_f[F_COUNT];

static const char NOISE[] = "!<>-_\\/[]{}=+*^?#%$&@01";

static String scramble(const String &target, uint8_t framesLeft, uint8_t framesTotal) {
  String out;
  out.reserve(target.length());
  const size_t settledUpTo = target.length() * (framesTotal - framesLeft) / framesTotal;
  for (size_t i = 0; i < target.length(); i++) {
    if (i < settledUpTo || target[i] == ' ') {
      out += target[i];
    } else {
      out += NOISE[random(sizeof(NOISE) - 1)];
    }
  }
  return out;
}

// Cleared area must cover the glitch ghosts, which land outside the field.
static constexpr int16_t GHOST_DX = 2;

static void paintField(FieldId id, bool glitched) {
  Field &f = s_f[id];

  int16_t cx = f.x - GHOST_DX;
  int16_t cw = f.w + GHOST_DX * 2;
  if (cx < 0) {
    cw += cx;
    cx = 0;
  }
  tft.fillRect(cx, f.y, cw, f.h, C_BG);

  String shown = f.decode ? scramble(f.text, f.decode, 6) : f.text;

  if (glitched) {

    putText(f.x - GHOST_DX, f.y, shown, C_MAGENTA, f.size);
    putText(f.x + GHOST_DX, f.y, shown, C_CYAN, f.size);
  }
  putText(f.x, f.y, shown, f.decode ? C_CYAN : f.color, f.size);
}

static void setField(FieldId id, const String &text, uint16_t color) {
  Field &f = s_f[id];
  if (f.text == text && f.color == color) return;
  f.text = text;
  f.color = color;
  f.decode = 6;
  paintField(id, false);
}

static constexpr int16_t L_LEFT = 14, L_RIGHT = 146;
static constexpr int16_t P_LEFT = 6, P_RIGHT = 74;

static void placeFields() {
  if (isLandscape()) {
    s_f[F_SSID]    = {40, 21, 106, 8, 1, C_TEXT, s_f[F_SSID].text, 0};
    s_f[F_IP]      = {40, 32, 106, 8, 1, C_DIM, s_f[F_IP].text, 0};
    s_f[F_CLIENTS] = {56, 50, 26, 16, 2, C_CYAN, s_f[F_CLIENTS].text, 0};
    s_f[F_STATE]   = {92, 54, 54, 8, 1, C_DIM, s_f[F_STATE].text, 0};
  } else {
    s_f[F_SSID]    = {P_LEFT, 42, 68, 8, 1, C_TEXT, s_f[F_SSID].text, 0};
    s_f[F_IP]      = {P_LEFT, 66, 68, 8, 1, C_DIM, s_f[F_IP].text, 0};
    s_f[F_CLIENTS] = {P_LEFT, 98, 40, 24, 3, C_CYAN, s_f[F_CLIENTS].text, 0};
    s_f[F_STATE]   = {18, 130, 56, 8, 1, C_DIM, s_f[F_STATE].text, 0};
  }
}

// The panel clips its outermost row and column, so the frame sits inset.
static constexpr int16_t FRAME_INSET = 3;
static constexpr int16_t FRAME_ARM = 9;
static constexpr int16_t FRAME_THICK = 2;

static void drawCorners(uint16_t color) {
  const int16_t l = FRAME_INSET;
  const int16_t t = FRAME_INSET;
  const int16_t r = screenW() - 1 - FRAME_INSET;
  const int16_t b = screenH() - 1 - FRAME_INSET;

  for (int16_t k = 0; k < FRAME_THICK; k++) {

    tft.drawFastHLine(l, t + k, FRAME_ARM, color);
    tft.drawFastVLine(l + k, t, FRAME_ARM, color);

    tft.drawFastHLine(r - FRAME_ARM + 1, t + k, FRAME_ARM, color);
    tft.drawFastVLine(r - k, t, FRAME_ARM, color);

    tft.drawFastHLine(l, b - k, FRAME_ARM, color);
    tft.drawFastVLine(l + k, b - FRAME_ARM + 1, FRAME_ARM, color);

    tft.drawFastHLine(r - FRAME_ARM + 1, b - k, FRAME_ARM, color);
    tft.drawFastVLine(r - k, b - FRAME_ARM + 1, FRAME_ARM, color);
  }
}

static void drawSdIcon(int16_t x, int16_t y, uint16_t color) {
  const int16_t w = 9, h = 12, bevel = 3;

  for (int16_t j = 0; j < h; j++) {
    tft.drawFastHLine(x, y + j, (j < bevel) ? (w - bevel + j) : w, color);
  }
  for (int16_t i = 2; i <= 6; i += 2) {
    tft.drawFastVLine(x + i, y + h - 4, 3, C_BG);
  }
}

static void paintSdState(int8_t state) {
  const int16_t x = isLandscape() ? 96 : 60;
  const int16_t y = isLandscape() ? 2 : 17;
  tft.fillRect(x, y, 10, 13, C_BG);
  if (state == 1) drawSdIcon(x, y, C_CYAN);
  else if (state == 2) drawSdIcon(x, y, C_AMBER);
}

static void drawDashed(int16_t y, int16_t from, int16_t to, uint16_t color) {
  for (int16_t x = from; x < to; x += 4) {
    tft.drawFastHLine(x, y, 2, color);
  }
}

static void drawChrome(const String &arrangement, const String &name) {
  tft.fillScreen(C_BG);
  drawCorners(C_CYAN);

  String badge = arrangement;

  if (isLandscape()) {

    putText(L_LEFT, 4, "//" + truncate(name, 13), C_CYAN, 1);
    putText(L_RIGHT - badge.length() * 6, 4, badge, C_MAGENTA, 1);
    tft.drawFastHLine(L_LEFT, 15, L_RIGHT - L_LEFT, C_DIM);
    putText(L_LEFT, 21, "NET", C_DIM, 1);
    putText(L_LEFT, 32, "IP", C_DIM, 1);
    drawDashed(44, L_LEFT, L_RIGHT, C_DIM);
    putText(L_LEFT, 54, "NODES", C_DIM, 1);
  } else {
    putText(P_LEFT + 8, 4, "//" + truncate(name, 7), C_CYAN, 1);
    tft.drawFastHLine(P_LEFT, 15, P_RIGHT - P_LEFT, C_DIM);
    putText(P_LEFT, 22, badge, C_MAGENTA, 1);
    putText(P_LEFT, 32, "NET", C_DIM, 1);
    putText(P_LEFT, 56, "IP", C_DIM, 1);
    drawDashed(84, P_LEFT, P_RIGHT, C_DIM);
    putText(P_LEFT, 88, "NODES", C_DIM, 1);
  }
}

void displayBegin() {
  pinMode(LED_DI_PIN, OUTPUT);
  pinMode(LED_CI_PIN, OUTPUT);
  digitalWrite(LED_CI_PIN, LOW);
  ledSet(0, 0, 0);

  tftSPI.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);
  tft.initR(INITR_MINI160x80);
  tft.setRotation(s_rotation);
  tft.invertDisplay(true);
  tft.fillScreen(C_BG);

  ledcAttach(TFT_BL, 1000, 8);
  ledcWrite(TFT_BL, TFT_BL_DUTY_ON);
  s_backlightLit = true;
}

void displaySetRotation(uint8_t rotation) {
  if (rotation > 3 || rotation == s_rotation) return;
  s_rotation = rotation;
  tft.setRotation(s_rotation);
  tft.fillScreen(C_BG);
  s_fullRepaint = true;
}

uint8_t displayGetRotation() { return s_rotation; }

void displaySetScreenOn(bool on) {
  s_screenOn = on;
  applyBacklight();
}

void displayWake() {
  s_wakeUntil = millis() + SCREEN_WAKE_MS;
  applyBacklight();
}

void displaySetLed(bool on, uint8_t r, uint8_t g, uint8_t b) {
  s_ledOn = on;
  s_ledR = r;
  s_ledG = g;
  s_ledB = b;
  ledShow(s_ledR, s_ledG, s_ledB);
}

void displayShowJoin(const String &ssid, const String &password) {
  s_fullRepaint = true;
  s_splash = true;
  applyBacklight();
  tft.fillScreen(C_BG);
  drawCorners(C_MAGENTA);

  putText(14, 4, "//ACCESS", C_CYAN, 1);
  tft.drawFastHLine(8, 15, screenW() - 16, C_DIM);

  putText(8, 22, "SSID", C_DIM, 1);
  putText(8, 32, truncate(ssid, charsPerLine()), C_TEXT, 1);
  putText(8, 48, "KEY", C_DIM, 1);
  putText(8, 58, truncate(password, charsPerLine()), C_LIME, 1);

  ledShow(s_ledR, s_ledG, s_ledB);
}

void displayShowMessage(const String &title, const String &detail) {
  s_fullRepaint = true;
  s_splash = true;
  displayWake();
  tft.fillScreen(C_BG);
  drawCorners(C_RED);

  putText(14, 4, "//ALERT", C_MAGENTA, 1);
  tft.drawFastHLine(8, 15, screenW() - 16, C_DIM);
  putText(8, 30, truncate(title, charsPerLine()), C_TEXT, 1);
  putText(8, 44, truncate(detail, charsPerLine()), C_DIM, 1);

  ledShow(255, 140, 0);
}

static String s_arrangement, s_name;
static int8_t s_sdState = -1;
static int s_clients = -1;
static DuckyState s_duckyState = DUCKY_IDLE;
static int s_line = -1, s_total = -1;
static bool s_running = false;
static bool s_armed = false;

void displayUpdate(const DisplayInfo &info) {
  applyBacklight();

  if (s_fullRepaint || s_splash) {
    s_fullRepaint = false;
    s_splash = false;
    s_arrangement = info.arrangement;
    s_name = info.name;
    placeFields();
    drawChrome(info.arrangement, info.name);
    s_sdState = -1;

    for (uint8_t i = 0; i < F_COUNT; i++) s_f[i].text = "";
    s_clients = -1;
  }

  if (info.arrangement != s_arrangement || info.name != s_name) {
    s_arrangement = info.arrangement;
    s_name = info.name;
    drawChrome(info.arrangement, info.name);
    s_sdState = -1;
    for (uint8_t i = 0; i < F_COUNT; i++) s_f[i].text = "";
    s_clients = -1;
  }

  int8_t sd = info.sdExposed ? 2 : (info.sdPresent ? 1 : 0);
  if (sd != s_sdState) {
    s_sdState = sd;
    paintSdState(sd);
  }

  setField(F_SSID, truncate(info.ssid, isLandscape() ? 17 : 11), C_TEXT);
  setField(F_IP, truncate(info.ip, isLandscape() ? 17 : 11), C_DIM);

  if (info.clients != s_clients) {
    s_clients = info.clients;
    setField(F_CLIENTS, String(info.clients), info.clients > 0 ? C_CYAN : C_DIM);
  }

  String state;
  uint16_t color = C_DIM;
  switch (info.ducky.state) {
    case DUCKY_ARMED:   state = "ARMED " + String(info.ducky.countdown) + "S"; color = C_MAGENTA; break;
    case DUCKY_RUNNING: state = "EXEC " + String(info.ducky.line) + "/" + String(info.ducky.total); color = C_AMBER; break;
    case DUCKY_DONE:    state = "DONE";    color = C_LIME; break;
    case DUCKY_ABORTED: state = "HALTED";  color = C_AMBER; break;
    case DUCKY_ERROR:   state = "FAULT";   color = C_RED; break;
    default:            state = "STANDBY"; color = C_DIM; break;
  }
  setField(F_STATE, state, color);

  s_duckyState = info.ducky.state;
  s_line = info.ducky.line;
  s_total = info.ducky.total;
  s_running = (info.ducky.state == DUCKY_RUNNING);
  s_armed = (info.ducky.state == DUCKY_ARMED);

  switch (info.ducky.state) {
    case DUCKY_ARMED:   ledShow(255, 45, 138); break;
    case DUCKY_RUNNING: ledShow(255, 170, 0); break;
    case DUCKY_ERROR:   ledShow(255, 0, 0); break;
    case DUCKY_DONE:    ledShow(0, 180, 90); break;
    default:            ledShow(s_ledR, s_ledG, s_ledB); break;
  }
}

static uint32_t s_nextFrame = 0;
static uint32_t s_nextGlitch = 0;
static int8_t s_glitchField = -1;
static uint8_t s_blink = 0;

static void drawProgress() {
  const int16_t x = isLandscape() ? L_LEFT : P_LEFT;
  const int16_t y = isLandscape() ? 70 : 140;
  const int16_t w = isLandscape() ? (L_RIGHT - L_LEFT) : (P_RIGHT - P_LEFT);
  const uint8_t cells = isLandscape() ? 16 : 8;
  const int16_t cw = w / cells;

  uint8_t filled = 0;
  if (s_total > 0) filled = (uint8_t)((long)s_line * cells / s_total);

  for (uint8_t i = 0; i < cells; i++) {
    uint16_t c = (i < filled) ? C_AMBER : C_DIM;
    tft.fillRect(x + i * cw, y, cw - 2, 4, c);
  }
}

static void clearProgress() {
  const int16_t x = isLandscape() ? L_LEFT : P_LEFT;
  const int16_t y = isLandscape() ? 70 : 140;
  const int16_t w = isLandscape() ? (L_RIGHT - L_LEFT) : (P_RIGHT - P_LEFT);
  tft.fillRect(x, y, w, 4, C_BG);
}

void displayTick() {
  if (s_splash) return;

  uint32_t now = millis();
  if (now < s_nextFrame) return;
  s_nextFrame = now + 70;

  applyBacklight();

  bool decoding = false;
  for (uint8_t i = 0; i < F_COUNT; i++) {
    if (s_f[i].decode) {
      s_f[i].decode--;
      paintField((FieldId)i, false);
      decoding = true;
    }
  }

  if (s_glitchField >= 0) {
    paintField((FieldId)s_glitchField, false);
    s_glitchField = -1;
  } else if (!decoding && now > s_nextGlitch) {
    s_glitchField = random(F_COUNT);
    paintField((FieldId)s_glitchField, true);
    s_nextGlitch = now + random(2500, 7000);
  }

  static bool lastRunning = false;
  s_blink++;
  const int16_t bx = isLandscape() ? 84 : P_LEFT + 4;
  const int16_t by = isLandscape() ? 55 : 131;
  bool fast = s_running || s_armed;
  bool on = fast ? ((s_blink / 2) & 1) : ((s_blink / 8) & 1);
  uint16_t mark = s_armed ? C_MAGENTA : (s_running ? C_AMBER : C_CYAN);
  tft.fillRect(bx, by, 5, 5, on ? mark : C_BG);

  if (s_running) {
    drawProgress();
    lastRunning = true;
  } else if (lastRunning) {
    clearProgress();
    lastRunning = false;
  }
}
