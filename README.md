# experimental-aircraft-efis
Custom experimental aircraft EFIS system with Teensy 4.1 cockpit interface, Raspberry Pi CM5 avionics master, and three remote CAN sensor modules (AHRS, Engine Monitor, Magnetometer)

## D10 button matrix bench setup

Start with the [Arduino IDE + Teensy 4.1 step-by-step guide](TEENSY_FIRMWARE.md)
for screenshots, continuity-checked wiring references, upload instructions, and
optional CAN examples. The complete [Arduino sketch and button assignment header](firmware/d10_efis_button_control)
provide serial press/release/hold diagnostics and LED PWM bench commands.
This is a bench demonstration, not flight-ready control firmware.
