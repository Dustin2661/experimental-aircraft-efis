# D10 REV19 FINAL R3 button matrix firmware

## Scope and electrical cautions

The repository did not include the PCB CAD files, assembled-board continuity
measurements, or the CM5 CAN protocol. The firmware uses the supplied J1
description as an unverified wiring reference. Confirm every signal with a
multimeter before connecting the Teensy; this is not flight-certified firmware.

J1 pin 8 is the board's **+5 V backlight supply**, not a GPIO or PWM input.
Never connect it to Teensy pin 9. The board connects the LED anodes to this
common supply and the cathodes to ground, so the board as described does not
provide a PWM-controlled LED net. Pin 9 outputs a 3.3 V PWM control signal for
an external, suitably rated high-side LED power driver. Do not connect the
LEDs directly to pin 9. All Teensy GPIOs are 3.3 V only.

The suggested MCU-pad labels conflict for Teensy pins 4 and 8. The Arduino
digital pin numbers are distinct physical pins; this firmware uses the
proposed digital assignments and relies on the Teensy core's pin mapping.
Verify pin continuity and the exact assembled connector orientation before
wiring.

J1 pin 8 may power the PCB backlights; it must not be joined to Teensy VIN
without checking the supply and USB back-feed arrangement. Use a common ground.
The CAN controller in Teensy 4.1 requires an external CAN transceiver; the
controller pins cannot be connected directly to CANH/CANL.

## J1-to-Teensy wiring

| J1 signal | J1 pad | Teensy pin | Firmware role |
|---|---:|---:|---|
| ROW0 | 9 | 0 | Active-low row output |
| ROW1 | 10 | 1 | Active-low row output |
| ROW2 | 12 | 2 | Active-low row output |
| ROW3 | 2 | 3 | Active-low row output |
| ROW4 | 4 | 4 | Active-low row output |
| COL0 | 1 | 5 | Input pull-up |
| COL1 | 3 | 6 | Input pull-up |
| COL2 | 5 | 7 | Input pull-up |
| COL3 | 7 | 8 | Input pull-up |
| Ground | 6 | GND | Common ground |
| Backlight +5 V | 8 | Separate regulated +5 V supply | Supply only; not GPIO |
| PWM control | — | 9 | Optional external LED driver input |
| No copper | 11, 13, 14 | — | Leave unconnected |

The PCB documentation in the issue describes a 12-position connector but
numbers positions through 14, with 11/13/14 having no copper. Confirm the
physical connector numbering and orientation on the actual board.

## Matrix map

Looking at the button side with SW1 at the left:

| Row | COL0 (J1-1) | COL1 (J1-3) | COL2 (J1-5) | COL3 (J1-7) |
|---|---|---|---|---|
| ROW0 (J1-9) | SW1 | SW2 | SW3 | SW4 |
| ROW1 (J1-10) | SW5 | SW6 | SW7 | SW8 |
| ROW2 (J1-12) | SW9 | SW10 | SW11 | SW12 |
| ROW3 (J1-2) | SW13 | SW14 | SW15 | SW16 |
| ROW4 (J1-4) | SW17 | SW18 | SW19 | SW20 |

Proposed EFIS labels for the buttons are SW1 AP Engage, SW2 AP Disconnect,
SW3 Heading Hold, SW4 Altitude Hold, SW5 Approach, SW6 Nav Source, SW7/SW8
Display Brightness up/down, SW9 PFD/MFD, SW10/SW11 Pitch Trim up/down,
SW12/SW13 Roll Trim left/right, SW14 LiDAR toggle, SW15 System Menu,
SW16/SW17 Menu up/down, SW18 Menu Select, SW19 Emergency Standby, and SW20
Reserved. These are labels only; the firmware sends button IDs and states, not
the per-button CAN IDs in the proposal.

The firmware selects one row LOW, waits 50 µs, then reads the four active-low
column inputs. It scans all rows from a 10 ms hardware timer flag; GPIO and CAN
work remain in the main loop, not in an interrupt. Each button must remain in
its candidate state for 20 ms before a press or release is reported. The
unsigned elapsed-time comparison handles `millis()` rollover.

## CAN protocol used by this firmware

CAN is standard 11-bit, 500 kbit/s. Button press/release events use ID `0x600`
and an eight-byte payload:

| Byte | Meaning |
|---:|---|
| 0 | Button ID, SW1=0 through SW20=19 |
| 1 | State: 1 pressed, 0 released |
| 2–7 | Zero |

LED brightness commands use standard ID `0x620`, DLC 8, with byte 0 containing
the 0–255 brightness. Other bytes are ignored. The command ID is an explicit
firmware convention, not an existing CM5 protocol: coordinate it with the
CM5 software before integration. The suggested per-button IDs `0x601`–`0x614`
are not used, because they conflict with the specified common event frame on
`0x600`; the repository contains no EFIS CAN specification to resolve that.

The firmware reports received/transmitted frames on USB serial at 115200 baud,
counts failed CAN transmissions, and reports a receive timeout after 5 seconds.
No message authentication or aircraft-level fail-safe is implemented.

## Build and bench test

Build with PlatformIO from this directory after installing the Teensy platform
and the `FlexCAN_T4` and `WDT_T4` Arduino libraries:

```sh
pio run
```

The source also uses Arduino/Teensy core APIs and can be included in a
Teensyduino project with those two libraries installed. The PlatformIO config
does not fetch unversioned third-party libraries automatically.

1. With all power removed, verify J1 pad 6 to each LED cathode and confirm the
   row/column nets for the switch matrix with continuity mode.
2. Connect only the 3.3 V-safe row/column signals and common ground first.
   Leave the +5 V backlight supply and optional PWM driver disconnected.
3. Power the Teensy by USB, open serial at 115200 baud, and check reported
   events while pressing each switch in order.
4. Connect CAN through a suitable transceiver; verify ID, DLC, and payload at
   the CM5. Confirm the actual CM5 bitrate and command protocol first.
5. Only after the LED power path and external driver are verified, send
   brightness values on `0x620`. Enter `r` in serial to request the LED ramp
   test. The test drives pin 9 and has no effect without the external driver.
6. Test the watchdog by a controlled bench-only hang, then verify reset and
   recovery before relying on it.

This firmware cannot establish connector orientation, LED current limiting,
CAN termination, watchdog reset policy, or aircraft power compatibility. Do
not connect it to an aircraft system until those are independently reviewed.
