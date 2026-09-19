#include "web_api.h"
#include "config.h"
#include "ducky.h"
#include "storage.h"
#include "web_assets.h"

#include "ui_display.h"
#include "usb_drive.h"

#include <DNSServer.h>
#include <WebServer.h>
#include <WiFi.h>

static WebServer server(80);
static DNSServer dns;
static String g_ssid;

static uint32_t g_rebootAt = 0;

static String jsonEscape(const String &in) {
  String out;
  out.reserve(in.length() + 16);
  for (size_t i = 0; i < in.length(); i++) {
    char c = in[i];
    switch (c) {
      case '"':  out += "\\\""; break;
      case '\\': out += "\\\\"; break;
      case '\n': out += "\\n";  break;
      case '\r': out += "\\r";  break;
      case '\t': out += "\\t";  break;
      default:
        if ((uint8_t)c < 0x20) {
          char buf[7];
          snprintf(buf, sizeof(buf), "\\u%04x", (uint8_t)c);
          out += buf;
        } else {
          out += c;
        }
    }
  }
  return out;
}

static const char *stateName(DuckyState s) {
  switch (s) {
    case DUCKY_ARMED:   return "armed";
    case DUCKY_RUNNING: return "running";
    case DUCKY_DONE:    return "done";
    case DUCKY_ABORTED: return "aborted";
    case DUCKY_ERROR:   return "error";
    default:            return "idle";
  }
}

static void sendJson(int code, const String &body) {
  server.send(code, "application/json; charset=utf-8", body);
}

static void sendError(int code, const String &msg) {
  sendJson(code, "{\"error\":\"" + jsonEscape(msg) + "\"}");
}

static void handleIndex() {
  server.sendHeader("Cache-Control", "no-store");
  server.send_P(200, "text/html; charset=utf-8", PAGE_INDEX);
}

static void handleState() {
  DuckyStatus st = duckyGetStatus();

  String json = "{";
  json += "\"ssid\":\"" + jsonEscape(g_ssid) + "\",";
  json += "\"version\":\"" FIRMWARE_VERSION "\",";
  json += "\"ip\":\"" + WiFi.softAPIP().toString() + "\",";
  json += "\"clients\":" + String(WiFi.softAPgetStationNum()) + ",";
  json += "\"layout\":\"" + duckyGetLayout() + "\",";
  json += "\"arrangement\":\"" + duckyGetArrangement() + "\",";
  json += "\"state\":\"" + String(stateName(st.state)) + "\",";
  json += "\"line\":" + String(st.line) + ",";
  json += "\"total\":" + String(st.total) + ",";
  json += "\"countdown\":" + String(st.countdown) + ",";
  json += "\"message\":\"" + jsonEscape(st.message) + "\",";

  json += "\"payloads\":[";
  const std::vector<PayloadInfo> &items = storageListDetailed();
  for (size_t i = 0; i < items.size(); i++) {
    if (i) json += ",";
    json += "{\"name\":\"" + jsonEscape(items[i].name) + "\",";
    json += "\"os\":\"" + jsonEscape(items[i].os) + "\"}";
  }
  json += "]}";

  sendJson(200, json);
}

static void handlePayloadGet() {
  String name = server.arg("name");
  if (!storageExists(name)) {
    sendError(404, "Payload not found");
    return;
  }
  server.send(200, "text/plain; charset=utf-8", storageRead(name));
}

static void handlePayloadSave() {
  String name = server.arg("name");
  String content = server.arg("content");

  if (!storageNameIsValid(name)) {
    sendError(400, "Invalid name: letters, digits, dot, dash and underscore only");
    return;
  }
  if (content.length() > MAX_SCRIPT_BYTES) {
    sendError(413, "Payload too large");
    return;
  }
  if (!storageWrite(name, content)) {
    sendError(500, "Could not write to flash");
    return;
  }
  sendJson(200, "{\"ok\":true}");
}

static void handlePayloadDelete() {
  String name = server.arg("name");
  if (!storageDelete(name)) {
    sendError(404, "Could not delete");
    return;
  }
  sendJson(200, "{\"ok\":true}");
}

static void handleRun() {
  String script = server.arg("script");
  String name = server.arg("name");
  String origin = "editor";

  if (script.isEmpty() && !name.isEmpty()) {
    if (!storageExists(name)) {
      sendError(404, "Payload not found");
      return;
    }
    script = storageRead(name);
    origin = name;
  }

  if (script.isEmpty()) {
    sendError(400, "Empty script");
    return;
  }

  Settings st = storageLoadSettings();
  uint16_t delay = st.startDelay;
  if (server.hasArg("delay")) {
    long d = server.arg("delay").toInt();
    if (d < 0 || d > START_DELAY_MAX) {
      sendError(400, "Delay must be between 0 and " + String(START_DELAY_MAX) + " seconds");
      return;
    }
    delay = (uint16_t)d;
    if (delay != st.startDelay) {
      st.startDelay = delay;
      st.layout = duckyGetLayout();
      storageSaveSettings(st);
    }
  }

  if (!duckyRun(script, origin, delay)) {
    sendError(409, "A payload is already running");
    return;
  }
  sendJson(202, "{\"ok\":true}");
}

static void handleStop() {
  duckyAbort();
  sendJson(200, "{\"ok\":true}");
}

static void handleLayout() {
  String layout = server.arg("layout");
  if (!duckySetLayout(layout)) {
    sendError(400, "Unknown layout code");
    return;
  }

  Settings st = storageLoadSettings();
  st.layout = duckyGetLayout();
  storageSaveSettings(st);
  sendJson(200, "{\"ok\":true}");
}

static void handleLog() {
  uint32_t since = server.hasArg("since") ? (uint32_t)strtoul(server.arg("since").c_str(), nullptr, 10) : 0;
  uint32_t seq = 0;
  String text = duckyGetLogSince(since, seq);
  sendJson(200, "{\"seq\":" + String(seq) + ",\"text\":\"" + jsonEscape(text) + "\"}");
}

static void handleLogClear() {
  uint32_t seq = duckyClearLog();
  sendJson(200, "{\"ok\":true,\"seq\":" + String(seq) + "}");
}

static void handleSettingsGet() {
  Settings s = storageLoadSettings();

  String json = "{";
  json += "\"layout\":\"" + jsonEscape(duckyGetLayout()) + "\",";
  json += "\"ssid\":\"" + jsonEscape(g_ssid) + "\",";
  json += "\"password\":\"" + jsonEscape(s.password.isEmpty() ? storageDefaultPassword() : s.password) + "\",";
  json += "\"ssidMax\":" + String(SSID_MAX_LEN) + ",";
  json += "\"passwordMin\":" + String(PASSWORD_MIN_LEN) + ",";
  json += "\"passwordMax\":" + String(PASSWORD_MAX_LEN) + ",";
  json += "\"rotation\":" + String(displayGetRotation()) + ",";
  json += "\"screen\":" + String(s.screenOn ? 1 : 0) + ",";
  json += "\"led\":" + String(s.ledOn ? 1 : 0) + ",";
  char color[8];
  snprintf(color, sizeof(color), "#%02X%02X%02X", s.ledR, s.ledG, s.ledB);
  json += "\"ledColor\":\"" + String(color) + "\",";
  json += "\"startDelay\":" + String(s.startDelay) + ",";
  json += "\"startDelayMax\":" + String(START_DELAY_MAX) + ",";
  json += "\"deviceName\":\"" + jsonEscape(s.deviceName.isEmpty()
              ? String(DEVICE_NAME_DEFAULT) : s.deviceName) + "\",";
  json += "\"deviceNameMax\":" + String(DEVICE_NAME_MAX) + ",";
  json += "\"showAccess\":" + String(s.showAccess ? 1 : 0) + ",";
  UsbDriveStatus drv = usbDriveGetStatus();
  json += "\"usbDrive\":" + String(drv.exposed ? 1 : 0) + ",";
  json += "\"usbCard\":" + String(drv.cardPresent ? 1 : 0) + ",";
  json += "\"usbSizeMB\":" + String((uint32_t)drv.sizeMB) + ",";
  json += "\"layouts\":[";
  for (size_t i = 0; i < duckyLayoutCount(); i++) {
    LayoutInfo li = duckyLayoutAt(i);
    if (i) json += ",";
    json += "{\"code\":\"" + String(li.code) + "\",";
    json += "\"name\":\"" + String(li.name) + "\",";
    json += "\"arrangement\":\"" + String(li.arrangement) + "\"}";
  }
  json += "]";
  json += "}";
  sendJson(200, json);
}

static void handleWifiSave() {
  String ssid = server.arg("ssid");
  String password = server.arg("password");

  if (!storageSsidIsValid(ssid)) {
    sendError(400, "Network name must be 1 to " + String(SSID_MAX_LEN) + " characters");
    return;
  }
  if (!storagePasswordIsValid(password)) {
    sendError(400, "Password must be " + String(PASSWORD_MIN_LEN) + " to " +
                     String(PASSWORD_MAX_LEN) + " characters");
    return;
  }

  Settings s = storageLoadSettings();
  s.layout = duckyGetLayout();
  s.ssid = ssid;
  s.password = password;

  if (!storageSaveSettings(s)) {
    sendError(500, "Could not write to flash");
    return;
  }

  sendJson(200, "{\"ok\":true,\"reboot\":true}");

  displayShowMessage("WI-FI UPDATED", "restarting");
  g_rebootAt = millis() + 1500;
}

static void handleDisplaySave() {
  Settings st = storageLoadSettings();
  st.layout = duckyGetLayout();

  if (server.hasArg("rotation")) {
    int r = server.arg("rotation").toInt();
    if (r < 0 || r > 3) {
      sendError(400, "Rotation must be 0, 1, 2 or 3");
      return;
    }
    st.rotation = (uint8_t)r;
  }
  if (server.hasArg("screen")) st.screenOn = (server.arg("screen").toInt() != 0);
  if (server.hasArg("led")) st.ledOn = (server.arg("led").toInt() != 0);
  if (server.hasArg("showAccess")) st.showAccess = (server.arg("showAccess").toInt() != 0);

  if (server.hasArg("ledColor")) {
    String c = server.arg("ledColor");
    c.trim();
    if (c.startsWith("#")) c = c.substring(1);
    if (c.length() != 6) {
      sendError(400, "Colour must be six hex digits");
      return;
    }
    char *end = nullptr;
    long v = strtol(c.c_str(), &end, 16);
    if (end == nullptr || *end != '\0') {
      sendError(400, "Colour must be six hex digits");
      return;
    }
    st.ledR = (v >> 16) & 0xFF;
    st.ledG = (v >> 8) & 0xFF;
    st.ledB = v & 0xFF;
  }

  if (!storageSaveSettings(st)) {
    sendError(500, "Could not write to flash");
    return;
  }

  displaySetRotation(st.rotation);
  displaySetScreenOn(st.screenOn);
  displaySetLed(st.ledOn, st.ledR, st.ledG, st.ledB);

  sendJson(200, "{\"ok\":true}");
}

static void handleDriveSave() {
  if (!server.hasArg("exposed")) {
    sendError(400, "Missing exposed flag");
    return;
  }
  bool exposed = (server.arg("exposed").toInt() != 0);

  Settings st = storageLoadSettings();
  st.layout = duckyGetLayout();
  st.usbDrive = exposed;
  if (!storageSaveSettings(st)) {
    sendError(500, "Could not write to flash");
    return;
  }

  usbDriveSetExposed(exposed);

  UsbDriveStatus drv = usbDriveGetStatus();
  if (exposed && !drv.cardPresent) {
    sendError(409, "No card in the slot");
    return;
  }
  sendJson(200, "{\"ok\":true}");
}

static bool requireCardOwnership() {
  if (usbDriveFsAvailable()) return true;
  UsbDriveStatus d = usbDriveGetStatus();
  if (!d.cardPresent) {
    sendError(404, "No card in the slot");
  } else {
    sendError(409, "The card is attached to the host. Detach it to browse from here.");
  }
  return false;
}

static void handleSdList() {
  if (!requireCardOwnership()) return;

  String path = server.hasArg("path") ? server.arg("path") : String("/");
  if (path.isEmpty()) path = "/";

  std::vector<SdEntry> entries;
  if (!usbDriveList(path, entries)) {
    sendError(400, "Cannot open " + path);
    return;
  }

  String json = "{\"path\":\"" + jsonEscape(path) + "\",\"entries\":[";
  for (size_t i = 0; i < entries.size(); i++) {
    if (i) json += ",";
    json += "{\"name\":\"" + jsonEscape(entries[i].name) + "\",";
    json += "\"size\":" + String(entries[i].size) + ",";
    json += "\"dir\":" + String(entries[i].isDir ? 1 : 0) + "}";
  }
  json += "]}";
  sendJson(200, json);
}

static void handleSdDownload() {
  if (!requireCardOwnership()) return;

  String path = server.arg("path");
  File f = usbDriveOpen(path);
  if (!f || f.isDirectory()) {
    sendError(404, "File not found");
    return;
  }

  String name = path.substring(path.lastIndexOf('/') + 1);
  server.sendHeader("Content-Disposition", "attachment; filename=\"" + name + "\"");
  server.streamFile(f, "application/octet-stream");
  f.close();
}

static void handleSdDelete() {
  if (!requireCardOwnership()) return;

  if (!usbDriveDelete(server.arg("path"))) {
    sendError(400, "Could not delete");
    return;
  }
  sendJson(200, "{\"ok\":true}");
}

static void handleNameSave() {
  String name = server.arg("name");
  if (!storageDeviceNameIsValid(name)) {
    sendError(400, "Name must be 1 to " + String(DEVICE_NAME_MAX) +
                     " printable characters");
    return;
  }

  Settings st = storageLoadSettings();
  st.layout = duckyGetLayout();
  st.deviceName = name;
  if (!storageSaveSettings(st)) {
    sendError(500, "Could not write to flash");
    return;
  }

  usbDriveSetDeviceName(name);
  sendJson(200, "{\"ok\":true}");
}

static void handleFactoryReset() {
  storageResetSettings();
  sendJson(200, "{\"ok\":true,\"reboot\":true}");
  displayShowMessage("FACTORY RESET", "restarting");
  g_rebootAt = millis() + 1500;
}

static void handleNotFound() {
  server.sendHeader("Location", "http://" + WiFi.softAPIP().toString() + "/", true);
  server.send(302, "text/plain", "");
}

void webBegin(const String &ssid) {
  g_ssid = ssid;

  dns.setErrorReplyCode(DNSReplyCode::NoError);
  dns.start(53, "*", WiFi.softAPIP());

  server.on("/", HTTP_GET, handleIndex);
  server.on("/api/state", HTTP_GET, handleState);
  server.on("/api/payload", HTTP_GET, handlePayloadGet);
  server.on("/api/payload", HTTP_POST, handlePayloadSave);
  server.on("/api/payload/delete", HTTP_POST, handlePayloadDelete);
  server.on("/api/run", HTTP_POST, handleRun);
  server.on("/api/stop", HTTP_POST, handleStop);
  server.on("/api/layout", HTTP_POST, handleLayout);
  server.on("/api/log", HTTP_GET, handleLog);
  server.on("/api/log/clear", HTTP_POST, handleLogClear);
  server.on("/api/settings", HTTP_GET, handleSettingsGet);
  server.on("/api/settings/wifi", HTTP_POST, handleWifiSave);
  server.on("/api/settings/display", HTTP_POST, handleDisplaySave);
  server.on("/api/settings/drive", HTTP_POST, handleDriveSave);
  server.on("/api/settings/name", HTTP_POST, handleNameSave);
  server.on("/api/sd/list", HTTP_GET, handleSdList);
  server.on("/api/sd/download", HTTP_GET, handleSdDownload);
  server.on("/api/sd/delete", HTTP_POST, handleSdDelete);
  server.on("/api/settings/reset", HTTP_POST, handleFactoryReset);
  server.onNotFound(handleNotFound);

  server.begin();
}

void webLoop() {
  dns.processNextRequest();
  server.handleClient();

  if (g_rebootAt && millis() >= g_rebootAt) {
    ESP.restart();
  }
}
