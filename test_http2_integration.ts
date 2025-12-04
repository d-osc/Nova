// Test HTTP/2 module integration
import * as http2 from 'nova:http2';

console.log('Testing nova:http2 integration...');
console.log('http2 module type:', typeof http2);
console.log('createServer type:', typeof http2.createServer);

const server = http2.createServer();
console.log('Server created:', typeof server);

server.on('stream', (stream: any, headers: any) => {
  console.log('Request received!');
  stream.respond({
    ':status': 200,
    'content-type': 'text/plain'
  });
  stream.end('Hello from Nova HTTP/2!');
});

server.listen(3000, () => {
  console.log('Nova HTTP/2 server listening on port 3000');
});
