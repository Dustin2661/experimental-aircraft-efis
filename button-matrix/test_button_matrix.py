#!/usr/bin/env python3
"""Interactive serial bench test for the Teensy 4.1 button-matrix firmware."""

import argparse
import sys
import time


class TestFailure(Exception):
    pass


def read_matching_line(port, expected, timeout):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        line = port.readline().decode("ascii", errors="replace").strip()
        if line == expected:
            return
        if line.startswith("ERROR"):
            raise TestFailure(f"Firmware reported {line!r}")
    raise TestFailure(f"Timed out waiting for {expected!r}")


def test_device(port_name, baud, timeout):
    try:
        import serial
    except ImportError as error:
        raise TestFailure("Install pyserial (python3 -m pip install pyserial) to use this test") from error

    passed = 0
    with serial.Serial(port_name, baudrate=baud, timeout=0.2, write_timeout=timeout) as port:
        read_matching_line(port, "READY", timeout)
        port.write(b"TEST\n")
        read_matching_line(port, "TEST OK", timeout)

        for button in range(1, 21):
            port.write(f"LED {button} 2048\n".encode("ascii"))
            read_matching_line(port, f"LED {button} 2048", timeout)
            print(f"Button {button}: verify its LED, then press and release it.")
            read_matching_line(port, f"BUTTON {button} DOWN", timeout)
            read_matching_line(port, f"BUTTON {button} UP", timeout)
            port.write(f"LED {button} 0\n".encode("ascii"))
            read_matching_line(port, f"LED {button} 0", timeout)
            print(f"  PASS: button {button} and LED")
            passed += 1

    print(f"PASS: {passed}/20 buttons and LEDs verified")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", required=True, help="Teensy serial port, e.g. /dev/ttyACM0 or COM4")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--timeout", type=float, default=30.0, help="seconds allowed for each press/release")
    args = parser.parse_args()
    try:
        test_device(args.port, args.baud, args.timeout)
    except (OSError, TestFailure) as error:
        print(f"FAIL: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
