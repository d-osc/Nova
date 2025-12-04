// Deno HTTP Hello World Benchmark
const handler = (_req: Request): Response => {
  return new Response("Hello World!", {
    status: 200,
    headers: { "Content-Type": "text/plain" },
  });
};

console.log("Deno server running on http://localhost:3000");
Deno.serve({ port: 3000, handler });
