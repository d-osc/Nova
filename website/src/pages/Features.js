import React from 'react';
import { useLanguage } from '../contexts/LanguageContext';
import './Features.css';

function Features() {
  const { t } = useLanguage();

  return (
    <div className="features-page">
      <section className="page-hero">
        <div className="container">
          <h1 className="page-title">
            <span className="gradient-text">{t('features.title')}</span>
          </h1>
          <p className="page-subtitle">
            {t('features.subtitle')}
          </p>
        </div>
      </section>

      <section className="section">
        <div className="container">
          <div className="feature-detailed">
            <div className="feature-content">
              <h2>⚡ Ultra-Fast SQLite</h2>
              <p>
                Nova's SQLite implementation is 5-10x faster than Node.js better-sqlite3,
                featuring statement caching, connection pooling, and zero-copy optimizations.
              </p>
              <ul>
                <li>LRU statement cache (up to 128 prepared statements)</li>
                <li>Connection pooling (up to 32 reusable connections)</li>
                <li>Zero-copy strings with std::string_view</li>
                <li>Arena allocator for O(1) allocations</li>
                <li>WAL mode and memory-mapped I/O</li>
              </ul>
            </div>
            <div className="feature-code">
              <pre>
                <code>{`import { Database } from 'nova:sqlite';

const db = new Database(':memory:');

// Statement caching automatic
const stmt = db.prepare(
  'SELECT * FROM users WHERE id = ?'
);

// 5x faster with cached statements!
for (let i = 0; i < 10000; i++) {
  stmt.all(i);
}`}</code>
              </pre>
            </div>
          </div>

          <div className="feature-detailed reverse">
            <div className="feature-content">
              <h2>🔧 Node.js Compatible</h2>
              <p>
                40+ built-in modules compatible with Node.js APIs. Drop-in replacement
                for many Node.js applications with better performance.
              </p>
              <ul>
                <li>File System (fs), Path, OS modules</li>
                <li>HTTP, HTTPS, HTTP/2 servers</li>
                <li>Crypto, Buffer, Streams</li>
                <li>Child Process, Worker Threads, Cluster</li>
                <li>And many more...</li>
              </ul>
            </div>
            <div className="feature-code">
              <pre>
                <code>{`import { readFileSync } from 'nova:fs';
import { createServer } from 'nova:http';

const server = createServer((req, res) => {
  const content = readFileSync('index.html');
  res.writeHead(200, {
    'Content-Type': 'text/html'
  });
  res.end(content);
});

server.listen(3000);`}</code>
              </pre>
            </div>
          </div>

          <div className="feature-detailed">
            <div className="feature-content">
              <h2>📦 Built-in Package Manager</h2>
              <p>
                npm-compatible package manager built directly into Nova. Install and
                manage dependencies without additional tools.
              </p>
              <ul>
                <li>Install from npm registry</li>
                <li>Support for dev dependencies</li>
                <li>Global package installation</li>
                <li>Automatic dependency resolution</li>
                <li>Package.json compatibility</li>
              </ul>
            </div>
            <div className="feature-code">
              <pre>
                <code>{`# Install dependencies
nova pm install

# Install specific package
nova pm install lodash

# Dev dependency
nova pm install --save-dev typescript

# Global install
nova pm install -g typescript

# Update packages
nova pm update`}</code>
              </pre>
            </div>
          </div>

          <div className="feature-detailed reverse">
            <div className="feature-content">
              <h2>🚀 LLVM-Powered Compilation</h2>
              <p>
                Multi-stage compilation pipeline through AST, HIR, MIR, and LLVM IR
                for maximum optimization and native performance.
              </p>
              <ul>
                <li>Aggressive inlining and optimization</li>
                <li>SIMD vectorization</li>
                <li>Dead code elimination</li>
                <li>Constant folding and propagation</li>
                <li>Link-time optimization (LTO)</li>
              </ul>
            </div>
            <div className="feature-code">
              <pre>
                <code>{`TypeScript/JavaScript
        ↓
      AST (Abstract Syntax Tree)
        ↓
      HIR (High-level IR)
        ↓
      MIR (Mid-level IR)
        ↓
      LLVM IR
        ↓
    Native Code (x64/ARM)`}</code>
              </pre>
            </div>
          </div>

          <div className="feature-detailed">
            <div className="feature-content">
              <h2>💾 Efficient Memory Management</h2>
              <p>
                Advanced garbage collection and memory management techniques result
                in 30-50% less memory usage compared to Node.js.
              </p>
              <ul>
                <li>Generational garbage collector</li>
                <li>Zero-copy string operations</li>
                <li>Arena allocators for temporary data</li>
                <li>Smart pointer optimizations</li>
                <li>Memory pool reuse</li>
              </ul>
            </div>
            <div className="feature-code">
              <pre>
                <code>{`// Nova automatically optimizes
// memory usage

// Zero-copy when possible
const str = "Hello World";
const sub = str.slice(0, 5);

// Efficient array operations
const arr = new Array(1000000);
arr.push(1); // No reallocation

// Smart GC
function process() {
  const data = new Array(10000);
  // Automatically freed
}`}</code>
              </pre>
            </div>
          </div>

          <div className="feature-detailed reverse">
            <div className="feature-content">
              <h2>✅ Battle-Tested Reliability</h2>
              <p>
                511 comprehensive tests with 100% pass rate ensure reliability
                and compatibility across all features.
              </p>
              <ul>
                <li>Unit tests for all core features</li>
                <li>Integration tests for modules</li>
                <li>Performance regression tests</li>
                <li>Cross-platform compatibility tests</li>
                <li>Continuous integration</li>
              </ul>
            </div>
            <div className="feature-code">
              <pre>
                <code>{`# Run all tests
python run_all_tests.py

Results:
✓ 511 tests passing
✓ 100% pass rate
✓ All platforms supported

# Run specific tests
nova tests/test_array.ts
nova tests/test_sqlite.ts
nova tests/test_http.ts`}</code>
              </pre>
            </div>
          </div>
        </div>
      </section>

      <section className="language-features section">
        <div className="container">
          <h2 className="section-title">Complete Language Support</h2>
          <div className="features-grid-compact">
            <div className="feature-item">
              <h3>✓ Modern JavaScript</h3>
              <p>ES2023+ features including async/await, generators, destructuring</p>
            </div>
            <div className="feature-item">
              <h3>✓ TypeScript</h3>
              <p>Full TypeScript support with type inference and checking</p>
            </div>
            <div className="feature-item">
              <h3>✓ Classes & OOP</h3>
              <p>Complete class system with inheritance, getters/setters</p>
            </div>
            <div className="feature-item">
              <h3>✓ Async Programming</h3>
              <p>Promises, async/await, async generators</p>
            </div>
            <div className="feature-item">
              <h3>✓ Advanced Operators</h3>
              <p>Optional chaining, nullish coalescing, spread operator</p>
            </div>
            <div className="feature-item">
              <h3>✓ Built-in Types</h3>
              <p>Array, String, Number, Object, Map, Set, TypedArray</p>
            </div>
          </div>
        </div>
      </section>
    </div>
  );
}

export default Features;
