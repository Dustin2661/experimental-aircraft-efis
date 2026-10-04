# D10 finalized 20-button interface

## Status and limits

This is a bench-development interface, **not flight-ready avionics**. This branch
previously contained only a README. The firmware supplies matrix scanning, button
requests over CAN, and PWM control; it does not supply the CM5 UI, autopilot
interlocks, a V16 radio driver, or a verified carrier recovery circuit.
The schematic, assembled PCB, radio protocol, and aircraft installation must be
reviewed and tested before use. The accompanying `teensy-d10-wiring.pdf` is a
conceptual wiring diagram, **not an assembly-approved schematic**.

## Final front-panel mapping

Front/button view: SW1 is far left, SW20 far right, in one physical line.
Button ID = SW number minus one; row = ID / 4; column = ID % 4.

| SW | ID | Label | CAN ID | Request / receiver responsibility |
|---|---|---|---|---|
| 1 | 0 | LIDAR | 0x601 | Toggle acquisition and synthetic-vision/PFD overlay |
| 2 | 1 | AUTOPILOT | 0x602 | Open controls; require confirmation and valid system status before engagement |
| 3 | 2 | EMERG COM | 0x603 | Request independent Teensy backup COM control; see integration boundary below |
| 4 | 3 | CM5 BOOT | 0x604 | Ground-only USB recovery/flash request; not a soft reboot or power command |
| 5 | 4 | MAP | 0x605 | Moving map with GPS position/AHRS heading; optional LiDAR overlay |
| 6 | 5 | COM | 0x606 | Edit COM standby digits; select active/standby |
| 7 | 6 | NAV | 0x607 | Edit NAV standby digits; select active/standby |
| 8 | 7 | ENGINE | 0x608 | Engine-monitor CAN data: RPM, temperatures, pressures |
| 9 | 8 | TRAFFIC | 0x609 | FreeFlight ADS-B IN targets, range, bearing |
| 10 | 9 | PFD | 0x60A | Return to default attitude/altitude/airspeed display; always accessible |
| 11 | 10 | DIRECT TO | 0x60B | Airport, waypoint or lat/lon destination; flight-plan/autopilot integration |
| 12 | 11 | FLIGHT PLAN | 0x60C | Edit/load plans, legs, ETA; activate leg |
| 13 | 12 | AIRPORT | 0x60D | SD airport database: diagram, runways, FBO, frequencies |
| 14 | 13 | COM SWAP | 0x60E | Exchange COM active/standby |
| 15 | 14 | NAV SWAP | 0x60F | Exchange NAV active/standby |
| 16 | 15 | XPDR | 0x610 | Garmin SL70R RS-232 code, mode, ident controls |
| 17 | 16 | BARO | 0x611 | QNH/elevation-reference adjustment and comparison |
| 18 | 17 | ALERT ACK | 0x612 | Suppress annunciation only; retain underlying fault and fault log |
| 19 | 18 | BACK | 0x613 | Cancel pending edits or return to previous page |
| 20 | 19 | MENU / DIM | 0x614 | Short press: settings; hold strictly over 500 ms: brightness; release confirms |

These messages are requests, not permission to actuate safety-critical systems.
For example SW2 never directly engages an autopilot and SW4 never pulses an
unverified GPIO. PFD access and alert persistence must be enforced by the receiver.

## Matrix and CAN wiring

User-supplied REV19 FINAL R3 mapping, **not independently continuity-verified**:

| Signal | J1 rear position | Teensy 4.1 GPIO | Switches |
|---|---|---|---|
| ROW0 | 9 | 0 | SW1–SW4 |
| ROW1 | 10 | 1 | SW5–SW8 |
| ROW2 | 12 | 2 | SW9–SW12 |
| ROW3 | 2 | 3 | SW13–SW16 |
| ROW4 | 4 | 4 | SW17–SW20 |
| COL0 | 1 | 5 | SW1, SW5, SW9, SW13, SW17 |
| COL1 | 3 | 6 | SW2, SW6, SW10, SW14, SW18 |
| COL2 | 5 | 7 | SW3, SW7, SW11, SW15, SW19 |
| COL3 | 7 | 8 | SW4, SW8, SW12, SW16, SW20 |
| LED ground | 6 | GND | All switch LED pad 5 cathodes |
| LED positive | 8 | **No direct GPIO connection** | Resistor-fed LED pad 6 anodes |

Rear view with J1 on the right: position 1 lower-right, position 2 above it;
numbering increases leftward. Positions 11, 13 and 14 have no copper pad.
Do not renumber around missing pads; check fitted connector alignment.
Switch pads 3/4 connect to row; pads 1/2 connect through the series diode to
column. These are duplicated terminals of one switch, not independent switches.
Verify diode polarity supports active-low row scanning before powering.
Only the selected row drives LOW; inactive rows are high-impedance, columns use
3.3 V pull-ups. Scan every 5 ms, allow 50 us settling, debounce each edge for 20 ms.
No blocking delay is used for holds; elapsed-time arithmetic handles millis rollover.

CAN1 uses Teensy TX pin 22 and RX pin 23, through a **3.3 V-compatible CAN
transceiver**, not directly to CANH/CANL. Use common reference ground and correct
bus-end termination. This project defaults to 500 kbit/s; verify against the
actual EFIS bus.

## Safe backlight control

**Teensy 4.1 GPIO is not 5 V tolerant. Never connect GPIO9 to J1-8.**
Because the cathodes are fixed to J1-6 ground, use a rated **high-side** PWM
switch or backlight driver with a documented 3.3 V-compatible control input:

```text
Protected/current-limited +5 V --> high-side PWM driver --> J1-8 LED positive
Teensy GPIO9 (3.3 V PWM) -------> driver PWM input (default OFF pull-down)
Teensy GND --------------------> driver GND -----------> J1-6 LED cathodes
```

The firmware assumes active-high control, 8-bit duty cycle (0–255), 500 Hz.
It starts at 128. Keep GPIO9 disconnected from the power node until driver
polarity, current/inrush rating, PWM frequency suitability and current limiting
are verified. A 3.3 V-compatible level shifter alone is not necessarily a
load-rated power switch.

**Do not build the proposed N-MOS drain-to-anode/source-to-ground circuit.**
It shunts the +5 V node toward ground instead of switching the supply in series.
Do not tie +5 V to a gate connected to a Teensy GPIO. A discrete high-side P-MOS
requires a separate level-shifting transistor and gate pull-up to its source;
do not drive that gate directly with 3.3 V GPIO.
A TLP521 alone does not provide LED current regulation or a rated high-side driver.
A blanket 100 ohm resistor is not validated current limiting for 20 parallel LEDs.
Verify each LED's existing series resistor, total load, supply protection and
thermal limits; size any added components from measured load and datasheets.

## CAN contract

Standard 11-bit data frames, DLC 8. Function ID is 0x601 + button ID.
Payload: `[button_id, event, 0, 0, 0, 0, 0, 0]`.

- Event 1: debounced press.
- Event 2: one-shot MENU/DIM hold strictly over 500 ms after debounced press.
- Event 0: debounced release.
- 0x600 is reserved as the generic button-event ID; no duplicate frame is sent.

All non-MENU functions respond only to event 1. Do not perform swap/toggle
actions again on release. For SW20 the receiver must defer opening settings:
record press, enter dim mode on event 2, then on event 0 confirm brightness if
held or open settings if not held. A release without a matching press must not
open settings. Brightness adjustment comes from the UI/encoder integration,
which is not included here. CAN 0x620, DLC 8, supplies duty cycle in byte 0
(bytes 1–7 reserved zero); the Teensy updates PWM independently of scan timing.
Extended and remote frames are ignored. The uint8_t API already bounds duty
cycle, so no ineffective `brightness > 255` check is used.
Check `d10_can_tx_error_count()` for rejected transmissions; there is no delivery
guarantee, retry queue or safety certification. A valid driver initialization
does not prove a connected bus or an available CM5.

## CM5-NANO-B recovery interface: ground maintenance only

SW4 sends 0x604 as a maintenance request. It **does not control power, BOOT,
RUN, reset or reboot**. A 100 ms pulse on a running CM5 does not establish USB
recovery: BOOT must be held in the required state during power-on.

Until the exact carrier schematic/revision is reviewed, the only documented
operating path is the carrier's manual BOOT procedure: on the ground, remove
power, hold the carrier BOOT control as instructed by Waveshare, attach the
USB slave/flashing connection, and apply power following the vendor procedure.
Use Raspberry Pi `rpiboot`/imaging tools; a flash request itself does not flash
storage or change EEPROM.

An eventual electronic interface may use isolated relay **contacts** across the
verified BOOT switch/header, or an open-drain transistor to the verified BOOT net.
Relay coils require a rated driver/flyback protection, never direct GPIO drive.
Do not assign a Teensy GPIO, pin voltage or pulse timing without schematic review.
Verify BOOT pull-up voltage, reset/power sequence, back-power paths and default
released state; require a physical ground-maintenance interlock. The schematic
was **not available for inspection in this task**, so assembly approval remains
blocked. No guessed recovery wiring is enabled in firmware.

## Emergency COM: independent path and integration specification

**SW3 currently emits a request only. It is not a working backup radio.**
A CAN message to a failed CM5 cannot implement emergency COM. The future
Teensy radio service must receive the local SW3 press *before* CAN transmission
and own the radio without dependence on CM5 rendering, CAN acknowledgements or
CM5 power. This service is a separate design, not invented protocol bytes here.

```text
Local SW3 / independent knobs --> Teensy radio state machine
Teensy Serial5 TX20/RX21 <-----> 3.3 V RS-232 transceiver <-----> MGL V16
Independent frequency display <--> Teensy (separate interface, not yet selected)
Teensy -- CAN status/logging --> CM5 (optional observer, never control prerequisite)
```

Pins 20/21 are proposed UART reservations, not initialized by this firmware.
RS-232 bipolar signals must never reach Teensy GPIO; use a suitable transceiver
and the manufacturer's verified radio connector, signal ground and wiring.
Confirm Serial5 pin selection against PJRC documentation before assembly.

Required driver contract:

- Read/validate last confirmed active and standby frequencies from local storage.
- On SW3, enter autonomous mode only when valid state, radio transport and a local
  frequency display are available; visibly report failures without pretending
  the radio accepted a command. A second SW3 requests exit.
- Local knobs/adjacent-button assignment remains to be designed. COM EDIT and COM
  SWAP should dispatch locally in emergency mode rather than requiring CM5.
- Serialize ownership: reject normal CM5 tuning while autonomous; resume only via
  an explicitly designed/validated handoff command. No resume CAN ID is allocated
  here. Reject stale or malformed commands and retain state on CM5 loss.
- Only publish/log confirmed radio state; bound serial timeouts and keep matrix
  scanning responsive. Report emergency state separately from a button event.

Before implementation, review the V16 installation manual and ICD for the exact
firmware revision: connector pins, baud, framing, units/channel spacing, checksums,
commands, responses, acknowledgements, retries and timeout behavior. These values
are **unverified**; no direct RS-232 commands are transmitted by this project.

Proposed storage contract (not implemented): two alternating 32-byte records at
EEPROM offsets 0 and 32. Explicit little-endian serialization, not raw structs:
bytes 0–3 magic `D10C`, 4–5 version, 6–7 record length, 8–11 sequence,
12–15 active frequency Hz, 16–19 standby frequency Hz, 20–23 channel-spacing
descriptor, 24–27 reserved zero, 28–31 CRC-32/ISO-HDLC of bytes 0–27.
Validate version, CRC and frequencies against the reviewed radio channel plan;
select the newest valid sequence using rollover-safe ordering. Write the inactive
slot only after radio acknowledgement and rate-limit writes for EEPROM endurance.
Corruption or no valid record must show a local error and prohibit automatic tuning,
not choose a guessed frequency. Reserve these offsets only after checking other
EEPROM users; power-loss and corruption testing are mandatory.

## Build and focused tests

From any working directory (paths below match this checkout):

```sh
python -m platformio run --project-dir /home/runner/work/experimental-aircraft-efis/experimental-aircraft-efis/button-matrix/teensy-firmware
gcc -std=c11 -Wall -Wextra -Werror -pedantic -fsanitize=address,undefined \
  /home/runner/work/experimental-aircraft-efis/experimental-aircraft-efis/tests/test_d10.c \
  /home/runner/work/experimental-aircraft-efis/experimental-aircraft-efis/button-matrix/teensy-firmware/d10_button_matrix.c \
  /home/runner/work/experimental-aircraft-efis/experimental-aircraft-efis/button-matrix/teensy-firmware/d10_can_driver.c \
  -o /tmp/test_d10
/tmp/test_d10
```

The Teensy Arduino framework supplies FlexCAN_T4; no separate radio library is
assumed. Build artifacts stay in ignored `.pio/` directories.
Host tests cover all mappings, frame payloads, bounce, simultaneous inputs,
hold/release boundaries, timer rollover, invalid IDs, CAN failures and PWM bounds.
The target build was attempted but platform download failed with HTTPClientError;
**target compilation and hardware behavior remain unverified**.

Bench acceptance before aircraft assembly:

1. Disconnect all power; verify every J1 row/column and LED polarity/continuity.
2. Verify the high-side driver independently with a current-limited supply, then
   measure GPIO9 never exceeding 3.3 V; check PWM 0/128/255 and driver OFF default.
3. Capture each switch's unique CAN ID, DLC and press/release; test bounce and
   simultaneous presses. Hold SW20 for 500 ms (no hold), then longer (one hold).
4. Verify receiver confirmation for AP, fault retention for ACK, always-available
   PFD, and release-to-confirm dim behavior.
5. Complete carrier schematic review and test manual recovery on the ground.
6. Implement/verify the V16 driver and independent display, then remove CM5 power
   and test local tuning, storage corruption/power loss, ownership and recovery.

## Manufacturer references (review before construction)

- PJRC voltage/pinout: https://www.pjrc.com/store/teensy41.html
- Waveshare carrier instructions/schematic resources: https://www.waveshare.com/wiki/CM5-NANO-B
- Raspberry Pi USB provisioning: https://github.com/raspberrypi/usbboot
- MGL radio and ICD: https://mglavionics.co.za/V16.html
  and https://www.mglavionics.co.za/Docs/V16%20ICD%20V4.pdf

The carrier schematic and V16 ICD could not be retrieved for verification.
