export const DEG = Math.PI / 180;
export const METERS_PER_FOOT = 0.3048;
export const STALE_AFTER_MS = 2000;

export const INITIAL_FLIGHT = Object.freeze({
  pitch: 0,
  roll: 0,
  heading: 221,
  groundSpeed: 142,
  altitude: 10130,
  verticalSpeed: 0,
  east: 0,
  north: 0,
});

const limits = {
  pitch: [-85, 85],
  roll: [-180, 180],
  heading: [0, 360],
  groundSpeed: [0, 600],
  altitude: [-1500, 60000],
  verticalSpeed: [-10000, 10000],
  east: [-10000000, 10000000],
  north: [-10000000, 10000000],
};

// A complete sample prevents old and new attitude channels being mixed.
export function validateFlight(sample) {
  if (!sample || typeof sample !== 'object') return null;
  const flight = {};
  for (const [key, [minimum, maximum]] of Object.entries(limits)) {
    const value = sample[key];
    if (typeof value !== 'number' || !Number.isFinite(value) || value < minimum || value > maximum) return null;
    flight[key] = key === 'heading' ? value % 360 : value;
  }
  return flight;
}

export function signedAngle(angle) {
  return ((angle + 180) % 360 + 360) % 360 - 180;
}

export function projectPoint(right, forward, up, pitch, roll, focal, cx, cy) {
  const p = pitch * DEG;
  const r = roll * DEG;
  const depth = forward * Math.cos(p) + up * Math.sin(p);
  if (depth < 25) return null;
  const x = right * focal / depth;
  const y = (forward * Math.sin(p) - up * Math.cos(p)) * focal / depth;
  return {
    x: cx + x * Math.cos(r) + y * Math.sin(r),
    y: cy - x * Math.sin(r) + y * Math.cos(r),
  };
}

// Deliberately procedural, not a terrain database or obstacle clearance model.
export function terrainHeight(east, north) {
  return 2100
    + 520 * Math.sin(east / 5400) * Math.cos(north / 7200)
    + 320 * Math.sin((east + north) / 3100)
    + 160 * Math.cos(east / 1500 + Math.sin(north / 2600))
    + 760 * Math.exp(-(((north + 26000) / 10500) ** 2));
}

export function demoFlight(seconds) {
  return {
    ...INITIAL_FLIGHT,
    pitch: 3 * Math.sin(seconds / 7),
    roll: 12 * Math.sin(seconds / 9),
    heading: (221 + 9 * Math.sin(seconds / 18)) % 360,
    altitude: 10130 + 100 * Math.sin(seconds / 12),
    groundSpeed: 142 + 3 * Math.sin(seconds / 8),
    verticalSpeed: 500 * Math.cos(seconds / 12),
    north: -seconds * 72,
  };
}
