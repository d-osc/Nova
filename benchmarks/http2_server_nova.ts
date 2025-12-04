// Nova HTTP/2 Server Benchmark
import * as http2 from 'nova:http2';

const server = http2.createServer();

server.on('stream', (stream: any, headers: any) => {
  stream.respond({
    'content-type': 'text/plain',
    ':status': 200
  });
  stream.end('Hello HTTP/2 from Nova!');
});

server.listen(3000, () => {
  console.log('Nova HTTP/2 server listening on port 3000');
});
