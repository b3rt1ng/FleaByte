#pragma once
#include <Arduino.h>

#define FIRMWARE_VERSION "0.4.0"

#define AP_SSID_PREFIX     "Fleabyte"
#define AP_PASSWORD_PREFIX "flea-"
#define AP_CHANNEL     6
#define AP_MAX_CLIENTS 4
#define MDNS_HOST      "fleabyte"

#define TFT_MOSI 3
#define TFT_SCLK 5
#define TFT_CS   4
#define TFT_DC   2
#define TFT_RST  1
#define TFT_BL   38

// Active low: 0 is full brightness, 255 is off.
#define TFT_BL_DUTY_ON 0

#define TFT_ROTATION 1

#define TFT_SWAP_RED_BLUE 1

#define LED_DI_PIN 40
#define LED_CI_PIN 39
#define LED_BRIGHTNESS 6

#define SD_MMC_CLK_PIN 12
#define SD_MMC_CMD_PIN 16
#define SD_MMC_D0_PIN  14
#define SD_MMC_D1_PIN  17
#define SD_MMC_D2_PIN  21
#define SD_MMC_D3_PIN  18

#define BOOT_PIN 0

#define DEFAULT_CHAR_DELAY_MS 8
#define DEFAULT_LINE_DELAY_MS 0
#define MAX_SCRIPT_BYTES      16384
#define MAX_LOG_BYTES         4096

#define PAYLOAD_DIR   "/payloads"
#define SETTINGS_FILE "/settings.txt"
#define MAX_NAME_LEN  40

#define SSID_MIN_LEN     1
#define SSID_MAX_LEN     32
#define PASSWORD_MIN_LEN 8
#define PASSWORD_MAX_LEN 63

#define FACTORY_RESET_HOLD_MS 5000

// The LED reports state rather than taste, so the colours are fixed.
// Standby blue, red while waiting for a device or running, green once a
// payload lands.
#define LED_STANDBY_R 0x00
#define LED_STANDBY_G 0x28
#define LED_STANDBY_B 0xC8
#define LED_DONE_R    0x00
#define LED_DONE_G    0xC8
#define LED_DONE_B    0x3C

#define LED_BREATH_WAIT_MS 2200
#define LED_BREATH_RUN_MS  700
#define LED_OUTCOME_MS     5000

// Colours never cut over. Half of this fades the old one out, half fades the
// new one in, so green to blue dips through black instead of through the
// muddy teals a straight interpolation would cross.
#define LED_FADE_MS  420
#define LED_FRAME_MS 16

#define SCREEN_WAKE_MS 8000

#define START_DELAY_MAX 3600

// Bumping this overwrites the bundled example payloads on next boot.
#define PAYLOAD_SEED_VERSION 7

#define DEVICE_NAME_DEFAULT "Fleabyte"
#define DEVICE_NAME_MAX     16
