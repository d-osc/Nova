// Simple HTTP/2 test using the callback-based API
import * as http2 from 'nova:http2';

console.log('Creating HTTP/2 server...');

// Create server with request handler callback
const server = http2.createServer((req: any, res: any) => {
  console.log('Request received!');

  // Write headers
  res.writeHead(200, {
    'content-type': 'text/plain'
  });

  // Send response
  res.write('Hello from Nova HTTP/2!');
  res.end();
});

console.log('Server object created');
console.log('Starting server on port 8080...');

// Listen on port 8080
http2.Server_listen(server, 8080, '127.0.0.1', () => {
  console.log('Server is listening on http://127.0.0.1:8080');
});
