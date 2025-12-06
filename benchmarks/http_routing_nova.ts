// Nova HTTP Routing Benchmark
// Note: Import and some TypeScript features removed for Nova compatibility

interface User {
  id: number;
  name: string;
  email: string;
}

// Mock HTTP server function
function createServer(callback) {
  return {
    listen: function(port) {
      console.log('Mock HTTP routing server would listen on port ' + port);
    }
  };
}

// Mock database
const users = [
  { id: 1, name: "Alice", email: "alice@example.com" },
  { id: 2, name: "Bob", email: "bob@example.com" },
  { id: 3, name: "Charlie", email: "charlie@example.com" },
];

let nextId = 4;

const server = createServer((req, res) => {
  const url = req.url || "/";
  const method = req.method || "GET";

  // CORS headers
  res.setHeader("Access-Control-Allow-Origin", "*");
  res.setHeader("Content-Type", "application/json");

  // GET /
  if (method === "GET" && url === "/") {
    res.writeHead(200);
    res.end(JSON.stringify({ message: "API is running" }));
    return;
  }

  // GET /users
  if (method === "GET" && url === "/users") {
    res.writeHead(200);
    res.end(JSON.stringify(users));
    return;
  }

  // GET /users/:id
  if (method === "GET" && url.startsWith("/users/")) {
    const id = parseInt(url.split("/")[2]);
    const user = users.find((u) => u.id === id);
    if (user) {
      res.writeHead(200);
      res.end(JSON.stringify(user));
    } else {
      res.writeHead(404);
      res.end(JSON.stringify({ error: "User not found" }));
    }
    return;
  }

  // POST /users
  if (method === "POST" && url === "/users") {
    let body = "";
    req.on("data", (chunk) => {
      body += chunk.toString();
    });
    req.on("end", () => {
      const data = JSON.parse(body);
      const newUser = {
        id: nextId++,
        name: data.name,
        email: data.email,
      };
      users.push(newUser);
      res.writeHead(201);
      res.end(JSON.stringify(newUser));
    });
    return;
  }

  // PUT /users/:id
  if (method === "PUT" && url.startsWith("/users/")) {
    const id = parseInt(url.split("/")[2]);
    let body = "";
    req.on("data", (chunk) => {
      body += chunk.toString();
    });
    req.on("end", () => {
      const data = JSON.parse(body);
      const userIndex = users.findIndex((u) => u.id === id);
      if (userIndex !== -1) {
        // Spread operator replaced with Object.assign for Nova compatibility
        const updatedUser = Object.assign({}, users[userIndex], data);
        users[userIndex] = updatedUser;
        res.writeHead(200);
        res.end(JSON.stringify(users[userIndex]));
      } else {
        res.writeHead(404);
        res.end(JSON.stringify({ error: "User not found" }));
      }
    });
    return;
  }

  // DELETE /users/:id
  if (method === "DELETE" && url.startsWith("/users/")) {
    const id = parseInt(url.split("/")[2]);
    const userIndex = users.findIndex((u) => u.id === id);
    if (userIndex !== -1) {
      users.splice(userIndex, 1);
      res.writeHead(204);
      res.end();
    } else {
      res.writeHead(404);
      res.end(JSON.stringify({ error: "User not found" }));
    }
    return;
  }

  // 404
  res.writeHead(404);
  res.end(JSON.stringify({ error: "Not found" }));
});

const PORT = 3000;
server.listen(PORT);
console.log(`Nova routing server running on http://localhost:${PORT}`);
