# T-Dongle Keyboard

Firmware for the LilyGO T-Dongle S3 that turns it into a USB keyboard you
drive from a web page.

Plug it in and the dongle brings up its own Wi-Fi network. Join it, open the
page, pick the keyboard layout of the machine it is plugged into, write or
select a script, and run it. The screen shows the network, how many devices
are connected, and what the dongle is doing.

## What it does

* Keystroke scripting with a DuckyScript subset, from a web editor
* Twelve keyboard layouts, switchable at runtime and mid-script
* Payload library stored on the dongle, editable from the browser
* Cancellable countdown before a payload starts
* microSD exposed to the host as a removable drive, with a file browser
* Status screen with run progress, and a configurable RGB LED
* Served entirely from the dongle, no internet, no cloud

## Scope of use

Machines you own, or for which you hold written authorisation. This is a
keystroke injection tool: it types into whatever it is plugged into.

Execution is always triggered from the web interface. Nothing runs on
plug-in, and the button on the dongle never starts a payload.

## Hardware

LilyGO T-Dongle S3 (ESP32-S3, 16 MB flash, no PSRAM). Nothing else is
required. A microSD card is optional and only used by the USB drive feature.

Pin assignments come from the
[official LilyGO examples](https://github.com/Xinyuan-LilyGO/T-Dongle-S3).

| Function | Pins |
|---|---|
| ST7735 display, 160x80 | MOSI 3, SCLK 5, CS 4, DC 2, RST 1, backlight 38 |
| APA102 LED | data 40, clock 39 |
| Button | 0 |
| microSD (SD_MMC) | CLK 12, CMD 16, D0 14, D1 17, D2 21, D3 18 |

## Installing

Prebuilt images are attached to each
[release](../../releases). One file, flashed at offset `0x0`:

```sh
esptool --chip esp32s3 --port /dev/ttyACM0 write-flash 0x0 firmware.bin
```

Hold the button while plugging the dongle in to put it in the bootloader
first, then release. The image stops before the filesystem partition, so an
update keeps saved payloads and settings.

A browser flasher is published alongside each release if you would rather
not install anything.

## Building

Requires ESP32 Arduino core **3.3.0 or later** and USB mode set to
**USB-OTG (TinyUSB)**. The "Hardware CDC and JTAG" mode cannot do HID, and
the sketch refuses to build if you select it.

### arduino-cli

```sh
arduino-cli config add board_manager.additional_urls \
  https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli core update-index
arduino-cli core install esp32:esp32
arduino-cli lib install "Adafruit GFX Library" "Adafruit ST7735 and ST7789 Library"

./tools/build.sh
./tools/flash.sh
```

The scripts wrap this FQBN:

```
esp32:esp32:esp32s3:USBMode=default,CDCOnBoot=cdc,FlashSize=16M,PSRAM=disabled,PartitionScheme=custom
```

### Arduino IDE

Board *ESP32S3 Dev Module*, then USB Mode **USB-OTG (TinyUSB)**, USB CDC On
Boot *Enabled*, Flash Size *16MB*, PSRAM *Disabled*, Partition Scheme
*Custom*. Install **Adafruit GFX Library** and **Adafruit ST7735 and ST7789
Library** from the Library Manager.

### Flashing

First flash: hold the button while plugging the dongle in, then release.

After that `./tools/flash.sh` handles it alone. Once the firmware runs, the
serial port belongs to TinyUSB, which does not implement the DTR/RTS reset
esptool expects, so the script opens the port at 1200 baud to trigger
`usb_persist_restart(RESTART_BOOTLOADER)` in the core first.

If the button was held down while plugging in, the dongle stays in the ROM
bootloader: dark screen, no Wi-Fi, silent serial port. Unplug and replug
without touching it.

## First run

The screen shows the network name and password. Both derive from the device
MAC, so every dongle starts with different credentials:

```
T-Dongle-8218
dongle-A0058218
```

Join the network and the captive portal opens the page. Otherwise go to
`http://192.168.4.1` or `http://dongle.local`. Both can be changed in
settings.

Run `00-test-layout.txt` into a text editor on the target machine before
anything else. It types the characters that move between layouts, so a
mismatch is obvious at a glance.

### Getting back in

A mistyped Wi-Fi password would lock you out of the only interface. Hold the
button for five seconds: the settings file is deleted and the device reboots
on its built-in credentials. Payloads are kept.

## Script commands

| Command | Effect |
|---|---|
| `REM text` | Comment, also `#` and `//` |
| `META windows\|linux\|macos` | Tags the payload, shows an OS icon in the library |
| `STRING text` | Types the text |
| `STRINGLN text` | Types the text, then Enter |
| `DELAY n` | Pauses n milliseconds |
| `DEFAULTDELAY n` | Implicit pause after every line |
| `DEFAULTCHARDELAY n` | Pause between characters |
| `LAYOUT code` | Switches layout mid-script |
| `REPEAT n` | Replays the previous line n times |
| Named keys | `ENTER` `TAB` `ESC` `SPACE` `BACKSPACE` `DELETE` `INSERT` `HOME` `END` `PAGEUP` `PAGEDOWN` `UP` `DOWN` `LEFT` `RIGHT` `MENU` `CAPSLOCK` `PRINTSCREEN` `PAUSE` `F1`-`F12` |
| Modifiers | `CTRL` `SHIFT` `ALT` `GUI` `ALTGR` then a key: `GUI r`, `CTRL ALT DELETE` |

Layout codes: `us fr de ch hu es it pt br se dk jp`.

Only ASCII is typed. Accented characters in a `STRING` are skipped and
reported in the run log rather than producing a wrong key.

### Layouts and non-Latin input

Layout tables map ASCII to key positions, which is why the list stops where
it does. With a Cyrillic or CJK input mode active, no key produces an ASCII
letter at all, so no table can help. Scripts for those targets switch the
host back to Latin input first, usually with `GUI SPACE`. See
`50-switch-input-language.txt`.

## USB drive

The mass storage interface is always advertised; the setting decides whether
it reports a card. While the card is handed to the host, the file browser in
the settings page refuses. The host gets raw sector access and the firmware
sees a filesystem, and both views cannot be live at once without corrupting
the card.

Format cards as FAT32 or FAT16. The firmware never formats a card by itself,
whatever the mount error.

## Settings

Layout, device name, screen orientation and backlight, LED colour, Wi-Fi
credentials, USB drive. Stored on internal flash and kept across reflashing,
since the firmware goes to `app0` while settings live on the `spiffs`
partition.

Changing the Wi-Fi credentials restarts the dongle, because the network
being reconfigured is the one carrying the request. Everything else applies
live.

## Repository layout

| Path | Contents |
|---|---|
| `wifi_ducky_s3.ino` | Startup, access point, main loop |
| `config.h` | Pins, defaults, limits |
| `ducky.h/.cpp` | HID keyboard, layouts, interpreter, run task |
| `storage.h/.cpp` | Payload library and settings on LittleFS |
| `ui_display.h/.cpp` | ST7735 screen and APA102 LED |
| `usb_drive.h/.cpp` | Mass storage and card browsing |
| `web_api.h/.cpp` | HTTP server, API, captive portal |
| `web_assets.h` | Web interface, compiled into the firmware |
| `partitions.csv` | 16 MB layout, 4 MB app, 7.88 MB filesystem |
| `tools/` | Build, flash and release scripts |
| `docs/` | Browser flasher published to GitHub Pages |

Payloads run in their own FreeRTOS task, which keeps the web server
answering during a run and makes the progress readout and *Stop* work.

## Notes on the hardware

Things that cost time during development. None of them produce a compile
error.

**`PartitionScheme=custom` is mandatory.** Without it, arduino-esp32 ignores
the `partitions.csv` in the sketch folder silently and builds against a
1.25 MB partition.

**USB descriptor names are set at compile time.** With CDC on boot, the core
calls `USB.begin()` from `app_main()`, before `setup()`. A
`USB.productName()` call in `setup()` runs without error and does nothing.
The names live in `tools/fqbn.sh`. The compiled core is cached in
`~/.cache/arduino/cores` and that cache ignores `compiler.cpp.extra_flags`,
so clear it when changing them.

**`WiFi.softAPmacAddress()` returns zeros before `softAP()`.** Read the MAC
from eFuse with `esp_read_mac(mac, ESP_MAC_WIFI_SOFTAP)` instead.

**arduino-cli splits build properties on spaces.** A value containing a
space reaches the linker as several arguments. Quotes stay literal, since
arduino-cli does not invoke a shell.

**The backlight is active low.** A duty cycle of 0 is full brightness.

**The panel clips its outermost row and column.** Anything drawn flush to the
edge loses pixels, which is why the screen frame sits 3 px in.

**Nothing may draw outside the rectangle it clears.** The display has no
framebuffer and repaints only the fields that changed, so the glitch effect
declares the margin its channel-split ghosts land in.

**`SD_MMC.end()` can leave the card unidentifiable** until a power cycle, so
the firmware never unmounts. `readRAW()` needs the mount alive anyway, which
is why host and firmware access is arbitrated by rule rather than by
unmounting.

**A card that reads fine in a PC reader can still fail here.** A USB reader
and an SDMMC controller do not have the same tolerance. `ESP_ERR_TIMEOUT`
(0x107) means the card never answered, which is electrical rather than a
filesystem problem.

## Credits

Pin assignments and panel initialisation values come from
[LilyGO's T-Dongle S3 examples](https://github.com/Xinyuan-LilyGO/T-Dongle-S3)
(MIT). The display is driven through
[Adafruit GFX](https://github.com/adafruit/Adafruit-GFX-Library) and
[Adafruit ST7735](https://github.com/adafruit/Adafruit-ST7735-Library)
(BSD). Keyboard layout tables ship with the
[ESP32 Arduino core](https://github.com/espressif/arduino-esp32).

## License

MIT, see [LICENSE](LICENSE).
