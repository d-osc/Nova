import * as http from 'nova:http';

const server = http.createServer((req, res) => {
    res.writeHead(200, { 'Content-Type': 'text/plain' });
    res.end('Hello from Nova');
});

server.listen(3000);
console.log('Nova HTTP ready');
