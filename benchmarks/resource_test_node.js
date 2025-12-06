// HTTP server simulation for Nova
const port = 3001;

function createServer(callback) {
  return {
    listen: function(p, onListen) {
      console.log('Server running on port ' + p);
      if (onListen) onListen();
      // Simulate a request
      const mockReq = {};
      const mockRes = {
        writeHead: function(status, headers) {
          console.log('Response status: ' + status);
        },
        end: function(body) {
          console.log('Response body: ' + body);
        }
      };
      callback(mockReq, mockRes);
    }
  };
}

const server = createServer((req, res) => {
    res.writeHead(200, { 'Content-Type': 'text/plain' });
    res.end('Hello World');
});

server.listen(port, () => {
    console.log('Server running on port ' + port);
});
