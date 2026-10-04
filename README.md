# experimental-aircraft-efis
Custom experimental aircraft EFIS system with Teensy 4.1 cockpit interface, Raspberry Pi CM5 avionics master, and three remote CAN sensor modules (AHRS, Engine Monitor, Magnetometer)

## SL70R bench firmware

**Not flight-ready.** The initial Teensy 4.1 firmware implements SL70R mode/status
and replies-per-second reception, guarded command generation, and USB status relay
to CM5. It does not implement the other radios, ADS-B, CAN sensors, a button matrix,
altitude input, or an independent cockpit display.

Open `/home/runner/work/experimental-aircraft-efis/experimental-aircraft-efis/firmware/sl70r/sl70r.ino`
in Arduino IDE with Teensyduino, select Teensy 4.1 and a USB Serial configuration,
and compile/upload. The driver has no third-party library dependencies.
Hardware compilation and aircraft operation require separate validation.

### Wiring and independence

- SL70R: dedicated `Serial3`, **9600 baud, 8N1**; default Teensy RX pin 15,
  TX pin 14. Use a suitable 3.3 V-compatible RS-232 transceiver; never connect
  RS-232 voltages directly to Teensy GPIO. Verify SL70R connector pins against
  the installation wiring diagram; no connector pin numbers are assumed here.
- CM5: Teensy USB Serial. USB baud selection does not change SL70R baud.
  The sketch never waits for USB connection and keeps servicing SL70R when CM5
  disconnects. No cached CM5 commands are replayed on reconnect.
- Aircraft power, grounding, circuit protection, unpowered-interface behavior,
  and the independent pressure-altitude source must be engineered separately.
  Teensy independence requires its power not to depend on CM5's USB supply.

### Bench interface

Every second, the sketch attempts to publish one JSON line over USB containing
reported mode, squawk, IDENT, heartbeat bit, reply rate, freshness, pending-command,
timeout, emergency-latch, and parser-error information. USB output is skipped
when disconnected or when its transmit buffer lacks room; it never waits for CM5.
Fields are cached: only use mode/squawk/IDENT when `received` and `fresh` are true,
and reply rate when `replies_fresh` is true. Initial `O`/`0000` fields are
placeholders, not measurements. Heartbeat is a reported bit, not an independently
verified link-health guarantee.

Transmission is **disabled by default**. For an isolated, authorized bench test
only, define `SL70R_ENABLE_BENCH_TX=1` when compiling, after checking the checksum
assumption below. USB accepts exact newline-terminated requests:

- `SQUAWK 1200`: change code while preserving the last reported mode.
- `MODE O`, `MODE A`, or `MODE C`: change mode while preserving reported code.
- `IDENT`: one request, rejected in standby or while reported IDENT is active.
- `EMERGENCY`: request 7700 while preserving mode and latch code protection.
- `RELEASE 1200`: explicitly leave the emergency latch for a normal code.

Normal squawk/release requests reject 7500, 7600, and 7700; only `EMERGENCY`
requests 7700. No special command for 7500/7600 is implemented. Reported 7700
also sets the latch. It blocks ordinary squawk changes, not explicit mode control.
Release clears the latch only after matching reported mode/code arrives.
The latch is RAM-only and does not survive Teensy reset.

Commands require fresh mode status and an idle command slot. A transmitted packet
is reported as `sent_unconfirmed`; a later matching status ends the pending state.
IDENT additionally requires reported IDENT active. This is observed-state
confirmation, not a transaction acknowledgement or proof that ATC received IDENT.
No commands, IDENT pulses, resets, or heartbeat packets are automatically retried.
There is no automatic emergency activation, startup mode change, or squawk restore.

Status and command timeouts are **host design thresholds of 3500 ms**, not SL70R
manufacturer limits. Partial serial frames expire after 150 ms; partial USB
requests expire after 1000 ms. Stale data remains visible with freshness false.
Lost USB does not imply lost SL70R communication. Without fresh mode status,
commands are rejected rather than constructing a command using guessed settings.
No independent buttons or LEDs are wired by this sketch; USB commands are bench
inputs, not the final cockpit emergency interface.

### Protocol evidence and unresolved assumptions

Reference: Apollo SL70R Installation Manual, 560-0408-01a, Section 7,
as transcribed in this task from ManualsLib:
https://www.manualslib.com/manual/2059847/Ups-Aviation-Technologies-Apollo-Sl70r.html

- Printed p. 29 / PDF p. 37, Section 7.2: fixed 9600 8N1, broadcast operation,
  independent host/transponder startup order.
- Printed p. 31 / PDF p. 39, Tables 7-3 through 7-5: 17-byte `^MD` status,
  modes O/A/C, IDENT I/-, ASCII code, hex flags with heartbeat at bit 0.
- Printed p. 32 / PDF p. 40, Table 7-6: 11-byte `^RC`, replies per second.
- Printed pp. 35–36 / PDF pp. 43–44, Tables 7-12 through 7-14:
  15-byte `#MD` command, checksum covering bytes 0–11. Commands are ignored
  in setup/test or system-failure states.

The checksum implementation is an **inferred sum modulo 256**, consistent with
the supplied `#MD O,-,12006B`, `#MD A,I,235484`, `^MD O,-,12000006`,
`^MD A,I,23540120`, and `^RC 1200D6` examples. It is not an authenticated OEM
algorithm. Reception rejects packets inconsistent with this assumption; that
can make a real device appear stale if the assumption is wrong.

EXT message selection, heartbeat transmission/timing, repeated IDENT behavior,
and full system-status/self-test handling remain unverified and unimplemented.
Mode/reply reception depends on EXT output being configured externally.
Mode C selection alone does not establish valid altitude reporting.

The user-supplied assumption is that host loss for 60 seconds causes Mode C with
the last squawk, and a power cycle in that state falls back to 1200. This has
**not been independently verified and is not implemented or relied upon**.
The firmware never simulates the transponder's state after a timeout. Confirm
the actual unit's behavior, altitude-source dependencies, and recovery on a
properly equipped bench before aircraft use.

### Tests

From `/home/runner/work/experimental-aircraft-efis/experimental-aircraft-efis`,
run `make test` with a C++11 compiler. The native tests exercise manual examples,
all 4096 squawk codes, invalid data, overflow/resynchronization, stale data,
command confirmation/timeouts, IDENT gating, emergency protection, and timer wrap.
These tests do not verify electrical behavior, RF operation, USB hardware,
or Teensy compilation.
