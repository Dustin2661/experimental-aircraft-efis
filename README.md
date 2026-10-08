# experimental-aircraft-efis
Custom experimental aircraft EFIS system with Teensy 4.1 cockpit interface, Raspberry Pi CM5 avionics master, and three remote CAN sensor modules (AHRS, Engine Monitor, Magnetometer)

## Synthetic vision layout demonstrator

A dependency-free Canvas 2D display inspired by the supplied reference: blue sky,
green/yellow/red terrain, a white horizon and pitch ladder, bank scale, fixed
aircraft symbol, translucent groundspeed/GPS-altitude tapes, vertical speed, and
a heading compass with a magenta course line. The display scales to 16:9 panels
and letterboxes on other aspect ratios.

**This is a visual prototype, not flight instrumentation.** All terrain is
procedural and fictitious; destination, course, cross-track error, distance, and
ETE are static illustration values. Colors are illustrative, not a certified
terrain warning system. Do not use this display for navigation, obstacle
avoidance, or as a primary attitude instrument. Real terrain, sensor calibration,
hardware integration, integrity monitoring, and flight validation are not
implemented.

### Run on Raspberry Pi CM5 / CM5 Nano carrier

Use Raspberry Pi OS 64-bit with a working graphical desktop, Node.js 22 or newer,
and Chromium installed. The display needs no npm packages, native add-ons, GPU
extensions, or external network services. Carrier-board display support must
already be configured in the OS.

From the repository root:

```sh
npm start
```

Open `http://127.0.0.1:8080` in Chromium. For a dedicated panel, run this in the
Pi's graphical session:

```sh
chromium --kiosk http://127.0.0.1:8080
```

Keep the server running while Chromium is open. `PORT` can override port 8080;
the server binds only to localhost unless `HOST` is explicitly set. Do not expose
it to untrusted networks. This is not a boot-service or carrier-specific driver
installer.

The initial view is level flight at 142 knots, 10,130 feet and 221° true heading.
Use **Animate demo** to preview pitch, bank and terrain motion; **Reset demo**
returns to the reference pose. Full screen is also available without kiosk mode.
The overlay is limited to 30 FPS, moving terrain targets 15 FPS, the terrain
backing store is capped at 960 pixels wide, and the overlay at 1920 pixels wide.
These are rendering budgets, not measured CM5 frame-rate guarantees.

### Telemetry integration

A local browser-side bridge can call `window.updateEFIS(sample)` or dispatch an
`efis:telemetry` custom event with `sample` as its detail. No Teensy/CAN protocol
is assumed and no hardware transport is supplied. Every sample must contain
these finite numeric fields:

| Field | Units / convention | Accepted range |
| --- | --- | --- |
| `pitch` | degrees, nose up positive | −85 to 85 |
| `roll` | degrees, right bank positive | −180 to 180 |
| `heading` | degrees true, clockwise from north | 0 to 360 (360 becomes 0) |
| `groundSpeed` | knots, groundspeed, **not indicated airspeed** | 0 to 600 |
| `altitude` | feet MSL, GPS altitude | −1,500 to 60,000 |
| `verticalSpeed` | feet/minute, climb positive | −10,000 to 10,000 |
| `east`, `north` | meters in a local demo coordinate frame | −10,000,000 to 10,000,000 |

Complete valid samples atomically replace the state and disable demo animation.
The hook returns `true` for an accepted sample, `false` for an invalid one.
Send updates more frequently than every two seconds. After two seconds without
a valid sample, the display blanks synthetic vision and all flight values and
shows a stale-data alert; a new valid sample restores it. Terrain remains
fictitious even with external telemetry. Reload to return to demo mode.

### Development checks

```sh
npm run check
npm test
```

Tests use Node's built-in runner and cover sample validation, heading wrap,
attitude projection, demo generation, and static-server isolation.
