#include "storage.h"
#include "config.h"
#include "ducky.h"
#include <esp_mac.h>

#include <FS.h>
#include <LittleFS.h>
#include <algorithm>

static const char *DEMO_LAYOUT_TEST =
  "REM Checks that the selected layout matches the machine's keyboard.\n"
  "REM Open a text editor BEFORE running this payload.\n"
  "REM These are the characters that move between AZERTY and QWERTY.\n"
  "DELAY 500\n"
  "STRINGLN --- Fleabyte layout test ---\n"
  "STRINGLN azertyuiop qwertyuiop\n"
  "STRINGLN AZERTYUIOP QWERTYUIOP\n"
  "STRINGLN 0123456789\n"
  "STRINGLN @ # $ % & * ( ) - _ = +\n"
  "STRINGLN / \\ | [ ] { } < > ^ ~\n"
  "STRINGLN ! ? : ; , . ' \"\n"
  "STRINGLN If this line reads correctly, the layout is right.\n";

static const char *DEMO_REFERENCE =
  "REM Every command the interpreter understands.\n"
  "REM Running this types the reference out, so open a text editor first.\n"
  "DELAY 500\n"
  "STRINGLN == Fleabyte command reference ==\n"
  "STRINGLN\n"
  "STRINGLN REM text              a comment, does nothing\n"
  "STRINGLN META windows|linux|macos   tags this payload, never typed\n"
  "STRINGLN STRING text           types the text\n"
  "STRINGLN STRINGLN text         types the text, then Enter\n"
  "STRINGLN DELAY 500             waits 500 ms\n"
  "STRINGLN WAIT_FOR_HOST 5000    waits for the host to enumerate\n"
  "STRINGLN DEFAULTDELAY 50       pause after every line\n"
  "STRINGLN DEFAULTCHARDELAY 20   pause between characters\n"
  "STRINGLN LAYOUT <code>          switches layout mid-script\n"
  "STRINGLN   us fr de ch hu es it pt br se dk jp\n"
  "STRINGLN REPEAT 3              replays the previous line 3 times\n"
  "STRINGLN\n"
  "STRINGLN Named keys:\n"
  "STRINGLN   ENTER TAB ESC SPACE BACKSPACE DELETE INSERT\n"
  "STRINGLN   HOME END PAGEUP PAGEDOWN UP DOWN LEFT RIGHT\n"
  "STRINGLN   MENU CAPSLOCK PRINTSCREEN PAUSE F1 to F12\n"
  "STRINGLN\n"
  "STRINGLN Modifiers: CTRL SHIFT ALT GUI ALTGR\n"
  "STRINGLN   GUI r / CTRL ALT DELETE / CTRL SHIFT ESC\n"
  "STRINGLN\n"
  "STRINGLN Only ASCII is typed. Accented characters are skipped\n"
  "STRINGLN and reported in the run log.\n";

static const char *DEMO_NOTEPAD =
  "META windows\n"
  "REM Windows: opens Notepad and writes a line of proof.\n"
  "DELAY 300\n"
  "GUI r\n"
  "DELAY 600\n"
  "STRING notepad\n"
  "ENTER\n"
  "DELAY 1200\n"
  "STRINGLN Payload executed from Fleabyte.\n"
  "STRING Test machine only.\n";

static const char *DEMO_URL =
  "META windows\n"
  "REM Windows: opens a page in the default browser via the Run dialog.\n"
  "REM Also a quick layout check: a wrong layout mangles the slashes\n"
  "REM and dots, and the address fails to resolve.\n"
  "DELAY 300\n"
  "GUI r\n"
  "DELAY 600\n"
  "STRINGLN https://example.com\n";

static const char *DEMO_LINUX =
  "META linux\n"
  "REM Linux/GNOME: opens a terminal and prints a message.\n"
  "REM CTRL ALT T is the GNOME shortcut; adjust for your desktop.\n"
  "DELAY 300\n"
  "CTRL ALT t\n"
  "DELAY 1500\n"
  "STRINGLN echo \"Fleabyte - HID test $(date)\"\n";

static const char *DEMO_MACOS =
  "META macos\n"
  "REM macOS: opens TextEdit through Spotlight and types a line.\n"
  "DELAY 300\n"
  "GUI SPACE\n"
  "DELAY 800\n"
  "STRING TextEdit\n"
  "DELAY 700\n"
  "ENTER\n"
  "DELAY 2000\n"
  "STRINGLN Payload executed from Fleabyte.\n";

static const char *DEMO_TIMING =
  "REM Shows the timing commands. Open a text editor first.\n"
  "DELAY 500\n"
  "STRINGLN -- default speed --\n"
  "STRINGLN The quick brown fox jumps over the lazy dog.\n"
  "DEFAULTCHARDELAY 60\n"
  "STRINGLN -- slowed to 60 ms per character --\n"
  "STRINGLN The quick brown fox jumps over the lazy dog.\n"
  "DEFAULTCHARDELAY 0\n"
  "STRINGLN -- as fast as the host accepts --\n"
  "STRINGLN The quick brown fox jumps over the lazy dog.\n"
  "STRINGLN -- REPEAT replays the previous line --\n"
  "STRING .\n"
  "REPEAT 30\n"
  "ENTER\n"
  "STRINGLN done\n";

static const char *DEMO_IME =
  "REM Targets whose input mode is not Latin: Russian, Chinese, Korean,\n"
  "REM Japanese kana. A layout table cannot help there, because with a\n"
  "REM non-Latin input mode active no key produces an ASCII letter at all.\n"
  "REM The fix is to switch the host back to Latin input first.\n"
  "REM\n"
  "REM GUI SPACE  cycles input languages on Windows 10/11, GNOME, macOS\n"
  "REM ALT SHIFT  does the same on older Windows setups\n"
  "REM SHIFT      toggles Chinese/English on most Pinyin IMEs\n"
  "DELAY 300\n"
  "GUI SPACE\n"
  "DELAY 600\n"
  "LAYOUT us\n"
  "STRINGLN Latin input restored.\n";

static const char *DEMO_POWERSHELL =
  "META windows\n"
  "REM Windows: opens PowerShell and prints a line.\n"
  "REM The classic smoke test: if this shows up, HID, timing and layout\n"
  "REM are all working.\n"
  "DELAY 300\n"
  "GUI r\n"
  "DELAY 600\n"
  "STRING powershell\n"
  "ENTER\n"
  "REM PowerShell takes longer to appear than a text editor.\n"
  "DELAY 2000\n"
  "STRINGLN echo ok\n";

struct DemoPayload {
  const char *name;
  const char *body;
};

static const DemoPayload DEMOS[] = {
  {"00-test-layout.txt", DEMO_LAYOUT_TEST},
  {"01-command-reference.txt", DEMO_REFERENCE},
  {"10-windows-notepad.txt", DEMO_NOTEPAD},
  {"11-windows-open-url.txt", DEMO_URL},
  {"12-windows-powershell.txt", DEMO_POWERSHELL},
  {"20-linux-terminal.txt", DEMO_LINUX},
  {"30-macos-textedit.txt", DEMO_MACOS},
  {"40-timing-demo.txt", DEMO_TIMING},
  {"50-switch-input-language.txt", DEMO_IME},
};

bool storageNameIsValid(const String &name) {
  if (name.isEmpty() || name.length() > MAX_NAME_LEN) return false;
  for (size_t i = 0; i < name.length(); i++) {
    char c = name[i];
    bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') || c == '.' || c == '_' || c == '-';
    if (!ok) return false;
  }

  if (name == "." || name == "..") return false;
  return true;
}

static String pathFor(const String &name) {
  return String(PAYLOAD_DIR) + "/" + name;
}

static bool s_formatted = false;
static std::vector<PayloadInfo> s_metaCache;
static bool s_metaCacheValid = false;

static void invalidateMetaCache() { s_metaCacheValid = false; }

bool storageWasFormatted() { return s_formatted; }

bool storageBegin() {

  if (!LittleFS.begin(false)) {
    if (!LittleFS.begin(true)) return false;
    s_formatted = true;
  }

  if (!LittleFS.exists(PAYLOAD_DIR)) {
    LittleFS.mkdir(PAYLOAD_DIR);
  }

  Settings s = storageLoadSettings();
  if (s.seedVersion < PAYLOAD_SEED_VERSION) {
    for (const DemoPayload &d : DEMOS) {
      storageWrite(d.name, d.body);
    }
    s.seedVersion = PAYLOAD_SEED_VERSION;
    storageSaveSettings(s);
  }
  return true;
}

std::vector<String> storageList() {
  std::vector<String> out;
  File dir = LittleFS.open(PAYLOAD_DIR);
  if (!dir || !dir.isDirectory()) return out;

  File f = dir.openNextFile();
  while (f) {
    if (!f.isDirectory()) {
      String n = String(f.name());
      int slash = n.lastIndexOf('/');
      if (slash >= 0) n = n.substring(slash + 1);
      out.push_back(n);
    }
    f = dir.openNextFile();
  }
  std::sort(out.begin(), out.end(), [](const String &a, const String &b) {
    return strcmp(a.c_str(), b.c_str()) < 0;
  });
  return out;
}

bool storageExists(const String &name) {
  if (!storageNameIsValid(name)) return false;
  return LittleFS.exists(pathFor(name));
}

struct OsAlias {
  const char *word;
  const char *os;
};

static const OsAlias OS_ALIASES[] = {
  {"win", "windows"},   {"windows", "windows"}, {"win10", "windows"},
  {"win11", "windows"}, {"msw", "windows"},
  {"lin", "linux"},     {"linux", "linux"},     {"gnu", "linux"},
  {"ubuntu", "linux"},  {"debian", "linux"},    {"gnome", "linux"},
  {"kde", "linux"},     {"kali", "linux"},
  {"mac", "macos"},     {"macos", "macos"},     {"osx", "macos"},
  {"darwin", "macos"},  {"apple", "macos"},
};

static String osTagFor(const String &name) {
  File f = LittleFS.open(pathFor(name), "r");
  if (!f) return String();

  String head;
  size_t budget = 512;
  while (f.available() && budget--) head += (char)f.read();
  f.close();

  int pos = 0;
  while (pos < (int)head.length()) {
    int nl = head.indexOf('\n', pos);
    String line = (nl < 0) ? head.substring(pos) : head.substring(pos, nl);
    pos = (nl < 0) ? head.length() : nl + 1;
    line.replace("\r", "");
    line.trim();
    if (line.isEmpty()) continue;

    String upper = line;
    upper.toUpperCase();
    if (!upper.startsWith("META")) continue;

    String tags = line.substring(4);
    tags.toLowerCase();
    tags.replace(",", " ");
    int tp = 0;
    while (tp < (int)tags.length()) {
      while (tp < (int)tags.length() && tags[tp] == ' ') tp++;
      int start = tp;
      while (tp < (int)tags.length() && tags[tp] != ' ') tp++;
      if (tp == start) break;
      String word = tags.substring(start, tp);
      for (const OsAlias &a : OS_ALIASES) {
        if (word == a.word) return String(a.os);
      }
    }
  }
  return String();
}

const std::vector<PayloadInfo> &storageListDetailed() {
  if (s_metaCacheValid) return s_metaCache;
  s_metaCache.clear();
  for (const String &n : storageList()) {
    s_metaCache.push_back({n, osTagFor(n)});
  }
  s_metaCacheValid = true;
  return s_metaCache;
}

String storageRead(const String &name) {
  if (!storageNameIsValid(name)) return String();
  File f = LittleFS.open(pathFor(name), "r");
  if (!f) return String();
  String content = f.readString();
  f.close();
  return content;
}

bool storageWrite(const String &name, const String &content) {
  invalidateMetaCache();
  if (!storageNameIsValid(name)) return false;
  if (content.length() > MAX_SCRIPT_BYTES) return false;
  File f = LittleFS.open(pathFor(name), "w");
  if (!f) return false;
  size_t written = f.print(content);
  f.close();
  return written == content.length();
}

bool storageDelete(const String &name) {
  invalidateMetaCache();
  if (!storageNameIsValid(name)) return false;
  return LittleFS.remove(pathFor(name));
}

String storageDefaultPassword() {
  uint8_t mac[6] = {0};
  esp_read_mac(mac, ESP_MAC_WIFI_SOFTAP);
  char suffix[9];
  snprintf(suffix, sizeof(suffix), "%02X%02X%02X%02X", mac[2], mac[3], mac[4], mac[5]);
  return String(AP_PASSWORD_PREFIX) + suffix;
}

bool storageDeviceNameIsValid(const String &name) {
  if (name.isEmpty() || name.length() > DEVICE_NAME_MAX) return false;

  for (size_t i = 0; i < name.length(); i++) {
    uint8_t c = (uint8_t)name[i];
    if (c < 0x20 || c > 0x7E) return false;
  }
  return true;
}

bool storageSsidIsValid(const String &ssid) {
  if (ssid.length() < SSID_MIN_LEN || ssid.length() > SSID_MAX_LEN) return false;

  for (size_t i = 0; i < ssid.length(); i++) {
    if ((uint8_t)ssid[i] < 0x20 || (uint8_t)ssid[i] == 0x7F) return false;
  }
  return true;
}

bool storagePasswordIsValid(const String &password) {
  if (password.length() < PASSWORD_MIN_LEN || password.length() > PASSWORD_MAX_LEN) return false;

  for (size_t i = 0; i < password.length(); i++) {
    uint8_t c = (uint8_t)password[i];
    if (c < 0x20 || c > 0x7E) return false;
  }
  return true;
}

Settings storageLoadSettings() {
  Settings s;
  s.layout = "us";
  s.rotation = TFT_ROTATION;
  s.screenOn = true;
  s.ledOn = true;
  s.ledR = LED_DEFAULT_R;
  s.ledG = LED_DEFAULT_G;
  s.ledB = LED_DEFAULT_B;
  s.startDelay = 0;
  s.seedVersion = 0;
  s.usbDrive = false;
  s.showAccess = true;

  File f = LittleFS.open(SETTINGS_FILE, "r");
  if (!f) return s;

  String content = f.readString();
  f.close();

  if (content.indexOf('=') < 0) {

    String v = content;
    v.trim();
    if (v == "us" || v == "fr") s.layout = v;
    return s;
  }

  int pos = 0;
  while (pos < (int)content.length()) {
    int nl = content.indexOf('\n', pos);
    String line = (nl < 0) ? content.substring(pos) : content.substring(pos, nl);
    pos = (nl < 0) ? content.length() : nl + 1;
    line.replace("\r", "");

    int eq = line.indexOf('=');
    if (eq <= 0) continue;
    String key = line.substring(0, eq);
    String value = line.substring(eq + 1);
    key.trim();

    if (key == "layout") {
      value.trim();
      if (duckyLayoutExists(value)) s.layout = value;
    } else if (key == "ssid") {

      if (storageSsidIsValid(value)) s.ssid = value;
    } else if (key == "password") {
      if (storagePasswordIsValid(value)) s.password = value;
    } else if (key == "rotation") {
      int r = value.toInt();
      if (r >= 0 && r <= 3) s.rotation = (uint8_t)r;
    } else if (key == "screen") {
      s.screenOn = (value.toInt() != 0);
    } else if (key == "led") {
      s.ledOn = (value.toInt() != 0);
    } else if (key == "devicename") {

      if (storageDeviceNameIsValid(value)) s.deviceName = value;
    } else if (key == "showaccess") {
      s.showAccess = (value.toInt() != 0);
    } else if (key == "usbdrive") {
      s.usbDrive = (value.toInt() != 0);
    } else if (key == "seedversion") {
      long v = value.toInt();
      if (v >= 0 && v <= 65535) s.seedVersion = (uint16_t)v;
    } else if (key == "startdelay") {
      long d = value.toInt();
      if (d >= 0 && d <= START_DELAY_MAX) s.startDelay = (uint16_t)d;
    } else if (key == "ledcolor") {

      value.trim();
      if (value.length() == 6) {
        long v = strtol(value.c_str(), nullptr, 16);
        s.ledR = (v >> 16) & 0xFF;
        s.ledG = (v >> 8) & 0xFF;
        s.ledB = v & 0xFF;
      }
    }
  }
  return s;
}

bool storageSaveSettings(const Settings &s) {
  if (!s.ssid.isEmpty() && !storageSsidIsValid(s.ssid)) return false;
  if (!s.password.isEmpty() && !storagePasswordIsValid(s.password)) return false;

  File f = LittleFS.open(SETTINGS_FILE, "w");
  if (!f) return false;
  f.print("layout=");
  f.println(s.layout);
  if (!s.ssid.isEmpty()) {
    f.print("ssid=");
    f.println(s.ssid);
  }
  if (!s.password.isEmpty()) {
    f.print("password=");
    f.println(s.password);
  }
  f.print("rotation=");
  f.println(s.rotation);
  f.print("screen=");
  f.println(s.screenOn ? 1 : 0);
  f.print("led=");
  f.println(s.ledOn ? 1 : 0);
  f.print("startdelay=");
  f.println(s.startDelay);
  f.print("seedversion=");
  f.println(s.seedVersion);
  f.print("usbdrive=");
  f.println(s.usbDrive ? 1 : 0);
  f.print("showaccess=");
  f.println(s.showAccess ? 1 : 0);
  if (!s.deviceName.isEmpty()) {
    f.print("devicename=");
    f.println(s.deviceName);
  }
  char color[8];
  snprintf(color, sizeof(color), "%02X%02X%02X", s.ledR, s.ledG, s.ledB);
  f.print("ledcolor=");
  f.println(color);
  f.close();
  return true;
}

void storageResetSettings() {
  LittleFS.remove(SETTINGS_FILE);
}
