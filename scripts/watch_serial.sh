#!/usr/bin/env bash
# watch_serial.sh — Launch separate terminal monitor for OpenFeatureOS serial output
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PYTHON_SCRIPT="${SCRIPT_DIR}/watch_serial.py"

chmod +x "${PYTHON_SCRIPT}"
exec python3 "${PYTHON_SCRIPT}" "$@"
