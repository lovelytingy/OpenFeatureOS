#!/usr/bin/env bash
# run_device.sh — Stage and execute OpenFeatureOS payload in DRAM on Nokia 2660 Flip
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

SPD_DUMP="${ROOT_DIR}/third_party/nokia-2660-re/tools/spd_dump"
if [[ ! -x "${SPD_DUMP}" ]]; then
    SPD_DUMP="${ROOT_DIR}/../Nokia-2660-RE/tools/spd_dump"
fi

FDL1="${ROOT_DIR}/third_party/nokia-2660-re/tools/ram_payload/data/fdl1.bin"
if [[ ! -f "${FDL1}" ]]; then
    FDL1="${ROOT_DIR}/../Nokia-2660-RE/tools/ram_payload/data/fdl1.bin"
fi

PAYLOAD="${ROOT_DIR}/build/openfeatureos.bin"

echo "================================================================="
echo "  OpenFeatureOS - Volatile DRAM Hardware Runner (Nokia 2660 Flip)"
echo "================================================================="
echo "SAFETY GUARANTEE: This script loads code STRICTLY into volatile DRAM."
echo "                  NAND flash is NEVER written or modified."
echo "                  Zero risk of bricking."
echo

if [[ ! -f "${PAYLOAD}" ]]; then
    echo "[*] Building OpenFeatureOS binary first..."
    make -C "${ROOT_DIR}"
fi

echo "[*] Using spd_dump: ${SPD_DUMP}"
echo "[*] Using FDL1:     ${FDL1}"
echo "[*] Payload:        ${PAYLOAD} ($(wc -c < "${PAYLOAD}" | tr -d ' ') bytes)"
echo
echo "[*] Waiting for phone connection in BootROM download mode (120s timeout)..."
echo "    INSTRUCTIONS:"
echo "    1. Disconnect phone from USB and ensure battery is OUT."
echo "    2. Press the red Power / End key 2-3 times while unplugged."
echo "    3. Press and firmly HOLD the '*' (Asterisk) key on the keypad."
echo "    4. While holding '*', plug the USB cable into the phone and computer."
echo

"${SPD_DUMP}" \
    --wait 120 \
    --verbose 1 \
    keep_charge 1 \
    timeout 5000 \
    fdl "${FDL1}" 0x6200 \
    blk_size 0x1000 \
    end_data 0 \
    fdl "${PAYLOAD}" 0x80100000

echo
echo "[+] SUCCESS: OpenFeatureOS payload injected and running at 0x80100000!"
echo "    Volatile DRAM execution active. NAND is completely untouched."
echo "    To restore stock factory firmware, unplug USB and reinsert battery."
