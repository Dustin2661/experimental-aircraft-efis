import { createServer } from 'node:http';
import { readFile } from 'node:fs/promises';

const files = new Map([
  ['/', ['index.html', 'text/html; charset=utf-8']],
  ['/display.css', ['display.css', 'text/css; charset=utf-8']],
  ['/display.js', ['display.js', 'text/javascript; charset=utf-8']],
  ['/flight.js', ['flight.js', 'text/javascript; charset=utf-8']],
]);

export function createDisplayServer() {
  return createServer(async (request, response) => {
    let path;
    try {
      path = new URL(request.url, 'http://localhost').pathname;
    } catch {
      response.writeHead(400);
      response.end('Bad request');
      return;
    }
    const file = files.get(path);
    if (!file || !['GET', 'HEAD'].includes(request.method)) {
      response.writeHead(404);
      response.end('Not found');
      return;
    }
    try {
      const content = await readFile(new URL(`./public/${file[0]}`, import.meta.url));
      response.writeHead(200, {
        'Content-Type': file[1],
        'Cache-Control': 'no-cache',
        'X-Content-Type-Options': 'nosniff',
        'Content-Security-Policy': "default-src 'self'; script-src 'self'; style-src 'self'; img-src 'self'; connect-src 'self'; object-src 'none'; frame-ancestors 'none'",
      });
      response.end(request.method === 'HEAD' ? undefined : content);
    } catch {
      response.writeHead(500);
      response.end('Display unavailable');
    }
  });
}

if (process.argv[1] && import.meta.url === new URL(process.argv[1], 'file:').href) {
  const port = Number(process.env.PORT || 8080);
  const host = process.env.HOST || '127.0.0.1';
  createDisplayServer().listen(port, host, () => {
    console.log(`EFIS layout demo: http://${host}:${port}`);
  });
}
