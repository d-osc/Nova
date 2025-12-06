// HTTP Hello World Benchmark (Nova compatible version)
// Note: Bun.serve replaced with mock for Nova compatibility

const serverPort = 3000;

const server = {
  port: serverPort,
  serve: function(config) {
    console.log('Mock server running on http://localhost:' + serverPort);
    // Simulate a request
    const mockReq = {};
    const response = config.fetch(mockReq);
    console.log('Response: Hello World');
    return response;
  }
};

const config = {
  port: 3000,
  fetch: function(req) {
    return {
      body: "Hello World",
      headers: { "Content-Type": "text/plain" }
    };
  }
};

server.serve(config);
console.log('Nova server running on http://localhost:' + server.port);
