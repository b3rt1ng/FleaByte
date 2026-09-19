#include "usb_drive.h"
#include "config.h"

#include "USB.h"
#include "USBMSC.h"
#include "SD_MMC.h"

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

// Global: its constructor registers the USB interface, which has to happen
// before the core opens USB in app_main().
static USBMSC MSC;

// Guards the card controller and the flags below. The watch task, the web
// handlers and the main loop all reach this state, and two tasks mounting
// at once leaves the card unidentifiable until a power cycle.
//
// The MSC sector callbacks deliberately stay outside: they only run while
// the volume belongs to the host, which is exactly when the firmware does
// no filesystem work, and blocking them would stall the USB endpoint.
static SemaphoreHandle_t s_lock = nullptr;

static String s_deviceName = DEVICE_NAME_DEFAULT;
static bool s_cardPresent = false;
static bool s_exposed = false;
static uint64_t s_sizeMB = 0;

static void lockSd() {
  if (s_lock) xSemaphoreTake(s_lock, portMAX_DELAY);
}

static void unlockSd() {
  if (s_lock) xSemaphoreGive(s_lock);
}

static bool mountCardLocked();
static bool fsAvailableLocked();
static void cardWatchTask(void *arg);

static int32_t onRead(uint32_t lba, uint32_t offset, void *buffer, uint32_t bufsize) {
  uint32_t secSize = SD_MMC.sectorSize();
  if (!secSize) return -1;
  for (uint32_t i = 0; i < bufsize / secSize; i++) {
    if (!SD_MMC.readRAW((uint8_t *)buffer + (i * secSize), lba + i)) return -1;
  }
  return bufsize;
}

static int32_t onWrite(uint32_t lba, uint32_t offset, uint8_t *buffer, uint32_t bufsize) {
  uint32_t secSize = SD_MMC.sectorSize();
  if (!secSize) return -1;
  for (uint32_t i = 0; i < bufsize / secSize; i++) {
    if (!SD_MMC.writeRAW(buffer + (i * secSize), lba + i)) return -1;
  }
  return bufsize;
}

static bool onStartStop(uint8_t power_condition, bool start, bool load_eject) {
  (void)power_condition;
  (void)start;
  (void)load_eject;
  return true;
}

struct MountAttempt {
  bool mode1bit;
  int frequency;
  const char *label;
};

static const MountAttempt MOUNT_ATTEMPTS[] = {
  {false, BOARD_MAX_SDMMC_FREQ, "4-bit, default clock"},
  {false, 20000, "4-bit, 20 MHz"},
  {true, 20000, "1-bit, 20 MHz"},
  {true, 10000, "1-bit, 10 MHz"},
  {true, 400, "1-bit, 400 kHz"},
};

// No SD_MMC.end() on success: tearing the mount down left the card
// unidentifiable until a power cycle. format_if_mount_failed stays false.
static bool mountCardLocked() {
  for (const MountAttempt &a : MOUNT_ATTEMPTS) {
    if (!SD_MMC.setPins(SD_MMC_CLK_PIN, SD_MMC_CMD_PIN, SD_MMC_D0_PIN,
                        SD_MMC_D1_PIN, SD_MMC_D2_PIN, SD_MMC_D3_PIN)) {
      Serial.println("[sd] setPins refused");
      break;
    }
    bool mounted = SD_MMC.begin("/sdcard", a.mode1bit, false, a.frequency, 5);
    int type = SD_MMC.cardType();
    Serial.printf("[sd] try %-22s mount=%d type=%d\n", a.label, mounted, type);

    if (mounted && type != CARD_NONE) {
      s_cardPresent = true;
      s_sizeMB = SD_MMC.cardSize() / (1024 * 1024);
      Serial.printf("[sd] mounted: %s, %llu MB\n", a.label, s_sizeMB);
      return true;
    }
    SD_MMC.end();
  }

  Serial.println("[sd] every mount attempt failed (card not identified)");
  s_cardPresent = false;
  s_exposed = false;
  MSC.mediaPresent(false);
  return false;
}

static void publishGeometryLocked() {
  MSC.vendorID("LilyGO");
  MSC.productID(s_deviceName.c_str());
  MSC.productRevision("1.0");
  MSC.onRead(onRead);
  MSC.onWrite(onWrite);
  MSC.onStartStop(onStartStop);
  MSC.begin(SD_MMC.numSectors(), SD_MMC.sectorSize());
}

void usbDriveBegin(bool exposed, const String &deviceName) {
  s_lock = xSemaphoreCreateMutex();
  if (!deviceName.isEmpty()) s_deviceName = deviceName;

  lockSd();
  bool ok = mountCardLocked();
  if (ok) {
    publishGeometryLocked();
    s_exposed = exposed && s_cardPresent;
    MSC.mediaPresent(s_exposed);
    Serial.printf("USB drive: %llu MB card, exposed=%d\n", s_sizeMB, s_exposed);
  } else {
    Serial.println("USB drive: no usable card, the reader will report empty.");
  }
  unlockSd();

  xTaskCreatePinnedToCore(cardWatchTask, "sdwatch", 8192, nullptr, 1, nullptr, 0);
}

static void cardWatchTask(void *arg) {
  (void)arg;
  for (;;) {
    vTaskDelay(pdMS_TO_TICKS(15000));

    lockSd();
    if (!s_cardPresent && !s_exposed) {
      Serial.println("[sd] no card, retrying mount");
      if (mountCardLocked()) {
        publishGeometryLocked();
        Serial.println("[sd] card picked up after boot");
      }
    }
    unlockSd();
  }
}

void usbDriveSetDeviceName(const String &name) {
  if (name.isEmpty()) return;
  lockSd();
  s_deviceName = name;
  MSC.productID(s_deviceName.c_str());
  unlockSd();
}

String usbDriveGetDeviceName() {
  lockSd();
  String n = s_deviceName;
  unlockSd();
  return n;
}

void usbDriveSetExposed(bool exposed) {
  lockSd();
  Serial.printf("[sd] setExposed(%d) from exposed=%d present=%d\n",
                exposed, s_exposed, s_cardPresent);
  s_exposed = exposed && s_cardPresent;
  MSC.mediaPresent(s_exposed);
  unlockSd();
}

// readRAW() needs the mount alive, so host and firmware cannot be separated
// by unmounting. Exclusive access is a rule enforced here instead.
static bool fsAvailableLocked() {
  if (s_exposed) return false;
  if (s_cardPresent) return true;
  Serial.println("[sd] card marked absent, retrying mount");
  return mountCardLocked();
}

bool usbDriveFsAvailable() {
  lockSd();
  bool ok = fsAvailableLocked();
  unlockSd();
  return ok;
}

bool usbDrivePathIsSafe(const String &path) {
  if (path.isEmpty() || path[0] != '/') return false;
  if (path.indexOf("..") >= 0) return false;
  return true;
}

bool usbDriveList(const String &path, std::vector<SdEntry> &out) {
  if (!usbDrivePathIsSafe(path)) {
    Serial.printf("[sd] list %s refused: unsafe path\n", path.c_str());
    return false;
  }

  lockSd();
  if (!fsAvailableLocked()) {
    Serial.printf("[sd] list %s refused: exposed=%d present=%d\n",
                  path.c_str(), s_exposed, s_cardPresent);
    unlockSd();
    return false;
  }

  File dir = SD_MMC.open(path);
  if (!dir || !dir.isDirectory()) {
    Serial.printf("[sd] list %s: not a directory\n", path.c_str());
    if (dir) dir.close();
    unlockSd();
    return false;
  }

  File f = dir.openNextFile();
  while (f) {
    String n = String(f.name());
    int slash = n.lastIndexOf('/');
    if (slash >= 0) n = n.substring(slash + 1);
    out.push_back(SdEntry{n, (uint32_t)f.size(), f.isDirectory()});
    f.close();
    f = dir.openNextFile();
  }
  dir.close();
  unlockSd();

  Serial.printf("[sd] list %s -> %u entries\n", path.c_str(), (unsigned)out.size());
  return true;
}

File usbDriveOpen(const String &path) {
  if (!usbDrivePathIsSafe(path)) return File();
  lockSd();
  File f = fsAvailableLocked() ? SD_MMC.open(path, "r") : File();
  unlockSd();
  return f;
}

bool usbDriveDelete(const String &path) {
  if (!usbDrivePathIsSafe(path)) {
    Serial.printf("[sd] delete %s refused: unsafe path\n", path.c_str());
    return false;
  }

  lockSd();
  if (!fsAvailableLocked()) {
    unlockSd();
    Serial.printf("[sd] delete %s refused\n", path.c_str());
    return false;
  }

  File f = SD_MMC.open(path);
  if (!f) {
    unlockSd();
    Serial.printf("[sd] delete %s: not found\n", path.c_str());
    return false;
  }
  bool isDir = f.isDirectory();
  f.close();
  bool ok = isDir ? SD_MMC.rmdir(path) : SD_MMC.remove(path);
  unlockSd();

  Serial.printf("[sd] delete %s dir=%d -> %d\n", path.c_str(), isDir, ok);
  return ok;
}

UsbDriveStatus usbDriveGetStatus() {
  lockSd();
  UsbDriveStatus s{s_cardPresent, s_exposed, s_sizeMB};
  unlockSd();
  return s;
}
