import {
  DEG, METERS_PER_FOOT, STALE_AFTER_MS, INITIAL_FLIGHT,
  validateFlight, signedAngle, projectPoint, terrainHeight, demoFlight,
} from './flight.js';

const canvas = document.querySelector('#vision');
const ctx = canvas.getContext('2d', { alpha: false });
const terrainCanvas = document.createElement('canvas');
const terrainCtx = terrainCanvas.getContext('2d', { alpha: false });
const status = document.querySelector('#source-status');
const alert = document.querySelector('#annunciation');
const motion = document.querySelector('#motion');
let flight = { ...INITIAL_FLIGHT };
let external = false;
let receivedAt = 0;
let animated = false;
let demoStarted = 0;
let lastFrame = -Infinity;
let lastTerrain = -Infinity;
let terrainDirty = true;
let width = 1280;
let height = 720;
let scale = 1;

function resize() {
  const bounds = canvas.getBoundingClientRect();
  // Cap the backing store: CM5 performance must not depend on a 4K panel's DPR.
  const ratio = Math.min(window.devicePixelRatio || 1, 1920 / bounds.width);
  canvas.width = Math.max(1, Math.round(bounds.width * ratio));
  canvas.height = Math.max(1, Math.round(bounds.height * ratio));
  width = canvas.width;
  height = canvas.height;
  scale = width / 1280;
  terrainCanvas.width = Math.min(960, width);
  terrainCanvas.height = Math.round(terrainCanvas.width * height / width);
  terrainDirty = true;
}
new ResizeObserver(resize).observe(canvas);
resize();

function line(x1, y1, x2, y2, color = '#fff', weight = 1.5) {
  ctx.strokeStyle = color;
  ctx.lineWidth = weight;
  ctx.beginPath();
  ctx.moveTo(x1, y1);
  ctx.lineTo(x2, y2);
  ctx.stroke();
}

function text(value, x, y, size = 17, color = '#fff', align = 'center') {
  ctx.font = `500 ${size}px Arial, sans-serif`;
  ctx.textAlign = align;
  ctx.textBaseline = 'middle';
  ctx.fillStyle = color;
  ctx.fillText(value, x, y);
}

function terrainColor(elevation, distance, shade) {
  const clearance = flight.altitude * METERS_PER_FOOT - elevation;
  let rgb = clearance < 100 ? [179, 34, 24] : clearance < 330 ? [188, 169, 28] : [88, 125, 34];
  const haze = Math.min(.48, distance / 140000);
  rgb = rgb.map((channel, i) => Math.round(channel * shade * (1 - haze) + [104, 139, 160][i] * haze));
  return `rgb(${rgb.join(',')})`;
}

function drawTerrain() {
  const w = terrainCanvas.width;
  const h = terrainCanvas.height;
  const focal = w * .82;
  const cx = w / 2;
  const cy = h * .43;
  const sky = terrainCtx.createLinearGradient(0, 0, 0, h);
  sky.addColorStop(0, '#184375');
  sky.addColorStop(.55, '#377cb8');
  sky.addColorStop(1, '#a2c8d3');
  terrainCtx.fillStyle = sky;
  terrainCtx.fillRect(0, 0, w, h);

  // Fill the near-ground half plane; the mesh supplies relief above it.
  terrainCtx.save();
  terrainCtx.translate(cx, cy);
  terrainCtx.rotate(-flight.roll * DEG);
  const horizon = focal * Math.tan(flight.pitch * DEG);
  terrainCtx.fillStyle = '#456524';
  terrainCtx.fillRect(-w * 8, horizon, w * 16, h * 20);
  terrainCtx.restore();

  const columns = 44;
  const rows = 38;
  const heading = flight.heading * DEG;
  const sin = Math.sin(heading);
  const cos = Math.cos(heading);
  const altitude = flight.altitude * METERS_PER_FOOT;
  const grid = [];
  for (let row = 0; row <= rows; row++) {
    const distance = 80 * (75000 / 80) ** (row / rows);
    const points = [];
    for (let col = 0; col <= columns; col++) {
      const right = (col / columns * 2 - 1) * distance * 2.4;
      const east = flight.east + right * cos + distance * sin;
      const north = flight.north - right * sin + distance * cos;
      const elevation = terrainHeight(east, north);
      points.push({
        point: projectPoint(right, distance, elevation - altitude, flight.pitch, flight.roll, focal, cx, cy),
        elevation,
        distance,
      });
    }
    grid.push(points);
  }
  // Far-to-near painter's order avoids a depth buffer and GPU dependencies.
  for (let row = rows - 1; row >= 0; row--) {
    for (let col = 0; col < columns; col++) {
      const a = grid[row][col];
      const b = grid[row][col + 1];
      const c = grid[row + 1][col];
      const d = grid[row + 1][col + 1];
      triangle(a, c, b);
      triangle(b, c, d);
    }
  }

  function triangle(a, b, c) {
    if (!a.point || !b.point || !c.point) return;
    const points = [a.point, b.point, c.point];
    if (points.every(p => p.x < 0) || points.every(p => p.x > w)
      || points.every(p => p.y < 0) || points.every(p => p.y > h)) return;
    const elevation = (a.elevation + b.elevation + c.elevation) / 3;
    const shade = Math.max(.55, Math.min(1.35, 1 + (b.elevation - a.elevation) / Math.max(600, b.distance - a.distance)));
    terrainCtx.fillStyle = terrainColor(elevation, b.distance, shade);
    terrainCtx.beginPath();
    terrainCtx.moveTo(a.point.x, a.point.y);
    terrainCtx.lineTo(b.point.x, b.point.y);
    terrainCtx.lineTo(c.point.x, c.point.y);
    terrainCtx.closePath();
    terrainCtx.fill();
  }
}

function drawAttitude() {
  const cx = 640;
  const cy = 720 * .43;
  const focal = 1280 * .82;
  ctx.save();
  ctx.translate(cx, cy);
  ctx.rotate(-flight.roll * DEG);
  const horizon = focal * Math.tan(flight.pitch * DEG);
  line(-1500, horizon, 1500, horizon, '#fff6cf', 2);
  for (let angle = -30; angle <= 30; angle += 5) {
    if (!angle || Math.abs(angle - flight.pitch) > 20) continue;
    const y = focal * Math.tan((flight.pitch - angle) * DEG);
    const length = angle % 10 === 0 ? 68 : 36;
    ctx.setLineDash(angle < 0 ? [7, 5] : []);
    line(-length, y, -14, y, '#ffffffcc');
    line(14, y, length, y, '#ffffffcc');
    ctx.setLineDash([]);
    text(Math.abs(angle), -length - 18, y, 16);
    text(Math.abs(angle), length + 18, y, 16);
  }
  ctx.restore();

  // Bank scale and fixed aircraft symbol stay independent of terrain pitch.
  const bankY = 270;
  const radius = 156;
  ctx.save();
  ctx.translate(cx, bankY);
  ctx.strokeStyle = '#ffffffd9';
  ctx.lineWidth = 1.5;
  ctx.beginPath();
  ctx.arc(0, 0, radius, Math.PI * 7 / 6, Math.PI * 11 / 6);
  ctx.stroke();
  for (const degrees of [-60, -45, -30, -20, -10, 0, 10, 20, 30, 45, 60]) {
    const a = (degrees - 90) * DEG;
    const long = degrees % 30 === 0;
    line(Math.cos(a) * radius, Math.sin(a) * radius,
      Math.cos(a) * (radius + (long ? 14 : 7)), Math.sin(a) * (radius + (long ? 14 : 7)));
  }
  ctx.rotate(-flight.roll * DEG);
  ctx.fillStyle = '#fff';
  ctx.beginPath();
  ctx.moveTo(0, -radius + 6);
  ctx.lineTo(-7, -radius + 18);
  ctx.lineTo(7, -radius + 18);
  ctx.fill();
  ctx.restore();
  line(cx - 96, cy, cx - 28, cy, '#ffe764', 3);
  line(cx + 28, cy, cx + 96, cy, '#ffe764', 3);
  line(cx - 28, cy, cx - 17, cy + 9, '#ffe764', 3);
  line(cx + 28, cy, cx + 17, cy + 9, '#ffe764', 3);
  ctx.strokeStyle = '#ffe764';
  ctx.beginPath();
  ctx.arc(cx, cy, 4, 0, Math.PI * 2);
  ctx.stroke();
}

function drawTape(x, value, step, spacing, title, formatter, side) {
  const top = 154;
  const bottom = 467;
  const center = 720 * .43;
  const tapeWidth = 82;
  ctx.fillStyle = '#102d4b78';
  ctx.fillRect(x - tapeWidth / 2, top, tapeWidth, bottom - top);
  ctx.save();
  ctx.beginPath();
  ctx.rect(x - tapeWidth / 2, top, tapeWidth, bottom - top);
  ctx.clip();
  const first = Math.floor(value / step) * step;
  for (let tick = first - step * 8; tick <= first + step * 8; tick += step) {
    if (tick < 0 && title === 'GS KTS') continue;
    const y = center - (tick - value) / step * spacing;
    if (y < top - 20 || y > bottom + 20) continue;
    text(formatter(tick), x, y, 18);
    const edge = x + side * tapeWidth / 2;
    line(edge - side * 9, y, edge, y, '#ffffffa0', 1);
    line(edge - side * 5, y + spacing / 2, edge, y + spacing / 2, '#ffffffa0', 1);
  }
  ctx.restore();
  ctx.fillStyle = '#080e16';
  ctx.strokeStyle = '#ffffffcc';
  ctx.lineWidth = 1.5;
  ctx.beginPath();
  ctx.moveTo(x - 43, center - 21);
  ctx.lineTo(x + 43, center - 21);
  ctx.lineTo(x + 43, center - 8);
  ctx.lineTo(x + 43 + (side > 0 ? 9 : 0), center);
  ctx.lineTo(x + 43, center + 8);
  ctx.lineTo(x + 43, center + 21);
  ctx.lineTo(x - 43, center + 21);
  ctx.lineTo(x - 43, center + 8);
  ctx.lineTo(x - 43 - (side < 0 ? 9 : 0), center);
  ctx.lineTo(x - 43, center - 8);
  ctx.closePath();
  ctx.fill();
  ctx.stroke();
  text(Math.round(value), x, center + 1, 25);
  text(title, x, bottom + 19, 15);
}

function drawVerticalSpeed() {
  const x = 973;
  const cy = 720 * .43;
  line(x, 192, x, 426, '#ffffff77', 1);
  for (let tick = -2; tick <= 2; tick++) {
    const y = cy - tick * 50;
    line(x - 5, y, x + 4, y, '#ffffffcc', 1);
    if (tick) text(Math.abs(tick), x + 13, y, 14);
  }
  const y = cy - Math.max(-2, Math.min(2, flight.verticalSpeed / 1000)) * 50;
  line(x - 9, y, x + 7, y, '#ee73ee', 3);
  ctx.fillStyle = '#102136d9';
  ctx.fillRect(x + 6, cy - 12, 42, 24);
  text((flight.verticalSpeed / 1000).toFixed(1), x + 27, cy, 13);
  text('VS ×1000', x + 8, 463, 12);
}

function drawCompass() {
  const cx = 640;
  const cy = 657;
  const radius = 153;
  ctx.save();
  ctx.beginPath();
  ctx.rect(350, 492, 580, 228);
  ctx.clip();
  ctx.translate(cx, cy);
  ctx.fillStyle = '#18332b33';
  ctx.beginPath();
  ctx.arc(0, 0, radius, 0, Math.PI * 2);
  ctx.fill();
  ctx.strokeStyle = '#ffffff40';
  ctx.stroke();
  for (let tick = 0; tick < 360; tick += 5) {
    const angle = signedAngle(tick - flight.heading) * DEG;
    const sin = Math.sin(angle);
    const cos = Math.cos(angle);
    const major = tick % 30 === 0;
    line(sin * radius, -cos * radius, sin * (radius - (major ? 16 : 8)), -cos * (radius - (major ? 16 : 8)), '#ffffffd9', 1.5);
    if (major) {
      ctx.save();
      ctx.translate(sin * (radius - 30), -cos * (radius - 30));
      ctx.rotate(angle);
      text(({ 0: 'N', 90: 'E', 180: 'S', 270: 'W' })[tick] || tick / 10, 0, 0, 19);
      ctx.restore();
    }
  }
  ctx.rotate(signedAngle(234 - flight.heading) * DEG);
  line(0, -133, 0, 76, '#f365e9', 3);
  ctx.fillStyle = '#f365e9';
  ctx.beginPath();
  ctx.moveTo(0, -145);
  ctx.lineTo(-7, -125);
  ctx.lineTo(7, -125);
  ctx.fill();
  ctx.restore();
  line(cx, cy - 19, cx, cy + 17, '#fff', 2);
  line(cx - 15, cy + 3, cx + 15, cy + 3, '#fff', 2);
  ctx.fillStyle = '#080e16';
  ctx.strokeStyle = '#ffffff99';
  ctx.fillRect(cx - 43, 445, 86, 34);
  ctx.strokeRect(cx - 43, 445, 86, 34);
  const heading = Math.round(flight.heading) % 360;
  text(`${String(heading).padStart(3, '0')}°T`, cx, 463, 24);
  text('HDG', cx, 489, 12);
  text('XTRK 0.0 NM', cx, 614, 15);
}

function drawOverlay() {
  ctx.save();
  ctx.scale(scale, scale);
  ctx.shadowColor = '#0008';
  ctx.shadowBlur = 3;
  drawTape(353, flight.groundSpeed, 10, 52, 'GS KTS', String, 1);
  drawTape(913, flight.altitude, 100, 52, 'GPS ALT FT', String, -1);
  drawVerticalSpeed();
  drawAttitude();
  drawCompass();
  ctx.restore();
}

function render(now) {
  requestAnimationFrame(render);
  if (document.hidden || now - lastFrame < 1000 / 30) return;
  lastFrame = now;
  if (animated && !external) flight = demoFlight((now - demoStarted) / 1000);
  const stale = external && now - receivedAt > STALE_AFTER_MS;
  alert.hidden = !stale;
  status.textContent = external ? (stale ? 'TELEMETRY STALE' : 'EXTERNAL DATA · DEMO TERRAIN') : 'DEMO DATA';
  if (stale) {
    ctx.fillStyle = '#18202b';
    ctx.fillRect(0, 0, width, height);
  } else {
    if ((terrainDirty || animated || external) && now - lastTerrain >= 1000 / 15) {
      drawTerrain();
      lastTerrain = now;
      terrainDirty = false;
    }
    ctx.drawImage(terrainCanvas, 0, 0, width, height);
  }
  // Suppress all flight values on stale input, not just the attitude.
  if (!stale) drawOverlay();
}

// Integration hook for a local AHRS/CAN bridge; no hardware protocol is assumed.
window.updateEFIS = sample => {
  const next = validateFlight(sample);
  if (!next) return false;
  flight = next;
  external = true;
  animated = false;
  receivedAt = performance.now();
  terrainDirty = true;
  motion.disabled = true;
  motion.textContent = 'External telemetry';
  motion.setAttribute('aria-pressed', 'false');
  return true;
};
window.addEventListener('efis:telemetry', event => window.updateEFIS(event.detail));
motion.addEventListener('click', () => {
  animated = !animated;
  demoStarted = performance.now();
  if (!animated) flight = { ...INITIAL_FLIGHT };
  terrainDirty = true;
  motion.textContent = animated ? 'Reset demo' : 'Animate demo';
  motion.setAttribute('aria-pressed', String(animated));
});
document.querySelector('#fullscreen').addEventListener('click', async () => {
  try {
    if (document.fullscreenElement) await document.exitFullscreen();
    else await document.querySelector('main').requestFullscreen();
  } catch {
    status.textContent = 'FULL SCREEN UNAVAILABLE';
  }
});
requestAnimationFrame(render);
