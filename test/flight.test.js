import test from 'node:test';
import assert from 'node:assert/strict';
import {
  INITIAL_FLIGHT, validateFlight, projectPoint, signedAngle, demoFlight, terrainHeight,
} from '../public/flight.js';
import { createDisplayServer } from '../server.js';

test('telemetry requires a complete finite sample and normalizes north', () => {
  assert.deepEqual(validateFlight(INITIAL_FLIGHT), INITIAL_FLIGHT);
  assert.equal(validateFlight({ ...INITIAL_FLIGHT, heading: 360 }).heading, 0);
  for (const key of Object.keys(INITIAL_FLIGHT)) {
    for (const value of [NaN, Infinity, '1', undefined, null]) {
      assert.equal(validateFlight({ ...INITIAL_FLIGHT, [key]: value }), null);
    }
  }
  assert.equal(validateFlight(null), null);
  assert.equal(validateFlight({ ...INITIAL_FLIGHT, pitch: 90 }), null);
  assert.equal(validateFlight({ ...INITIAL_FLIGHT, groundSpeed: -1 }), null);
  assert.equal(validateFlight({ ...INITIAL_FLIGHT, altitude: 60001 }), null);
});

test('projection agrees with attitude direction and rejects points behind camera', () => {
  assert.deepEqual(projectPoint(0, 100, 0, 0, 0, 100, 50, 50), { x: 50, y: 50 });
  assert.ok(projectPoint(0, 100, 0, 10, 0, 100, 50, 50).y > 50);
  assert.ok(projectPoint(10, 100, 0, 0, 20, 100, 50, 50).y < 50);
  assert.equal(projectPoint(0, -100, 0, 0, 0, 100, 50, 50), null);
  assert.equal(projectPoint(0, 10, 0, 0, 0, 100, 50, 50), null);
});

test('heading wraps through north by the shortest angle', () => {
  assert.equal(signedAngle(1 - 359), 2);
  assert.equal(signedAngle(359 - 1), -2);
  assert.equal(signedAngle(720), 0);
});

test('demo and procedural terrain stay finite and valid', () => {
  for (let second = 0; second < 3600; second += 7) {
    const flight = demoFlight(second);
    assert.ok(validateFlight(flight));
    assert.ok(Number.isFinite(terrainHeight(flight.east, flight.north)));
  }
});

test('server serves only allowlisted display assets', async t => {
  const server = createDisplayServer();
  await new Promise(resolve => server.listen(0, '127.0.0.1', resolve));
  t.after(() => new Promise(resolve => server.close(resolve)));
  const base = `http://127.0.0.1:${server.address().port}`;
  const home = await fetch(base);
  assert.equal(home.status, 200);
  assert.match(home.headers.get('content-security-policy'), /default-src 'self'/);
  assert.match(await home.text(), /NOT FOR NAVIGATION/);
  for (const path of ['/display.js', '/flight.js', '/display.css']) {
    assert.equal((await fetch(base + path)).status, 200);
    assert.equal((await fetch(base + path, { method: 'HEAD' })).status, 200);
  }
  for (const path of ['/package.json', '/server.js', '/.git/config', '/%2e%2e/server.js']) {
    assert.equal((await fetch(base + path)).status, 404);
  }
  assert.equal((await fetch(base, { method: 'POST' })).status, 404);
});
