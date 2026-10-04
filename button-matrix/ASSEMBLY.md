# Button matrix assembly and bench verification

## Design status and safety

This is a bench-prototype design package, not a flight-qualified or production
release. KiCad DRC/ERC, exact footprint-to-datasheet checks, thermal review,
panel fit, and aircraft environmental/EMI testing are required before ordering
or installing a board. Do not connect this experimental control board to
flight-critical equipment or use it to command an autopilot in flight.

The layout is a 4-row by 5-column matrix. The supplied D10 reference describes
a different two-row layout and 14-pad connector; no reference image or Gerber
was present in this repository, so the board does not claim to reproduce that
copper artwork.

The KiCad files are review aids only: the schematic is a design-note sheet and
the PCB file is a placement/mechanical mockup. They do not yet contain
component footprints, connected nets, copper routing, or verified
manufacturing outputs. Do not send these files to a PCB fabricator.

The requested 100 nF capacitors across every switch are intentionally not
used. In this diode-isolated matrix, per-switch capacitors can couple row and
column transitions; input debouncing is instead performed in firmware for
20 ms. C2/C3 are rail bypass capacitors. The function-to-CAN table is proposed
integration metadata; the Teensy firmware reports button IDs over USB serial
and does not transmit CAN or actuate autopilot functions.

## Placement and assembly

The mechanical drawing shows the 120 x 90 mm board outline, four M3 mounting
holes and the 4 x 5 button-center grid. Switches are arranged left-to-right,
top-to-bottom as buttons 1 through 20. Put each 3 mm LED beside, not beneath,
the switch plunger. Observe LED polarity, diode polarity, IC pin 1, and
electrolytic polarity.

Recommended order:

1. Inspect the bare PCB for outline, hole, connector-key and footprint errors.
2. Fit the low-profile diodes, resistors and ceramic capacitors; inspect and
   electrically check each diode's orientation.
3. Fit U1, the bulk capacitor, switches, LEDs and keyed connector. Use the
   footprint/datasheet drawings for pin-1 and polarity orientation.
4. For hand soldering, use a temperature-controlled iron and the solder
   manufacturer's profile. A typical leaded hand-soldering starting point is
   320-350 C; use the alloy's recommended temperature and minimize dwell time.
   This is not a reflow profile.
5. Clean flux if required by the flux manufacturer and inspect all joints,
   especially the fine-pitch driver pins and connector.

## Power-on and tests

1. With the board unpowered, visually inspect for bridges and reversed parts.
2. Check continuity of each row and column and verify no row/column short.
   Confirm every switch diode is oriented column-to-row.
3. Check resistance between +5V and GND before power. Do not power the board
   if it appears shorted.
4. Connect only to a current-limited 5 V supply and the Teensy at first.
   Verify +5V_LED and +3V3 logic rails and shared ground. Never apply 5 V to
   Teensy GPIO.
5. Load the Teensy firmware and run `python3 test_button_matrix.py --port
   <serial-port>`. The script lights each LED at 50% duty, then asks the
   operator to press and release each button in sequence. It cannot simulate a
   physical button press.
6. Verify every LED and button reports the expected one-based button number.
   Check simultaneous presses on separate rows/columns to check for ghosting.
7. Measure the actual +5 V current. Twenty red LEDs are nominally limited to
   roughly 100 mA total by the driver current setting; use measured component
   values and actual data sheets, not this estimate, for supply design.

## Troubleshooting

| Symptom | Checks |
|---|---|
| No serial `READY` | USB cable/port, firmware upload, baud rate 115200 |
| All buttons appear pressed | Common ground, column pull-ups, row pin assignment, shorted connector |
| One row or column fails | J1 continuity, connector orientation, switch/diode solder joints |
| Multiple-key ghosting | Diode orientation and that all 20 isolation diodes are fitted |
| LED does not light | LED polarity, series resistor, TLC5947 strap/pinout, 3.3 V logic and 5 V LED rails |
| All LEDs are dim/off | LED driver control wiring, XLAT/SCLK/SIN pin map and driver current-set resistor |
| Board resets on LED activity | 5 V supply wiring, common ground, bypass capacitors, source current limit |

## BOM and sourcing

The accompanying CSV gives designators, quantities and suggested MPNs where
known. Generic passives and LEDs must be selected by their datasheets and
availability. Obtain vendor quotations before relying on the proposed
sub-$50 target; PCB fabrication, shipping, tooling and assembly are excluded.
