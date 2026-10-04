# experimental-aircraft-efis

Custom experimental aircraft EFIS system with Teensy 4.1 cockpit interface,
Raspberry Pi CM5 avionics master, and three remote CAN sensor modules (AHRS,
Engine Monitor, Magnetometer).

## 20-button matrix prototype

The [`button-matrix/`](button-matrix/) directory contains the proposed 4x5
button matrix, Teensy firmware core and serial bench-test program, connector
pin map, BOM, assembly notes, function map and mechanical drawing.

**The supplied KiCad schematic and PCB are review/placement mockups, not a
manufacturing release.** They have no placed electrical footprints, netlist or
routed copper, and have not passed KiCad ERC/DRC. Do not fabricate or install
them. Verify component datasheets, current limits, panel dimensions, and
electrical integration before constructing a board. This prototype is not
flight-qualified and must not command flight-critical equipment.
