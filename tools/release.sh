#!/usr/bin/env bash
# Builds a single flashable image plus its checksum into release/.
# The image stops before the filesystem partition, so flashing it keeps
# saved payloads and settings.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "$ROOT/tools/fqbn.sh"

OUT="$ROOT/release"
rm -rf "$OUT"
mkdir -p "$OUT/raw"

arduino-cli compile --fqbn "$FQBN" "${BUILD_PROPS[@]}" \
  --output-dir "$OUT/raw" "$ROOT/$SKETCH"

ESPTOOL="$(find "$HOME/.arduino15/packages/esp32/tools/esptool_py" \
  -type f \( -name esptool -o -name esptool.py \) 2>/dev/null | head -1)"
BOOTAPP="$(find "$HOME/.arduino15/packages/esp32" -name boot_app0.bin 2>/dev/null | head -1)"

if [[ -z "$ESPTOOL" || -z "$BOOTAPP" ]]; then
  echo "esptool or boot_app0.bin not found in the installed core" >&2
  exit 1
fi

"$ESPTOOL" --chip esp32s3 merge-bin -o "$OUT/firmware.bin" \
  0x0     "$OUT/raw/${SKETCH}.ino.bootloader.bin" \
  0x8000  "$OUT/raw/${SKETCH}.ino.partitions.bin" \
  0xe000  "$BOOTAPP" \
  0x10000 "$OUT/raw/${SKETCH}.ino.bin"

VERSION="$(sed -n 's/.*FIRMWARE_VERSION *"\([^"]*\)".*/\1/p' "$ROOT/$SKETCH/config.h")"

( cd "$OUT" && sha256sum firmware.bin > firmware.bin.sha256 )

echo
echo "version  ${VERSION}"
echo "image    $(du -h "$OUT/firmware.bin" | cut -f1)  ->  $OUT/firmware.bin"
sed 's/^/sha256   /' "$OUT/firmware.bin.sha256"
