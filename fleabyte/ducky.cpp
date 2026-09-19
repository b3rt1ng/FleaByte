#include "ducky.h"
#include "config.h"

#include "USB.h"
#include "USBHIDKeyboard.h"

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

static USBHIDKeyboard Keyboard;

struct LayoutEntry {
  const char *code;
  const char *name;
  const char *arrangement;
  const uint8_t *table;
};

static const LayoutEntry LAYOUTS[] = {
  {"us", "English (US)",       "QWERTY", KeyboardLayout_en_US},
  {"fr", "French",             "AZERTY", KeyboardLayout_fr_FR},
  {"de", "German",             "QWERTZ", KeyboardLayout_de_DE},
  {"ch", "Swiss French",       "QWERTZ", KeyboardLayout_fr_CH},
  {"hu", "Hungarian",          "QWERTZ", KeyboardLayout_hu_HU},
  {"es", "Spanish",            "QWERTY", KeyboardLayout_es_ES},
  {"it", "Italian",            "QWERTY", KeyboardLayout_it_IT},
  {"pt", "Portuguese",         "QWERTY", KeyboardLayout_pt_PT},
  {"br", "Portuguese (Brazil)","QWERTY", KeyboardLayout_pt_BR},
  {"se", "Swedish",            "QWERTY", KeyboardLayout_sv_SE},
  {"dk", "Danish",             "QWERTY", KeyboardLayout_da_DK},
  {"jp", "Japanese",           "JIS",    KeyboardLayout_ja_JP},
};

static const LayoutEntry *findLayout(const String &code) {

  String c = (code == "en") ? String("us") : code;
  for (const LayoutEntry &e : LAYOUTS) {
    if (c.equals(e.code)) return &e;
  }
  return nullptr;
}

size_t duckyLayoutCount() { return sizeof(LAYOUTS) / sizeof(LAYOUTS[0]); }

LayoutInfo duckyLayoutAt(size_t i) {
  const LayoutEntry &e = LAYOUTS[i];
  return LayoutInfo{e.code, e.name, e.arrangement};
}

bool duckyLayoutExists(const String &code) { return findLayout(code) != nullptr; }

static QueueHandle_t g_queue = nullptr;
static SemaphoreHandle_t g_mutex = nullptr;
static volatile bool g_abort = false;

static DuckyState g_state = DUCKY_IDLE;
static int g_line = 0;
static int g_total = 0;
static int g_countdown = 0;
static uint16_t g_pendingDelay = 0;
static String g_message = "Ready";
static String g_log;
static uint32_t g_logSeq = 0;
static String g_layoutCode = "us";

static uint32_t g_defaultDelay = DEFAULT_LINE_DELAY_MS;
static uint32_t g_charDelay = DEFAULT_CHAR_DELAY_MS;

struct KeyEntry {
  const char *name;
  uint8_t code;
};

static const KeyEntry KEY_TABLE[] = {
  {"ENTER", KEY_RETURN},
  {"RETURN", KEY_RETURN},
  {"TAB", KEY_TAB},
  {"ESC", KEY_ESC},
  {"ESCAPE", KEY_ESC},
  {"SPACE", ' '},
  {"BACKSPACE", KEY_BACKSPACE},
  {"DELETE", KEY_DELETE},
  {"DEL", KEY_DELETE},
  {"INSERT", KEY_INSERT},
  {"HOME", KEY_HOME},
  {"END", KEY_END},
  {"PAGEUP", KEY_PAGE_UP},
  {"PAGEDOWN", KEY_PAGE_DOWN},
  {"UP", KEY_UP_ARROW},
  {"UPARROW", KEY_UP_ARROW},
  {"DOWN", KEY_DOWN_ARROW},
  {"DOWNARROW", KEY_DOWN_ARROW},
  {"LEFT", KEY_LEFT_ARROW},
  {"LEFTARROW", KEY_LEFT_ARROW},
  {"RIGHT", KEY_RIGHT_ARROW},
  {"RIGHTARROW", KEY_RIGHT_ARROW},
  {"MENU", KEY_MENU},
  {"APP", KEY_MENU},
  {"CAPSLOCK", KEY_CAPS_LOCK},
  {"NUMLOCK", KEY_NUM_LOCK},
  {"SCROLLLOCK", KEY_SCROLL_LOCK},
  {"PRINTSCREEN", KEY_PRINT_SCREEN},
  {"PAUSE", KEY_PAUSE},
  {"BREAK", KEY_PAUSE},
  {"F1", KEY_F1},   {"F2", KEY_F2},   {"F3", KEY_F3},   {"F4", KEY_F4},
  {"F5", KEY_F5},   {"F6", KEY_F6},   {"F7", KEY_F7},   {"F8", KEY_F8},
  {"F9", KEY_F9},   {"F10", KEY_F10}, {"F11", KEY_F11}, {"F12", KEY_F12},
};

static const KeyEntry MOD_TABLE[] = {
  {"CTRL", KEY_LEFT_CTRL},
  {"CONTROL", KEY_LEFT_CTRL},
  {"SHIFT", KEY_LEFT_SHIFT},
  {"ALT", KEY_LEFT_ALT},
  {"GUI", KEY_LEFT_GUI},
  {"WINDOWS", KEY_LEFT_GUI},
  {"WIN", KEY_LEFT_GUI},
  {"CMD", KEY_LEFT_GUI},
  {"META", KEY_LEFT_GUI},
  {"SUPER", KEY_LEFT_GUI},
  {"ALTGR", KEY_RIGHT_ALT},
  {"RALT", KEY_RIGHT_ALT},
};

static bool lookupKey(const String &name, uint8_t &out) {
  for (const KeyEntry &e : KEY_TABLE) {
    if (name.equals(e.name)) {
      out = e.code;
      return true;
    }
  }
  return false;
}

static bool lookupMod(const String &name, uint8_t &out) {
  for (const KeyEntry &e : MOD_TABLE) {
    if (name.equals(e.name)) {
      out = e.code;
      return true;
    }
  }
  return false;
}

static void lock() { xSemaphoreTake(g_mutex, portMAX_DELAY); }
static void unlock() { xSemaphoreGive(g_mutex); }

static void logLine(const String &s) {
  lock();
  g_log += s;
  g_log += '\n';
  g_logSeq += s.length() + 1;
  if (g_log.length() > MAX_LOG_BYTES) {

    int cut = g_log.indexOf('\n', g_log.length() - MAX_LOG_BYTES);
    g_log = (cut >= 0) ? g_log.substring(cut + 1) : g_log.substring(g_log.length() - MAX_LOG_BYTES);
  }
  unlock();
}

static void setState(DuckyState st, const String &msg) {
  lock();
  g_state = st;
  g_message = msg;
  unlock();
}

static void setProgress(int line, int total) {
  lock();
  g_line = line;
  g_total = total;
  unlock();
}

static bool duckySleep(uint32_t ms) {
  const uint32_t step = 10;
  uint32_t waited = 0;
  while (waited < ms) {
    if (g_abort) return false;
    uint32_t chunk = (ms - waited < step) ? (ms - waited) : step;
    vTaskDelay(pdMS_TO_TICKS(chunk));
    waited += chunk;
  }
  return !g_abort;
}

static int typeString(const String &text) {
  int skipped = 0;
  for (size_t i = 0; i < text.length(); i++) {
    if (g_abort) return skipped;
    uint8_t c = (uint8_t)text[i];
    if (c < 0x20 || c > 0x7E) {
      skipped++;
      continue;
    }
    Keyboard.write(c);
    if (g_charDelay) vTaskDelay(pdMS_TO_TICKS(g_charDelay));
  }
  return skipped;
}

static bool pressCombo(String tokens[], int count, String &err) {
  uint8_t mods[8];
  int modCount = 0;
  int i = 0;

  for (; i < count; i++) {
    uint8_t m;
    String upper = tokens[i];
    upper.toUpperCase();
    if (!lookupMod(upper, m)) break;
    if (modCount < 8) mods[modCount++] = m;
  }

  uint8_t finalKey = 0;
  bool hasFinal = false;
  if (i < count) {
    String last = tokens[i];
    String upper = last;
    upper.toUpperCase();
    if (lookupKey(upper, finalKey)) {
      hasFinal = true;
    } else if (last.length() == 1) {

      finalKey = (uint8_t)last[0];
      hasFinal = true;
    } else {
      err = "Unknown key: " + last;
      return false;
    }
    if (i + 1 < count) {
      err = "Too many arguments after " + last;
      return false;
    }
  }

  if (modCount == 0 && !hasFinal) {
    err = "Empty line";
    return false;
  }

  for (int k = 0; k < modCount; k++) Keyboard.press(mods[k]);
  if (hasFinal) Keyboard.press(finalKey);
  vTaskDelay(pdMS_TO_TICKS(20));
  Keyboard.releaseAll();
  return true;
}

static void splitTokens(const String &line, String out[], int maxTokens, int &count) {
  count = 0;
  int i = 0;
  while (i < (int)line.length() && count < maxTokens) {
    while (i < (int)line.length() && line[i] == ' ') i++;
    if (i >= (int)line.length()) break;
    int start = i;
    while (i < (int)line.length() && line[i] != ' ') i++;
    out[count++] = line.substring(start, i);
  }
}

static bool executeLine(const String &raw, String &err) {
  String line = raw;
  line.replace('\t', ' ');
  line.trim();
  if (line.isEmpty()) return true;

  int sp = line.indexOf(' ');
  String cmd = (sp < 0) ? line : line.substring(0, sp);
  String rest = (sp < 0) ? String("") : line.substring(sp + 1);
  String cmdUpper = cmd;
  cmdUpper.toUpperCase();

  if (cmdUpper == "REM" || cmdUpper.startsWith("//") || cmd.startsWith("#")) {
    return true;
  }

  if (cmdUpper == "META") {
    return true;
  }

  if (cmdUpper == "STRING") {
    int skipped = typeString(rest);
    if (skipped > 0) {
      logLine("  ! skipped " + String(skipped) + " non-ASCII character(s)");
    }
    return true;
  }

  if (cmdUpper == "STRINGLN") {
    int skipped = typeString(rest);
    if (skipped > 0) {
      logLine("  ! skipped " + String(skipped) + " non-ASCII character(s)");
    }
    Keyboard.press(KEY_RETURN);
    vTaskDelay(pdMS_TO_TICKS(10));
    Keyboard.releaseAll();
    return true;
  }

  if (cmdUpper == "DELAY") {
    String v = rest;
    v.trim();
    if (v.isEmpty()) {
      err = "DELAY expects a duration in milliseconds";
      return false;
    }
    return duckySleep((uint32_t)v.toInt());
  }

  if (cmdUpper == "DEFAULTDELAY" || cmdUpper == "DEFAULT_DELAY") {
    g_defaultDelay = (uint32_t)rest.toInt();
    return true;
  }

  if (cmdUpper == "DEFAULTCHARDELAY" || cmdUpper == "DEFAULT_CHAR_DELAY") {
    g_charDelay = (uint32_t)rest.toInt();
    return true;
  }

  if (cmdUpper == "LAYOUT") {
    String code = rest;
    code.trim();
    code.toLowerCase();
    if (!duckySetLayout(code)) {
      err = "Unknown layout: " + code + " (expected fr or us)";
      return false;
    }
    logLine("  layout -> " + code);
    return true;
  }

  String tokens[8];
  int count = 0;
  splitTokens(line, tokens, 8, count);
  if (count == 0) return true;

  uint8_t single;
  String firstUpper = tokens[0];
  firstUpper.toUpperCase();
  if (count == 1 && lookupKey(firstUpper, single)) {
    Keyboard.press(single);
    vTaskDelay(pdMS_TO_TICKS(20));
    Keyboard.releaseAll();
    return true;
  }

  return pressCombo(tokens, count, err);
}

static bool armingCountdown(uint16_t seconds) {
  for (int left = seconds; left > 0; left--) {
    lock();
    g_state = DUCKY_ARMED;
    g_countdown = left;
    g_message = "Starting in " + String(left) + "s";
    unlock();
    if (!duckySleep(1000)) return false;
  }
  lock();
  g_countdown = 0;
  unlock();
  return !g_abort;
}

static void runScript(const String &script) {
  g_defaultDelay = DEFAULT_LINE_DELAY_MS;
  g_charDelay = DEFAULT_CHAR_DELAY_MS;

  int total = 0;
  for (size_t i = 0; i < script.length(); i++) {
    if (script[i] == '\n') total++;
  }
  if (script.length() && script[script.length() - 1] != '\n') total++;

  if (g_pendingDelay) {
    logLine("== armed, " + String(g_pendingDelay) + "s ==");
    if (!armingCountdown(g_pendingDelay)) {
      logLine("== cancelled before start ==");
      setState(DUCKY_ABORTED, "Cancelled");
      g_abort = false;
      return;
    }
  }

  setState(DUCKY_RUNNING, "Running");
  setProgress(0, total);

  String previousLine;
  int lineNo = 0;
  int pos = 0;
  bool failed = false;

  while (pos < (int)script.length()) {
    if (g_abort) break;

    int nl = script.indexOf('\n', pos);
    String raw = (nl < 0) ? script.substring(pos) : script.substring(pos, nl);
    pos = (nl < 0) ? script.length() : nl + 1;
    raw.replace("\r", "");
    lineNo++;
    setProgress(lineNo, total);

    String trimmed = raw;
    trimmed.trim();
    if (trimmed.isEmpty()) continue;

    String upper = trimmed;
    upper.toUpperCase();
    if (upper == "REPEAT" || upper.startsWith("REPEAT ")) {
      int n = trimmed.substring(6).toInt();
      if (previousLine.isEmpty() || n <= 0) {
        logLine("L" + String(lineNo) + " REPEAT ignored");
        continue;
      }
      logLine("L" + String(lineNo) + " REPEAT " + String(n));
      for (int r = 0; r < n && !g_abort; r++) {
        String err;
        if (!executeLine(previousLine, err)) {
          logLine("L" + String(lineNo) + " error: " + err);
          setState(DUCKY_ERROR, err);
          failed = true;
          break;
        }
        if (g_defaultDelay) duckySleep(g_defaultDelay);
      }
      if (failed) break;
      continue;
    }

    String err;
    if (!executeLine(trimmed, err)) {
      if (g_abort) break;
      logLine("L" + String(lineNo) + " error: " + err);
      setState(DUCKY_ERROR, err);
      failed = true;
      break;
    }

    previousLine = trimmed;
    if (g_defaultDelay && !duckySleep(g_defaultDelay)) break;
  }

  Keyboard.releaseAll();

  if (g_abort) {
    logLine("== stopped at line " + String(lineNo) + " ==");
    setState(DUCKY_ABORTED, "Stopped at line " + String(lineNo));
  } else if (!failed) {
    logLine("== finished, " + String(lineNo) + " line(s) ==");
    setState(DUCKY_DONE, "Finished (" + String(lineNo) + " lines)");
  }
  g_abort = false;
}

static void duckyTask(void *arg) {
  (void)arg;
  for (;;) {
    char *script = nullptr;
    if (xQueueReceive(g_queue, &script, portMAX_DELAY) == pdTRUE && script) {
      runScript(String(script));
      free(script);
    }
  }
}

void duckyBegin() {
  g_mutex = xSemaphoreCreateMutex();
  g_queue = xQueueCreate(1, sizeof(char *));
  Keyboard.begin(KeyboardLayout_en_US);
  xTaskCreatePinnedToCore(duckyTask, "ducky", 8192, nullptr, 2, nullptr, 1);
}

bool duckyRun(const String &script, const String &origin, uint16_t delaySeconds) {
  if (duckyIsRunning()) return false;
  if (script.length() > MAX_SCRIPT_BYTES) return false;

  char *copy = strdup(script.c_str());
  if (!copy) return false;

  g_abort = false;
  g_pendingDelay = delaySeconds;
  setState(delaySeconds ? DUCKY_ARMED : DUCKY_RUNNING, "Starting");
  logLine("== " + origin + " ==");

  if (xQueueSend(g_queue, &copy, 0) != pdTRUE) {
    free(copy);
    setState(DUCKY_ERROR, "Execution queue full");
    return false;
  }
  return true;
}

void duckyAbort() {
  if (duckyIsRunning()) g_abort = true;
}

DuckyStatus duckyGetStatus() {
  DuckyStatus s;
  lock();
  s.state = g_state;
  s.line = g_line;
  s.total = g_total;
  s.countdown = g_countdown;
  s.message = g_message;
  unlock();
  return s;
}

bool duckyIsRunning() {
  lock();
  bool running = (g_state == DUCKY_RUNNING || g_state == DUCKY_ARMED);
  unlock();
  return running;
}

bool duckySetLayout(const String &code) {
  const LayoutEntry *e = findLayout(code);
  if (!e) return false;
  Keyboard.begin(e->table);
  lock();
  g_layoutCode = e->code;
  unlock();
  return true;
}

String duckyGetArrangement() {
  lock();
  String c = g_layoutCode;
  unlock();
  const LayoutEntry *e = findLayout(c);
  return e ? String(e->arrangement) : String("QWERTY");
}

String duckyGetLayout() {
  lock();
  String l = g_layoutCode;
  unlock();
  return l;
}

void duckyLog(const String &line) { logLine(line); }

String duckyGetLogSince(uint32_t since, uint32_t &seqOut) {
  lock();
  seqOut = g_logSeq;
  uint32_t base = g_logSeq - g_log.length();
  String out;
  if (since < g_logSeq) {
    out = (since <= base) ? g_log : g_log.substring(since - base);
  }
  unlock();
  return out;
}

uint32_t duckyClearLog() {
  lock();
  g_log = "";
  uint32_t seq = g_logSeq;
  unlock();
  return seq;
}
