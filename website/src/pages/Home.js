import React from 'react';
import { Link } from 'react-router-dom';
import './Home.css';

function Home() {
  return (
    <div className="home">
      {/* Hero Section */}
      <section className="hero">
        <div className="container">
          <div className="hero-content">
            <h1 className="hero-title">
              <span className="gradient-text">Nova</span>
              <br />
              TypeScript to Native Code
            </h1>
            <p className="hero-subtitle">
              Compile TypeScript/JavaScript directly to native code via LLVM.
              <br />
              <strong>5-10x faster SQLite</strong>, native performance, Node.js compatible.
            </p>
            <div className="hero-buttons">
              <Link to="/download" className="btn btn-primary">
                Get Started
              </Link>
              <Link to="/docs" className="btn btn-secondary">
                Documentation
              </Link>
            </div>
            <div className="hero-stats">
              <div className="stat">
                <div className="stat-value">5-10x</div>
                <div className="stat-label">Faster SQLite</div>
              </div>
              <div className="stat">
                <div className="stat-value">511</div>
                <div className="stat-label">Tests Passing</div>
              </div>
              <div className="stat">
                <div className="stat-value">40+</div>
                <div className="stat-label">Built-in Modules</div>
              </div>
            </div>
          </div>

          <div className="hero-code">
            <div className="code-header">
              <span></span>
              <span></span>
              <span></span>
            </div>
            <pre>
              <code>{`// hello.ts
import { Database } from 'nova:sqlite';

const db = new Database(':memory:');

db.exec(\`
  CREATE TABLE users (
    id INTEGER PRIMARY KEY,
    name TEXT
  )
\`);

const stmt = db.prepare(
  'INSERT INTO users (name) VALUES (?)'
);

stmt.run('Alice');
stmt.run('Bob');

const users = db.prepare(
  'SELECT * FROM users'
).all();

console.log(users);
// Ultra-fast! 5-10x faster than Node.js`}</code>
            </pre>
          </div>
        </div>
      </section>

      {/* Features Overview */}
      <section className="features-overview section">
        <div className="container">
          <h2 className="section-title">
            Why <span className="gradient-text">Nova</span>?
          </h2>
          <div className="features-grid">
            <div className="feature-card">
              <div className="feature-icon">⚡</div>
              <h3>Ultra Performance</h3>
              <p>
                Compiles to native code via LLVM with aggressive optimizations.
                5-10x faster SQLite than Node.js better-sqlite3.
              </p>
            </div>

            <div className="feature-card">
              <div className="feature-icon">🔧</div>
              <h3>Node.js Compatible</h3>
              <p>
                40+ built-in modules compatible with Node.js APIs.
                Drop-in replacement for many Node.js applications.
              </p>
            </div>

            <div className="feature-card">
              <div className="feature-icon">📦</div>
              <h3>Package Manager</h3>
              <p>
                npm-compatible package manager built-in.
                Install and manage dependencies seamlessly.
              </p>
            </div>

            <div className="feature-card">
              <div className="feature-icon">💾</div>
              <h3>Low Memory</h3>
              <p>
                30-50% less memory usage compared to Node.js.
                Efficient garbage collection and memory management.
              </p>
            </div>

            <div className="feature-card">
              <div className="feature-icon">🚀</div>
              <h3>Fast Startup</h3>
              <p>
                ~5-10ms startup time, 2-3x faster than Node.js.
                Perfect for CLI tools and serverless functions.
              </p>
            </div>

            <div className="feature-card">
              <div className="feature-icon">✅</div>
              <h3>Battle Tested</h3>
              <p>
                511 tests with 100% pass rate.
                Comprehensive test coverage for reliability.
              </p>
            </div>
          </div>
        </div>
      </section>

      {/* Performance Highlights */}
      <section className="performance section">
        <div className="container">
          <h2 className="section-title">
            <span className="gradient-text">Performance</span> that Matters
          </h2>
          <div className="perf-grid">
            <div className="perf-card">
              <h3>SQLite Operations</h3>
              <div className="perf-bar">
                <div className="perf-fill" style={{ width: '15%' }}>
                  <span>Nova: 180ms</span>
                </div>
              </div>
              <div className="perf-bar baseline">
                <div className="perf-fill-baseline" style={{ width: '100%' }}>
                  <span>Node.js: 1,200ms</span>
                </div>
              </div>
              <p className="perf-result">6.7x faster</p>
            </div>

            <div className="perf-card">
              <h3>Memory Usage</h3>
              <div className="perf-bar">
                <div className="perf-fill" style={{ width: '36%' }}>
                  <span>Nova: 90MB</span>
                </div>
              </div>
              <div className="perf-bar baseline">
                <div className="perf-fill-baseline" style={{ width: '100%' }}>
                  <span>Node.js: 250MB</span>
                </div>
              </div>
              <p className="perf-result">64% less memory</p>
            </div>

            <div className="perf-card">
              <h3>Startup Time</h3>
              <div className="perf-bar">
                <div className="perf-fill" style={{ width: '25%' }}>
                  <span>Nova: 5ms</span>
                </div>
              </div>
              <div className="perf-bar baseline">
                <div className="perf-fill-baseline" style={{ width: '100%' }}>
                  <span>Node.js: 20ms</span>
                </div>
              </div>
              <p className="perf-result">4x faster</p>
            </div>
          </div>
          <div className="cta-center">
            <Link to="/benchmarks" className="btn btn-primary">
              View All Benchmarks
            </Link>
          </div>
        </div>
      </section>

      {/* Quick Start */}
      <section className="quick-start section">
        <div className="container">
          <h2 className="section-title">Get Started in Seconds</h2>
          <div className="quick-start-grid">
            <div className="quick-start-step">
              <div className="step-number">1</div>
              <h3>Download</h3>
              <p>Download Nova for your platform</p>
              <code>wget nova-installer.sh</code>
            </div>

            <div className="quick-start-step">
              <div className="step-number">2</div>
              <h3>Install</h3>
              <p>Run the installer</p>
              <code>sh nova-installer.sh</code>
            </div>

            <div className="quick-start-step">
              <div className="step-number">3</div>
              <h3>Run</h3>
              <p>Execute your TypeScript code</p>
              <code>nova run app.ts</code>
            </div>
          </div>
        </div>
      </section>

      {/* CTA Section */}
      <section className="cta-section">
        <div className="container">
          <h2>Ready to experience native performance?</h2>
          <p>Join developers who are already building faster applications with Nova</p>
          <div className="cta-buttons">
            <Link to="/download" className="btn btn-primary">
              Download Nova
            </Link>
            <a
              href="https://github.com/d-osc/nova-lang"
              className="btn btn-secondary"
              target="_blank"
              rel="noopener noreferrer"
            >
              View on GitHub
            </a>
          </div>
        </div>
      </section>
    </div>
  );
}

export default Home;
