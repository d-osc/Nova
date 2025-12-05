// Test if nova:http works
import * as http from 'nova:http';

console.log('Testing nova:http...');
console.log('http module:', typeof http);
console.log('createServer:', typeof http.createServer);

const server = http.createServer((req: any, res: any) => {
  res.end('Hello from HTTP!');
});

console.log('Server created successfully');
console.log('Listening on port 3001...');

server.listen(3001, '127.0.0.1', () => {
  console.log('HTTP server ready');
});
