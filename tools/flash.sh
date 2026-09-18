#!/usr/bin/env bash
# Compiles and uploads. Pass a port to override detection.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "$ROOT/tools/fqbn.sh"

find_port() {
  arduino-cli board list --format json 2>/dev/null \
    | python3 -c 'import json,sys; p=[b["port"]["address"] for b in json.load(sys.stdin).get("detected_ports",[]) if b["port"].get("protocol")=="serial" and ("ACM" in b["port"]["address"] or "USB" in b["port"]["address"])]; print(p[0] if p else "")'
}

wait_for_port() {
  for _ in $(seq 1 15); do
    p="$(find_port)"
    if [[ -n "$p" ]]; then echo "$p"; return 0; fi
    sleep 1
  done
  return 1
}

PORT="${1:-$(wait_for_port || true)}"
if [[ -z "$PORT" ]]; then
  echo "No serial port found after 15s." >&2
  echo "Replug the dongle while holding the button, then try again." >&2
  exit 1
fi

# TinyUSB does not implement the DTR/RTS reset esptool expects. Opening the
# port at 1200 baud reboots the board into the bootloader instead.
echo "Switching to bootloader on $PORT"
python3 - "$PORT" <<'PY' || true
import serial, sys, time
try:
    s = serial.Serial(sys.argv[1], 1200, timeout=1)
    s.setDTR(False)
    time.sleep(0.3)
    s.close()
except Exception:
    pass
PY
sleep 3

PORT="${1:-$(wait_for_port || true)}"
if [[ -z "$PORT" ]]; then
  echo "Port disappeared after switching to the bootloader." >&2
  exit 1
fi

echo "Port: $PORT"
arduino-cli compile --fqbn "$FQBN" "${BUILD_PROPS[@]}" "$ROOT/$SKETCH"
arduino-cli upload --fqbn "$FQBN" --port "$PORT" "$ROOT/$SKETCH"

echo
echo "Flashed."
