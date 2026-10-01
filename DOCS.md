# Fleabyte documentation

Everything beyond getting one running. The overview lives in the
[README](README.md), and the hardware quirks in [NOTES.md](NOTES.md).

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
Fleabyte-8218
flea-A0058218
```

Join the network and the captive portal opens the page. Otherwise go to
`http://192.168.4.1` or `http://fleabyte.local`. Both can be changed in
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
| `WAIT_FOR_HOST [ms]` | Waits until the host acknowledges the keyboard, 5000 ms by default |
| `DEFAULTDELAY n` | Implicit pause after every line |
| `DEFAULTCHARDELAY n` | Pause between characters |
| `LAYOUT code` | Switches layout mid-script |
| `REPEAT n` | Replays the previous line n times |
| Named keys | `ENTER` `TAB` `ESC` `SPACE` `BACKSPACE` `DELETE` `INSERT` `HOME` `END` `PAGEUP` `PAGEDOWN` `UP` `DOWN` `LEFT` `RIGHT` `MENU` `CAPSLOCK` `PRINTSCREEN` `PAUSE` `F1`-`F12` |
| Modifiers | `CTRL` `SHIFT` `ALT` `GUI` `ALTGR` then a key: `GUI r`, `CTRL ALT DELETE` |

Layout codes: `us fr de ch hu es it pt br se dk jp`.

Only ASCII is typed. Accented characters in a `STRING` are skipped and
reported in the run log rather than producing a wrong key.

`WAIT_FOR_HOST` replaces the blind `DELAY` most payloads open with. Not
every host sends the report unprompted, so it carries on when the timeout
expires rather than failing the run.

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

Layout, device name, screen orientation and backlight, status light on or
off, Wi-Fi credentials, USB drive. Stored on internal flash and kept across
reflashing, since the firmware goes to `app0` while settings live on the
`spiffs` partition.

Changing the Wi-Fi credentials restarts the dongle, because the network
being reconfigured is the one carrying the request. Everything else applies
live.

## Status light

The colours are fixed and report state rather than taste. Only on or off is
configurable.

| Light | Meaning |
|---|---|
| Red, slow breath | No device has joined the access point yet |
| Blue, steady | Standby |
| Red, fast pulse | A payload is armed or running |
| Green, 5 seconds | The payload finished, then back to standby |
| Red, steady 5 seconds | The payload stopped on an error |

Colours never cut over. Each change fades the old one out and the new one
in, dipping through black rather than crossing the muddy hues a direct
interpolation would pass through. Both the fades and the breathing use the
same squared curve, since perceived brightness is far from linear and a ramp
that is linear in value reads as a cliff at one end.

## Fire after boot

Arms one run at the next power-up, from the bar under the editor. It arms
the script **as it stands in the editor**, not a reference to a library
entry: edit it and arm what you see. The device keeps its own snapshot in
`/armed.txt`, outside the payload directory, so renaming or deleting a
library payload leaves the arming alone.

The existence of that file is the armed state. There is no second copy of
that fact to drift out of sync, and the badge reports its size.

It is a single shot: the snapshot is deleted *before* the payload is queued,
and nothing is queued unless the delete took, so a crash or a replug during
the run cannot turn one arming into a payload that fires on every plug. A
factory reset clears it too.

At power-up the host has usually not finished setting the keyboard up, and
anything typed before that is lost. Start the script with `WAIT_FOR_HOST`,
or give it a start delay, which doubles as the window to pull the dongle
back out.

## Repository layout

| Path | Contents |
|---|---|
| `fleabyte.ino` | Startup, access point, main loop |
| `config.h` | Pins, defaults, limits |
| `ducky.h/.cpp` | HID keyboard, layouts, interpreter, run task |
| `storage.h/.cpp` | Payload library and settings on LittleFS |
| `ui_display.h/.cpp` | ST7735 screen and APA102 LED |
| `usb_drive.h/.cpp` | Mass storage and card browsing |
| `web_api.h/.cpp` | HTTP server, API, captive portal |
| `web_assets.h` | Web interface, compiled into the firmware |
| `partitions.csv` | 16 MB layout, 4 MB app, 7.88 MB filesystem |
| `tools/` | Build, flash, release and screen rendering scripts |
| `screens/` | Rendered screen images used by the README |

Payloads run in their own FreeRTOS task, which keeps the web server
answering during a run and makes the progress readout and *Stop* work.
