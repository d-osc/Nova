// Bun HTTP Routing Benchmark
interface User {
  id: number;
  name: string;
  email: string;
}

// Mock database
const users: User[] = [
  { id: 1, name: "Alice", email: "alice@example.com" },
  { id: 2, name: "Bob", email: "bob@example.com" },
  { id: 3, name: "Charlie", email: "charlie@example.com" },
];

let nextId = 4;

const server = Bun.serve({
  port: 3000,
  async fetch(req) {
    const url = new URL(req.url);
    const path = url.pathname;
    const method = req.method;

    // GET /
    if (method === "GET" && path === "/") {
      return Response.json({ message: "API is running" });
    }

    // GET /users
    if (method === "GET" && path === "/users") {
      return Response.json(users);
    }

    // GET /users/:id
    if (method === "GET" && path.startsWith("/users/")) {
      const id = parseInt(path.split("/")[2]);
      const user = users.find((u) => u.id === id);
      if (user) {
        return Response.json(user);
      } else {
        return Response.json({ error: "User not found" }, { status: 404 });
      }
    }

    // POST /users
    if (method === "POST" && path === "/users") {
      const data = await req.json();
      const newUser: User = {
        id: nextId++,
        name: data.name,
        email: data.email,
      };
      users.push(newUser);
      return Response.json(newUser, { status: 201 });
    }

    // PUT /users/:id
    if (method === "PUT" && path.startsWith("/users/")) {
      const id = parseInt(path.split("/")[2]);
      const data = await req.json();
      const userIndex = users.findIndex((u) => u.id === id);
      if (userIndex !== -1) {
        users[userIndex] = { ...users[userIndex], ...data };
        return Response.json(users[userIndex]);
      } else {
        return Response.json({ error: "User not found" }, { status: 404 });
      }
    }

    // DELETE /users/:id
    if (method === "DELETE" && path.startsWith("/users/")) {
      const id = parseInt(path.split("/")[2]);
      const userIndex = users.findIndex((u) => u.id === id);
      if (userIndex !== -1) {
        users.splice(userIndex, 1);
        return new Response(null, { status: 204 });
      } else {
        return Response.json({ error: "User not found" }, { status: 404 });
      }
    }

    // 404
    return Response.json({ error: "Not found" }, { status: 404 });
  },
});

console.log(`Bun routing server running on http://localhost:${server.port}`);
