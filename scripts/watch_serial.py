#!/usr/bin/env python3
"""
watch_serial.py — Real-time Serial Terminal Monitor for OpenFeatureOS
Target: Unisoc T107 (UMS9117) / Nokia 2660 Flip (UART1: 115200 8N1, 1.8V TTL)

Uses pure Python 3 standard library (os, termios, fcntl, select).
Zero third-party pip dependencies required.
"""

import sys
import os
import glob
import time
import termios
import select
import argparse

# Common USB serial patterns on macOS and Linux
PORT_PATTERNS = [
    "/dev/cu.usbserial*",
    "/dev/cu.usbmodem*",
    "/dev/cu.wchusbserial*",
    "/dev/cu.SLAB_USBtoUART*",
    "/dev/cu.CP210*",
    "/dev/cu.FTDI*",
    "/dev/cu.uart*",
    "/dev/ttyUSB*",
    "/dev/ttyACM*",
]

BAUD_MAP = {
    9600: termios.B9600,
    19200: termios.B19200,
    38400: termios.B38400,
    57600: termios.B57600,
    115200: termios.B115200,
    230400: termios.B230400,
}

def find_serial_ports():
    ports = []
    for pat in PORT_PATTERNS:
        ports.extend(glob.glob(pat))
    return sorted(list(set(ports)))

def configure_port(fd, baud_rate):
    if baud_rate not in BAUD_MAP:
        raise ValueError(f"Unsupported baud rate: {baud_rate}")
    baud_const = BAUD_MAP[baud_rate]

    attrs = termios.tcgetattr(fd)
    # iflag: Disable software flow control, carriage return mapping
    attrs[0] &= ~(termios.IXON | termios.IXOFF | termios.IXANY | termios.ICRNL | termios.INLCR)
    # oflag: Disable post-processing of output
    attrs[1] &= ~termios.OPOST
    # cflag: CS8 (8 bits), CREAD (enable receiver), CLOCAL (ignore modem control)
    attrs[2] &= ~(termios.CSIZE | termios.PARENB | termios.CSTOPB | termios.CRTSCTS)
    attrs[2] |= (termios.CS8 | termios.CREAD | termios.CLOCAL)
    # lflag: Raw mode (no echo, no canonical, no extended, no signals)
    attrs[3] &= ~(termios.ECHO | termios.ECHONL | termios.ICANON | termios.ISIG | termios.IEXTEN)
    # cc: Read minimum 1 byte, non-blocking timeout
    attrs[6][termios.VMIN] = 1
    attrs[6][termios.VTIME] = 0

    # Set input and output speed
    termios.cfsetispeed(attrs, baud_const)
    termios.cfsetospeed(attrs, baud_const)
    termios.tcsetattr(fd, termios.TCSANOW, attrs)

def main():
    parser = argparse.ArgumentParser(description="OpenFeatureOS Serial Console Monitor")
    parser.add_argument("-p", "--port", help="Serial device path (e.g., /dev/cu.usbserial-XXXX)")
    parser.add_argument("-b", "--baud", type=int, default=115200, help="Baud rate (default: 115200)")
    parser.add_argument("--timestamps", action="store_true", help="Prefix each line with wall clock timestamp")
    args = parser.parse_args()

    port = args.port
    baud = args.baud

    print("==================================================================")
    print("  OpenFeatureOS - Serial Console Terminal Monitor (115200 8N1)   ")
    print("==================================================================")
    print(f" Target Baud: {baud} bps | Target: Nokia 2660 Flip (UART1 1.8V TTL)")
    print(" Press Ctrl+C at any time to exit.\n")

    # If no port specified, auto-detect or wait for device
    if not port:
        ports = find_serial_ports()
        if len(ports) == 1:
            port = ports[0]
            print(f"[+] Auto-detected serial port: {port}")
        elif len(ports) > 1:
            print("[?] Multiple serial ports found:")
            for idx, p in enumerate(ports, 1):
                print(f"    [{idx}] {p}")
            port = ports[0]
            print(f"[*] Selecting first port: {port} (use -p to override)")
        else:
            print("[*] Waiting for USB serial adapter to be connected...")
            print("    (Plug in your 1.8V USB-to-UART adapter at any time)")
            try:
                while not port:
                    time.sleep(1.0)
                    available = find_serial_ports()
                    if available:
                        port = available[0]
                        print(f"\n[+] Detected new serial device: {port}")
                        break
                    sys.stdout.write(".")
                    sys.stdout.flush()
            except KeyboardInterrupt:
                print("\n[!] Aborted by user.")
                sys.exit(0)

    try:
        fd = os.open(port, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
    except Exception as e:
        print(f"[-] ERROR: Failed to open serial port {port}: {e}")
        print("    Check permissions or ensure the device is not held by another process.")
        sys.exit(1)

    try:
        configure_port(fd, baud)
    except Exception as e:
        print(f"[-] ERROR: Failed to configure serial parameters: {e}")
        os.close(fd)
        sys.exit(1)

    print(f"[+] Connected to {port} @ {baud} bps (8N1). Streaming live output...\n")
    print("------------------------------------------------------------------")

    line_buf = ""
    try:
        while True:
            # Poll for incoming bytes with 100ms timeout
            r, _, _ = select.select([fd], [], [], 0.1)
            if fd in r:
                data = os.read(fd, 1024)
                if not data:
                    time.sleep(0.01)
                    continue
                text = data.decode("utf-8", errors="replace")
                
                if args.timestamps:
                    line_buf += text
                    while "\n" in line_buf:
                        line, line_buf = line_buf.split("\n", 1)
                        ts = time.strftime("%H:%M:%S")
                        sys.stdout.write(f"[{ts}] {line}\n")
                        sys.stdout.flush()
                else:
                    sys.stdout.write(text)
                    sys.stdout.flush()
    except KeyboardInterrupt:
        print("\n\n[+] Session closed by user.")
    finally:
        try:
            os.close(fd)
        except Exception:
            pass

if __name__ == "__main__":
    main()
