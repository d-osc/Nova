// WebSocket Benchmark (simulated with TCP)
const net = require('net');

const totalMessages = 10000;
let received = 0;
const start = Date.now();

const server = net.createServer((socket) => {
    socket.on('data', (data) => {
        // Echo back
        socket.write(data);
    });
});

server.listen(0, '127.0.0.1', () => {
    const port = server.address().port;

    const client = net.createConnection({ port, host: '127.0.0.1' }, () => {
        // Send messages
        for (let i = 0; i < totalMessages; i++) {
            client.write(`Message ${i}\n`);
        }
    });

    client.on('data', (data) => {
        const lines = data.toString().split('\n').filter(l => l);
        received += lines.length;

        if (received >= totalMessages) {
            const elapsed = Date.now() - start;
            const mps = Math.round(totalMessages / (elapsed / 1000));
            console.log(`Total messages: ${totalMessages}`);
            console.log(`Time: ${elapsed}ms`);
            console.log(`Messages/sec: ${mps}`);
            client.end();
            server.close();
        }
    });

    client.on('error', (err) => {
        console.log('Error:', err.message);
    });
});
