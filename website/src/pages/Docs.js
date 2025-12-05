import React from 'react';
import './Docs.css';

function Docs() {
  return (
    <div className="docs-page">
      <section className="page-hero">
        <div className="container">
          <h1 className="page-title">
            <span className="gradient-text">Documentation</span>
          </h1>
          <p className="page-subtitle">
            Everything you need to get started with Nova
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
                href="https://github.com/d-osc/nova-lang#quick-start"
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
                href="https://github.com/d-osc/nova-lang/blob/master/docs/USAGE_GUIDE.md"
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
                href="https://github.com/d-osc/nova-lang/blob/master/docs/ARCHITECTURE.md"
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
                href="https://github.com/d-osc/nova-lang/blob/master/docs/BUILD.md"
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
                href="https://github.com/d-osc/nova-lang/blob/master/SQLITE_ULTRA_OPTIMIZATION.md"
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
                href="https://github.com/d-osc/nova-lang/blob/master/docs/METHODS_STATUS.md"
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
wget https://github.com/d-osc/nova-lang/releases/latest/download/nova-installer.sh
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
                  </ul>
                </div>

                <div className="feature-showcase">
                  <h4>Built-in Types</h4>
                  <ul>
                    <li>Array, String, Number, Object</li>
                    <li>Map, Set, WeakMap, WeakSet</li>
                    <li>TypedArray (Int8Array, Uint8Array, etc.)</li>
                    <li>Promise, RegExp, Date</li>
                    <li>Symbol, Proxy, Reflect</li>
                  </ul>
                </div>
              </div>
            </div>

            <div className="docs-cta">
              <h2>Need more help?</h2>
              <p>Check out our GitHub repository for more documentation and examples</p>
              <div className="cta-buttons">
                <a
                  href="https://github.com/d-osc/nova-lang"
                  className="btn btn-primary"
                  target="_blank"
                  rel="noopener noreferrer"
                >
                  View on GitHub
                </a>
                <a
                  href="https://github.com/d-osc/nova-lang/tree/master/examples"
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
