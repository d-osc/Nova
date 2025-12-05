import React from 'react';
import { useLanguage } from '../contexts/LanguageContext';
import './Docs.css';

function Docs() {
  const { t } = useLanguage();

  return (
    <div className="docs-page">
      <section className="page-hero">
        <div className="container">
          <h1 className="page-title">
            <span className="gradient-text">{t('docs.title')}</span>
          </h1>
          <p className="page-subtitle">
            {t('docs.subtitle')}
          </p>
        </div>
      </section>

      <section className="section">
        <div className="container">
          <div className="docs-grid">
            <div className="doc-card">
              <div className="doc-icon">🚀</div>
              <h3>Quick Start</h3>
              <p>Get up and running with Nova in minutes</p>
              <a
                href="https://github.com/d-osc/Nova#quick-start"
                className="doc-link"
                target="_blank"
                rel="noopener noreferrer"
              >
                Read Guide →
              </a>
            </div>

            <div className="doc-card">
              <div className="doc-icon">📖</div>
              <h3>Usage Guide</h3>
              <p>Comprehensive guide to using Nova compiler</p>
              <a
                href="https://github.com/d-osc/Nova/blob/master/docs/USAGE_GUIDE.md"
                className="doc-link"
                target="_blank"
                rel="noopener noreferrer"
              >
                Read Guide →
              </a>
            </div>

            <div className="doc-card">
              <div className="doc-icon">🏗️</div>
              <h3>Architecture</h3>
              <p>Learn about Nova's compiler architecture</p>
              <a
                href="https://github.com/d-osc/Nova/blob/master/docs/ARCHITECTURE.md"
                className="doc-link"
                target="_blank"
                rel="noopener noreferrer"
              >
                Read Guide →
              </a>
            </div>

            <div className="doc-card">
              <div className="doc-icon">🔧</div>
              <h3>Build Instructions</h3>
              <p>How to build Nova from source</p>
              <a
                href="https://github.com/d-osc/Nova/blob/master/docs/BUILD.md"
                className="doc-link"
                target="_blank"
                rel="noopener noreferrer"
              >
                Read Guide →
              </a>
            </div>

            <div className="doc-card">
              <div className="doc-icon">⚡</div>
              <h3>SQLite Optimization</h3>
              <p>Ultra-fast SQLite implementation details</p>
              <a
                href="https://github.com/d-osc/Nova/blob/master/SQLITE_ULTRA_OPTIMIZATION.md"
                className="doc-link"
                target="_blank"
                rel="noopener noreferrer"
              >
                Read Guide →
              </a>
            </div>

            <div className="doc-card">
              <div className="doc-icon">📋</div>
              <h3>Methods Status</h3>
              <p>Complete list of supported methods</p>
              <a
                href="https://github.com/d-osc/Nova/blob/master/docs/METHODS_STATUS.md"
                className="doc-link"
                target="_blank"
                rel="noopener noreferrer"
              >
                Read Guide →
              </a>
            </div>
          </div>

          <div className="docs-sections">
            <div className="docs-section">
              <h2>Getting Started</h2>
              <div className="code-example">
                <h4>Installation</h4>
                <pre>
                  <code>{`# Download and install
wget https://github.com/d-osc/Nova/releases/latest/download/nova-installer.sh
sh nova-installer.sh

# Verify installation
nova --version`}</code>
                </pre>
              </div>

              <div className="code-example">
                <h4>Hello World</h4>
                <pre>
                  <code>{`// hello.ts
function main(): number {
    console.log("Hello, Nova!");
    return 0;
}

// Run it
nova run hello.ts`}</code>
                </pre>
              </div>

              <div className="code-example">
                <h4>Compile to Binary</h4>
                <pre>
                  <code>{`# Compile to native executable
nova compile app.ts -o app

# Run the compiled binary
./app`}</code>
                </pre>
              </div>
            </div>

            <div className="docs-section">
              <h2>Built-in Modules</h2>
              <p>Nova provides 40+ Node.js-compatible built-in modules:</p>

              <div className="modules-list">
                <div className="module-group">
                  <h4>File System & OS</h4>
                  <ul>
                    <li><code>nova:fs</code> - File system operations</li>
                    <li><code>nova:path</code> - Path utilities</li>
                    <li><code>nova:os</code> - OS information</li>
                  </ul>
                </div>

                <div className="module-group">
                  <h4>Networking</h4>
                  <ul>
                    <li><code>nova:http</code> - HTTP server/client</li>
                    <li><code>nova:https</code> - HTTPS support</li>
                    <li><code>nova:http2</code> - HTTP/2 protocol</li>
                    <li><code>nova:net</code> - TCP/UDP sockets</li>
                    <li><code>nova:tls</code> - TLS/SSL</li>
                    <li><code>nova:dns</code> - DNS lookups</li>
                  </ul>
                </div>

                <div className="module-group">
                  <h4>Database</h4>
                  <ul>
                    <li><code>nova:sqlite</code> - Ultra-fast SQLite (5-10x faster)</li>
                  </ul>
                </div>

                <div className="module-group">
                  <h4>Utilities</h4>
                  <ul>
                    <li><code>nova:crypto</code> - Cryptography</li>
                    <li><code>nova:buffer</code> - Binary data</li>
                    <li><code>nova:stream</code> - Streams</li>
                    <li><code>nova:zlib</code> - Compression</li>
                    <li><code>nova:util</code> - Utilities</li>
                  </ul>
                </div>
              </div>

              <div className="code-example">
                <h4>Module Usage Example</h4>
                <pre>
                  <code>{`import { readFileSync, writeFileSync } from 'nova:fs';
import { Database } from 'nova:sqlite';
import { createServer } from 'nova:http';

// File operations
const data = readFileSync('input.txt', 'utf-8');
writeFileSync('output.txt', data.toUpperCase());

// SQLite database
const db = new Database(':memory:');
db.exec('CREATE TABLE users (id INTEGER PRIMARY KEY, name TEXT)');

// HTTP server
const server = createServer((req, res) => {
  res.writeHead(200, { 'Content-Type': 'text/plain' });
  res.end('Hello from Nova!');
});
server.listen(3000);`}</code>
                </pre>
              </div>
            </div>

            <div className="docs-section">
              <h2>Package Manager</h2>
              <p>Nova includes an npm-compatible package manager:</p>

              <div className="code-example">
                <pre>
                  <code>{`# Install dependencies from package.json
nova pm install

# Install a specific package
nova pm install lodash

# Install dev dependency
nova pm install --save-dev typescript

# Install global package
nova pm install -g typescript

# Update packages
nova pm update

# Remove a package
nova pm uninstall lodash`}</code>
                </pre>
              </div>
            </div>

            <div className="docs-section">
              <h2>CLI Commands</h2>
              <p>Complete reference of Nova CLI commands:</p>

              <div className="command-reference">
                <div className="command-item">
                  <h4><code>nova compile &lt;file&gt;</code></h4>
                  <p>Compile TypeScript to native executable using LLVM</p>
                  <pre>
                    <code>{`# Basic compilation
nova compile app.ts

# With output name
nova compile app.ts -o myapp

# With optimization level (0-3)
nova compile app.ts -O3

# Emit LLVM IR
nova compile app.ts --emit-llvm

# Verbose output
nova compile app.ts -v`}</code>
                  </pre>
                </div>

                <div className="command-item">
                  <h4><code>nova run &lt;file&gt;</code></h4>
                  <p>JIT compile and execute TypeScript/JavaScript</p>
                  <pre>
                    <code>{`# Run TypeScript file
nova run app.ts

# Run with arguments
nova run app.ts -- arg1 arg2

# Run with environment variables
NOVA_DEBUG=1 nova run app.ts`}</code>
                  </pre>
                </div>

                <div className="command-item">
                  <h4><code>nova build &lt;file&gt;</code></h4>
                  <p>Transpile TypeScript to JavaScript (like tsc)</p>
                  <pre>
                    <code>{`# Transpile to JavaScript
nova build app.ts

# Output to specific file
nova build app.ts -o output.js`}</code>
                  </pre>
                </div>

                <div className="command-item">
                  <h4><code>nova init [ts]</code></h4>
                  <p>Initialize a new Nova project</p>
                  <pre>
                    <code>{`# Initialize JavaScript project
nova init

# Initialize TypeScript project
nova init ts`}</code>
                  </pre>
                </div>

                <div className="command-item">
                  <h4><code>nova test</code></h4>
                  <p>Run automated tests</p>
                  <pre>
                    <code>{`# Run all tests
nova test

# Run specific test file
nova test tests/mytest.ts`}</code>
                  </pre>
                </div>
              </div>
            </div>

            <div className="docs-section">
              <h2>Language Features</h2>
              <div className="features-showcase">
                <div className="feature-showcase">
                  <h4>Modern JavaScript/TypeScript</h4>
                  <ul>
                    <li>ES2023+ syntax support</li>
                    <li>Async/await, Promises</li>
                    <li>Generators and async generators</li>
                    <li>Classes with inheritance</li>
                    <li>Destructuring and spread operator</li>
                    <li>Optional chaining and nullish coalescing</li>
                    <li>Template literals and tagged templates</li>
                    <li>Arrow functions and default parameters</li>
                    <li>Rest/spread operator</li>
                    <li>Module imports/exports (ESM)</li>
                  </ul>
                </div>

                <div className="feature-showcase">
                  <h4>Built-in Types & Objects</h4>
                  <ul>
                    <li>Array, String, Number, Object</li>
                    <li>Map, Set, WeakMap, WeakSet</li>
                    <li>TypedArray (Int8Array, Uint8Array, etc.)</li>
                    <li>Promise, RegExp, Date</li>
                    <li>Symbol, Proxy, Reflect</li>
                    <li>ArrayBuffer, DataView</li>
                    <li>BigInt (ES2020)</li>
                    <li>JSON, Math, Console</li>
                  </ul>
                </div>
              </div>

              <div className="code-example">
                <h4>Advanced Language Examples</h4>
                <pre>
                  <code>{`// Async/await with error handling
async function fetchData(url: string) {
  try {
    const response = await fetch(url);
    return await response.json();
  } catch (error) {
    console.error('Fetch failed:', error);
    throw error;
  }
}

// Generators
function* fibonacci() {
  let [prev, curr] = [0, 1];
  while (true) {
    yield curr;
    [prev, curr] = [curr, prev + curr];
  }
}

// Classes with decorators
class UserService {
  private users = new Map<number, User>();

  async getUser(id: number): Promise<User | null> {
    return this.users.get(id) ?? null;
  }

  async createUser(data: UserData): Promise<User> {
    const user = { id: Date.now(), ...data };
    this.users.set(user.id, user);
    return user;
  }
}

// Pattern matching (upcoming)
const result = match(value) {
  case 0: return "zero",
  case n if n > 0: return "positive",
  default: return "negative"
};`}</code>
                </pre>
              </div>
            </div>

            <div className="docs-section">
              <h2>Configuration</h2>
              <p>Configure Nova using <code>nova.config.json</code>:</p>

              <div className="code-example">
                <pre>
                  <code>{`{
  "compilerOptions": {
    "target": "es2023",
    "module": "esm",
    "outDir": "./dist",
    "sourceMap": true,
    "declaration": true,
    "strict": true,
    "noImplicitAny": true,
    "esModuleInterop": true
  },
  "include": ["src/**/*"],
  "exclude": ["node_modules", "**/*.spec.ts"],
  "optimization": {
    "level": 3,
    "inlining": true,
    "deadCodeElimination": true,
    "loopOptimization": true
  },
  "runtime": {
    "gc": "incremental",
    "threads": 4,
    "heapSize": "512M"
  }
}`}</code>
                </pre>
              </div>
            </div>

            <div className="docs-section">
              <h2>Performance Tips</h2>
              <div className="tips-grid">
                <div className="tip-card">
                  <h4>🚀 Use AOT Compilation</h4>
                  <p>Compile to native binaries for maximum performance:</p>
                  <pre>
                    <code>{`nova compile app.ts -O3 -o app
# 5-10x faster than JIT mode`}</code>
                  </pre>
                </div>

                <div className="tip-card">
                  <h4>⚡ Leverage Built-in Modules</h4>
                  <p>Nova's native modules are optimized for speed:</p>
                  <pre>
                    <code>{`// Ultra-fast SQLite
import { Database } from 'nova:sqlite';
// 5-10x faster than node-sqlite3`}</code>
                  </pre>
                </div>

                <div className="tip-card">
                  <h4>💾 Memory Management</h4>
                  <p>Use TypedArrays for better performance:</p>
                  <pre>
                    <code>{`// Efficient binary data
const buffer = new Uint8Array(1024);
// Zero-copy when possible`}</code>
                  </pre>
                </div>

                <div className="tip-card">
                  <h4>🔧 Optimization Flags</h4>
                  <p>Enable aggressive optimizations:</p>
                  <pre>
                    <code>{`nova compile app.ts -O3 \\
  --inline-functions \\
  --loop-unroll`}</code>
                  </pre>
                </div>
              </div>
            </div>

            <div className="docs-section">
              <h2>Debugging</h2>
              <p>Debug your Nova applications with these tools:</p>

              <div className="code-example">
                <h4>Enable Debug Mode</h4>
                <pre>
                  <code>{`# Run with debug output
NOVA_DEBUG=1 nova run app.ts

# Generate source maps
nova compile app.ts --source-map

# Verbose compilation
nova compile app.ts -v

# Emit LLVM IR for inspection
nova compile app.ts --emit-llvm -o app.ll`}</code>
                </pre>
              </div>

              <div className="code-example">
                <h4>Console Debugging</h4>
                <pre>
                  <code>{`// Standard console methods
console.log('Debug info:', data);
console.error('Error:', error);
console.warn('Warning:', message);
console.time('operation');
// ... code ...
console.timeEnd('operation');

// Stack traces
console.trace('Trace point');

// Assertions
console.assert(condition, 'Assertion failed');`}</code>
                </pre>
              </div>
            </div>

            <div className="docs-section">
              <h2>Testing</h2>
              <p>Write and run tests with Nova's built-in test runner:</p>

              <div className="code-example">
                <pre>
                  <code>{`// test/example.test.ts
import { test, expect } from 'nova:test';

test('basic arithmetic', () => {
  expect(2 + 2).toBe(4);
  expect(10 - 5).toBe(5);
});

test('async operations', async () => {
  const result = await fetchData();
  expect(result).toBeDefined();
  expect(result.length).toBeGreaterThan(0);
});

test('error handling', () => {
  expect(() => {
    throw new Error('Test error');
  }).toThrow('Test error');
});

// Run tests
// nova test`}</code>
                </pre>
              </div>
            </div>

            <div className="docs-section">
              <h2>Deployment</h2>
              <p>Deploy your Nova applications to production:</p>

              <div className="deployment-options">
                <div className="deploy-option">
                  <h4>📦 Standalone Binary</h4>
                  <pre>
                    <code>{`# Compile to optimized binary
nova compile app.ts -O3 -o app

# Deploy the single executable
scp app user@server:/usr/local/bin/
# No runtime dependencies needed!`}</code>
                  </pre>
                </div>

                <div className="deploy-option">
                  <h4>🐳 Docker Container</h4>
                  <pre>
                    <code>{`FROM alpine:latest

# Copy pre-compiled binary
COPY app /usr/local/bin/app

# Run the app
CMD ["/usr/local/bin/app"]

# Build: docker build -t myapp .
# Run: docker run -p 3000:3000 myapp`}</code>
                  </pre>
                </div>

                <div className="deploy-option">
                  <h4>☁️ Systemd Service</h4>
                  <pre>
                    <code>{`[Unit]
Description=Nova Application
After=network.target

[Service]
Type=simple
User=nova
WorkingDirectory=/opt/app
ExecStart=/opt/app/app
Restart=on-failure

[Install]
WantedBy=multi-user.target`}</code>
                  </pre>
                </div>
              </div>
            </div>

            <div className="docs-cta">
              <h2>Need more help?</h2>
              <p>Check out our GitHub repository for more documentation and examples</p>
              <div className="cta-buttons">
                <a
                  href="https://github.com/d-osc/Nova"
                  className="btn btn-primary"
                  target="_blank"
                  rel="noopener noreferrer"
                >
                  View on GitHub
                </a>
                <a
                  href="https://github.com/d-osc/Nova/tree/master/examples"
                  className="btn btn-secondary"
                  target="_blank"
                  rel="noopener noreferrer"
                >
                  View Examples
                </a>
              </div>
            </div>
          </div>
        </div>
      </section>
    </div>
  );
}

export default Docs;
