#pragma once
#include <Arduino.h>
#include <FS.h>
#include <vector>

struct UsbDriveStatus {
  bool cardPresent;
  bool exposed;
  uint64_t sizeMB;
};

void usbDriveBegin(bool exposed, const String &deviceName);

void usbDriveSetExposed(bool exposed);

UsbDriveStatus usbDriveGetStatus();

void usbDriveSetDeviceName(const String &name);
String usbDriveGetDeviceName();

struct SdEntry {
  String name;
  uint32_t size;
  bool isDir;
};

bool usbDriveFsAvailable();

bool usbDrivePathIsSafe(const String &path);

bool usbDriveList(const String &path, std::vector<SdEntry> &out);
File usbDriveOpen(const String &path);
bool usbDriveDelete(const String &path);
