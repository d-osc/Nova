// Node.js HTTP/2 Server Benchmark
const http2 = require('http2');

const server = http2.createServer();

server.on('stream', (stream, headers) => {
  stream.respond({
    'content-type': 'text/plain',
    ':status': 200
  });
  stream.end('Hello HTTP/2 from Node.js!');
});

server.listen(3000, () => {
  console.log('Node.js HTTP/2 server listening on port 3000');
});
