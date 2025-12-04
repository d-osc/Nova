// Simple HTTP/2 test for Nova
import * as http2 from 'nova:http2';

console.log('Creating HTTP/2 server...');

const server = http2.createServer();

server.on('stream', (stream: any, headers: any) => {
  console.log('Received request:', headers);

  stream.respond({
    'content-type': 'text/plain',
    ':status': 200
  });

  stream.end('Hello HTTP/2 from Nova!');
  console.log('Response sent');
});

server.listen(3000, () => {
  console.log('Nova HTTP/2 server listening on port 3000');
  console.log('Test with: curl --http2-prior-knowledge http://localhost:3000/');
});

// Keep server running
