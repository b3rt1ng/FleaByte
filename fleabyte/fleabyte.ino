
#include "config.h"
#include "ducky.h"
#include "storage.h"
#include "ui_display.h"
#include "usb_drive.h"
#include "web_api.h"

#include <ESPmDNS.h>
#include <WiFi.h>
#include <esp_mac.h>

#if ARDUINO_USB_MODE == 1
#error "Select USB Mode = USB-OTG (TinyUSB): hardware CDC mode cannot do HID."
#endif

static String g_ssid;
static String g_password;
static uint32_t g_nextRefresh = 0;
static uint32_t g_joinUntil = 0;
static bool g_lastButton = HIGH;

static bool g_joinLatched = true;
static uint32_t g_buttonDownAt = 0;
static bool g_resetArmed = false;

// From eFuse: WiFi.softAPmacAddress() returns zeros before softAP() runs.
static String defaultSsid() {
  uint8_t mac[6] = {0};
  esp_read_mac(mac, ESP_MAC_WIFI_SOFTAP);
  char suffix[8];
  snprintf(suffix, sizeof(suffix), "%02X%02X", mac[4], mac[5]);
  return String(AP_SSID_PREFIX) + "-" + suffix;
}

void setup() {
  Serial.begin(115200);

  pinMode(BOOT_PIN, INPUT_PULLUP);
  displayBegin();

  if (!storageBegin()) {
    Serial.println("LittleFS unavailable: the payload library will be empty.");
  } else if (storageWasFormatted()) {
    Serial.println("LittleFS could not be mounted and was reformatted: "
                   "payloads and settings have been lost.");
  }

  Settings settings = storageLoadSettings();
  g_ssid = settings.ssid.isEmpty() ? defaultSsid() : settings.ssid;
  g_password = settings.password.isEmpty() ? storageDefaultPassword() : settings.password;

  WiFi.mode(WIFI_AP);
  WiFi.softAP(g_ssid.c_str(), g_password.c_str(), AP_CHANNEL, 0, AP_MAX_CLIENTS);

  Serial.printf("AP %s -> http://%s\n", g_ssid.c_str(), WiFi.softAPIP().toString().c_str());

  if (MDNS.begin(MDNS_HOST)) {
    MDNS.addService("http", "tcp", 80);
  }

  displaySetRotation(settings.rotation);
  displaySetScreenOn(settings.screenOn);
  displaySetLed(settings.ledOn, settings.ledR, settings.ledG, settings.ledB);

  usbDriveBegin(settings.usbDrive, settings.deviceName);

  // USB names are set at compile time in tools/fqbn.sh: the core already
  // called USB.begin() in app_main(), so USB.productName() here does nothing.
  duckyBegin();
  duckySetLayout(settings.layout);

  if (storageWasFormatted()) {
    duckyLog("== filesystem was reformatted: payloads and settings lost ==");
  }

  webBegin(g_ssid);

  // Left up until a device joins, unless the operator would rather not
  // leave the password and a scannable code on show. The button still
  // reveals them, which needs the dongle in hand.
  g_joinLatched = settings.showAccess;
  if (g_joinLatched) displayShowJoin(g_ssid, g_password);
}

static void handleButton() {
  bool button = digitalRead(BOOT_PIN);
  uint32_t now = millis();

  if (g_lastButton == HIGH && button == LOW) {
    g_buttonDownAt = now;
    g_resetArmed = false;
  }

  if (button == LOW && !g_resetArmed && (now - g_buttonDownAt) >= FACTORY_RESET_HOLD_MS) {

    g_resetArmed = true;
    displayShowMessage("FACTORY RESET", "restoring defaults");
    storageResetSettings();
    delay(1200);
    ESP.restart();
  }

  if (g_lastButton == LOW && button == HIGH) {
    if (!g_resetArmed) {

      displayWake();
      displayShowJoin(g_ssid, g_password);
      g_joinUntil = now + 8000;
    }
    g_resetArmed = false;
  }

  g_lastButton = button;
}

void loop() {
  webLoop();
  handleButton();
  displayTick();

  uint32_t now = millis();
  if (now >= g_nextRefresh) {
    g_nextRefresh = now + 400;

    DisplayInfo info;
    info.ssid = g_ssid;
    info.ip = WiFi.softAPIP().toString();
    info.arrangement = duckyGetArrangement();
    info.name = usbDriveGetDeviceName();
    UsbDriveStatus drv = usbDriveGetStatus();
    info.sdPresent = drv.cardPresent;
    info.sdExposed = drv.exposed;
    info.clients = WiFi.softAPgetStationNum();
    info.ducky = duckyGetStatus();

    if (info.clients > 0 || duckyIsRunning()) g_joinLatched = false;

    if (!g_joinLatched && now >= g_joinUntil) displayUpdate(info);
  }

  delay(2);
}
