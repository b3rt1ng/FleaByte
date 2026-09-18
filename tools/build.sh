#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "$ROOT/tools/fqbn.sh"

arduino-cli compile --fqbn "$FQBN" "${BUILD_PROPS[@]}" "$ROOT/$SKETCH" "$@"
