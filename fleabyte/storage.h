#pragma once
#include <Arduino.h>
#include <vector>

bool storageBegin();

bool storageWasFormatted();

std::vector<String> storageList();

struct PayloadInfo {
  String name;
  String os;
};

const std::vector<PayloadInfo> &storageListDetailed();
bool storageExists(const String &name);
String storageRead(const String &name);
bool storageWrite(const String &name, const String &content);
bool storageDelete(const String &name);

bool storageNameIsValid(const String &name);

struct Settings {
  String layout;
  String ssid;
  String password;

  uint8_t rotation;
  bool screenOn;
  bool ledOn;

  uint16_t startDelay;
  uint16_t seedVersion;
  bool usbDrive;
  String deviceName;
  bool showAccess;
};

Settings storageLoadSettings();
bool storageSaveSettings(const Settings &s);

// The script armed for the next boot, snapshotted from the editor. Writing
// an empty one disarms. The size is cached, since /api/state reports the
// armed state every second and must not stat the file each time.
bool storageArmedWrite(const String &script);
bool storageArmedClear();
String storageArmedRead();
size_t storageArmedSize();

void storageResetSettings();

String storageDefaultPassword();

bool storageDeviceNameIsValid(const String &name);
bool storageSsidIsValid(const String &ssid);
bool storagePasswordIsValid(const String &password);
