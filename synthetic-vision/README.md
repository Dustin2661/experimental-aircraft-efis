# Synthetic vision display prototype

This is a standalone browser-rendered layout prototype based on the supplied
EFIS reference image. The repository has no CM5 display application or AHRS
feed, so the page renders illustrative terrain and default values rather than
live aircraft data. Do not use it for navigation or flight.

Open `index.html` in a modern browser. A host application can provide fresh
instrument values by calling `window.updateSyntheticVision(...)`, for example:

```js
window.updateSyntheticVision({
  pitch: 2,
  roll: -5,
  airspeed: 142,
  altitude: 10130,
  heading: 284,
  track: 278,
  verticalSpeed: 250,
  groundSpeed: 65,
  gpsAltitude: 10100,
  waypoint: "KSNY",
  distance: 34,
  crossTrack: 0.2
});
```

The update function clamps numeric values to displayable ranges. `pitch` and
`roll` are degrees; speed is knots; altitude and vertical speed are feet and
feet per minute. `heading` and `track` are degrees. This prototype supplies
only the visual layout; it does not implement sensor parsing, terrain
databases, or aircraft-system integration.
