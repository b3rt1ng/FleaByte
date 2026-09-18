#include "usb_drive.h"
#include "config.h"

#include "USB.h"
#include "USBMSC.h"
#include "SD_MMC.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// Global: its constructor registers the USB interface, which has to happen
// before the core opens USB in app_main().
static USBMSC MSC;

static String s_deviceName = DEVICE_NAME_DEFAULT;
static bool s_cardPresent = false;
static bool s_exposed = false;
static uint64_t s_sizeMB = 0;

static bool mountCard();
static bool remount();
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

void usbDriveBegin(bool exposed, const String &deviceName) {
  if (!deviceName.isEmpty()) s_deviceName = deviceName;

  if (!mountCard()) {
    Serial.println("USB drive: no usable card, the reader will report empty.");
    xTaskCreatePinnedToCore(cardWatchTask, "sdwatch", 4096, nullptr, 1, nullptr, 0);
    return;
  }

  MSC.vendorID("LilyGO");
  MSC.productID(s_deviceName.c_str());
  MSC.productRevision("1.0");
  MSC.onRead(onRead);
  MSC.onWrite(onWrite);
  MSC.onStartStop(onStartStop);

  if (!MSC.begin(SD_MMC.numSectors(), SD_MMC.sectorSize())) {
    s_cardPresent = false;
    MSC.mediaPresent(false);
    Serial.println("USB drive: card geometry rejected.");
    return;
  }

  usbDriveSetExposed(exposed);
  Serial.printf("USB drive: %llu MB card, exposed=%d\n", s_sizeMB, exposed);
  xTaskCreatePinnedToCore(cardWatchTask, "sdwatch", 4096, nullptr, 1, nullptr, 0);
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

// No SD_MMC.end() anywhere: tearing the mount down left the card
// unidentifiable until a power cycle. format_if_mount_failed stays false.
static bool mountCard() {
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

static bool remount() { return mountCard(); }

static void cardWatchTask(void *arg) {
  (void)arg;
  for (;;) {
    vTaskDelay(pdMS_TO_TICKS(15000));
    if (s_cardPresent || s_exposed) continue;

    Serial.println("[sd] no card, retrying mount");
    if (mountCard()) {
      MSC.vendorID("LilyGO");
      MSC.productID(s_deviceName.c_str());
      MSC.productRevision("1.0");
      MSC.onRead(onRead);
      MSC.onWrite(onWrite);
      MSC.onStartStop(onStartStop);
      MSC.begin(SD_MMC.numSectors(), SD_MMC.sectorSize());
      Serial.println("[sd] card picked up after boot");
    }
  }
}

void usbDriveSetDeviceName(const String &name) {
  if (name.isEmpty()) return;
  s_deviceName = name;

  MSC.productID(s_deviceName.c_str());
}

String usbDriveGetDeviceName() { return s_deviceName; }

void usbDriveSetExposed(bool exposed) {
  Serial.printf("[sd] setExposed(%d) from exposed=%d present=%d\n",
                exposed, s_exposed, s_cardPresent);

  s_exposed = exposed && s_cardPresent;
  MSC.mediaPresent(s_exposed);
}

// readRAW() needs the mount alive, so host and firmware cannot be separated
// by unmounting. Exclusive access is a rule enforced here instead.
bool usbDriveFsAvailable() {
  if (s_exposed) return false;
  if (s_cardPresent) return true;
  Serial.println("[sd] card marked absent, retrying mount");
  return remount();
}

bool usbDrivePathIsSafe(const String &path) {
  if (path.isEmpty() || path[0] != '/') return false;
  if (path.indexOf("..") >= 0) return false;
  return true;
}

bool usbDriveList(const String &path, std::vector<SdEntry> &out) {
  if (!usbDriveFsAvailable()) {
    Serial.printf("[sd] list %s refused: exposed=%d present=%d\n",
                  path.c_str(), s_exposed, s_cardPresent);
    return false;
  }
  if (!usbDrivePathIsSafe(path)) {
    Serial.printf("[sd] list %s refused: unsafe path\n", path.c_str());
    return false;
  }

  File dir = SD_MMC.open(path);
  if (!dir) {
    Serial.printf("[sd] list %s: open failed\n", path.c_str());
    return false;
  }
  if (!dir.isDirectory()) {
    Serial.printf("[sd] list %s: not a directory\n", path.c_str());
    dir.close();
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
  Serial.printf("[sd] list %s -> %u entries\n", path.c_str(), (unsigned)out.size());
  return true;
}

File usbDriveOpen(const String &path) {
  if (!usbDriveFsAvailable() || !usbDrivePathIsSafe(path)) return File();
  return SD_MMC.open(path, "r");
}

bool usbDriveDelete(const String &path) {
  if (!usbDriveFsAvailable() || !usbDrivePathIsSafe(path)) {
    Serial.printf("[sd] delete %s refused\n", path.c_str());
    return false;
  }
  File f = SD_MMC.open(path);
  if (!f) {
    Serial.printf("[sd] delete %s: not found\n", path.c_str());
    return false;
  }
  bool isDir = f.isDirectory();
  f.close();
  bool ok = isDir ? SD_MMC.rmdir(path) : SD_MMC.remove(path);
  Serial.printf("[sd] delete %s dir=%d -> %d\n", path.c_str(), isDir, ok);
  return ok;
}

UsbDriveStatus usbDriveGetStatus() {
  return UsbDriveStatus{s_cardPresent, s_exposed, s_sizeMB};
}
