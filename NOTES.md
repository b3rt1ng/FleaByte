# Notes on the hardware

Things that cost time building this. None of them produce a compile error.

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
