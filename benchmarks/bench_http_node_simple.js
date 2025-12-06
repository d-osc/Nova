// HTTP server simulation for Nova
// Note: This is a mock version since Nova doesn't have HTTP module yet

function createServer(callback) {
  return {
    listen: function(port) {
      console.log('Mock HTTP server would listen on port ' + port);
      console.log('Server callback ready');
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

server.listen(3000);
