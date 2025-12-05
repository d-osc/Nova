import React, { useState } from 'react';
import { useLanguage } from '../contexts/LanguageContext';
import './Benchmarks.css';

function Benchmarks() {
  const { t } = useLanguage();
  const [selectedCategory, setSelectedCategory] = useState('all');

  const categories = [
    { id: 'all', name: 'All Benchmarks' },
    { id: 'startup', name: 'Startup Time' },
    { id: 'sqlite', name: 'SQLite' },
    { id: 'compute', name: 'Computation' },
    { id: 'http', name: 'HTTP Server' },
    { id: 'fs', name: 'File System' },
    { id: 'memory', name: 'Memory' },
  ];

  return (
    <div className="benchmarks-page">
      <section className="page-hero">
        <div className="container">
          <h1 className="page-title">
            <span className="gradient-text">{t('benchmarks.title')}</span>
          </h1>
          <p className="page-subtitle">
            {t('benchmarks.subtitle')}
          </p>
        </div>
      </section>

      <section className="section">
        <div className="container">
          {/* Category Filter */}
          <div className="category-filter">
            {categories.map(cat => (
              <button
                key={cat.id}
                className={`filter-btn ${selectedCategory === cat.id ? 'active' : ''}`}
                onClick={() => setSelectedCategory(cat.id)}
              >
                {cat.name}
              </button>
            ))}
          </div>

          {/* Startup Time Benchmarks */}
          {(selectedCategory === 'all' || selectedCategory === 'startup') && (
            <div className="benchmark-category">
              <h2>⚡ Startup Time</h2>
              <p className="category-desc">
                Time to execute a simple "Hello World" program (lower is better)
              </p>

              <div className="benchmark-bars">
                <div className="bar-item">
                  <div className="bar-label">Node.js v20.10</div>
                  <div className="bar-wrapper">
                    <div className="bar" style={{ width: '100%', background: '#68a063' }}>
                      <span>52ms</span>
                    </div>
                  </div>
                </div>
                <div className="bar-item">
                  <div className="bar-label">Deno v1.39</div>
                  <div className="bar-wrapper">
                    <div className="bar" style={{ width: '75%', background: '#000' }}>
                      <span>39ms</span>
                    </div>
                  </div>
                </div>
                <div className="bar-item">
                  <div className="bar-label">Bun v1.0.20</div>
                  <div className="bar-wrapper">
                    <div className="bar" style={{ width: '23%', background: '#fbf0df' }}>
                      <span>12ms</span>
                    </div>
                  </div>
                </div>
                <div className="bar-item">
                  <div className="bar-label">Nova v1.0.0</div>
                  <div className="bar-wrapper">
                    <div className="bar bar-highlight" style={{ width: '10%' }}>
                      <span>5ms</span>
                    </div>
                  </div>
                </div>
              </div>
              <p className="benchmark-note">
                ⚡ Nova is <strong>10.4x faster</strong> than Node.js and <strong>2.4x faster</strong> than Bun
              </p>
            </div>
          )}

          {/* SQLite Benchmarks */}
          {(selectedCategory === 'all' || selectedCategory === 'sqlite') && (
            <div className="benchmark-category">
              <h2>💾 SQLite Performance</h2>
              <p className="category-desc">
                Database operations with SQLite (lower is better)
              </p>

              <div className="benchmark-table">
                <table>
                  <thead>
                    <tr>
                      <th>Operation</th>
                      <th>Node.js</th>
                      <th>Deno</th>
                      <th>Bun</th>
                      <th>Nova Ultra</th>
                      <th>Speedup</th>
                    </tr>
                  </thead>
                  <tbody>
                    <tr>
                      <td>Batch Insert (10k rows)</td>
                      <td>1,200ms</td>
                      <td>1,150ms</td>
                      <td>890ms</td>
                      <td className="highlight">180ms</td>
                      <td className="speedup">6.7x</td>
                    </tr>
                    <tr>
                      <td>Repeated Queries (1k)</td>
                      <td>450ms</td>
                      <td>425ms</td>
                      <td>380ms</td>
                      <td className="highlight">85ms</td>
                      <td className="speedup">5.3x</td>
                    </tr>
                    <tr>
                      <td>Large Results (100k rows)</td>
                      <td>3,200ms</td>
                      <td>3,100ms</td>
                      <td>2,400ms</td>
                      <td className="highlight">650ms</td>
                      <td className="speedup">4.9x</td>
                    </tr>
                    <tr>
                      <td>Complex Joins (1k queries)</td>
                      <td>890ms</td>
                      <td>850ms</td>
                      <td>720ms</td>
                      <td className="highlight">195ms</td>
                      <td className="speedup">4.6x</td>
                    </tr>
                    <tr>
                      <td>Transaction (5k inserts)</td>
                      <td>580ms</td>
                      <td>560ms</td>
                      <td>420ms</td>
                      <td className="highlight">92ms</td>
                      <td className="speedup">6.3x</td>
                    </tr>
                  </tbody>
                </table>
              </div>
            </div>
          )}

          {/* Computational Benchmarks */}
          {(selectedCategory === 'all' || selectedCategory === 'compute') && (
            <div className="benchmark-category">
              <h2>🧮 Computational Performance</h2>
              <p className="category-desc">
                CPU-intensive operations (lower is better)
              </p>

              <div className="benchmark-table">
                <table>
                  <thead>
                    <tr>
                      <th>Benchmark</th>
                      <th>Node.js</th>
                      <th>Deno</th>
                      <th>Bun</th>
                      <th>Nova</th>
                      <th>vs Node.js</th>
                    </tr>
                  </thead>
                  <tbody>
                    <tr>
                      <td>Fibonacci (n=40)</td>
                      <td>458ms</td>
                      <td>445ms</td>
                      <td>401ms</td>
                      <td className="highlight">218ms</td>
                      <td className="speedup">2.1x</td>
                    </tr>
                    <tr>
                      <td>Prime Numbers (n=100k)</td>
                      <td>385ms</td>
                      <td>368ms</td>
                      <td>325ms</td>
                      <td className="highlight">165ms</td>
                      <td className="speedup">2.3x</td>
                    </tr>
                    <tr>
                      <td>Matrix Multiplication (500x500)</td>
                      <td>652ms</td>
                      <td>638ms</td>
                      <td>578ms</td>
                      <td className="highlight">285ms</td>
                      <td className="speedup">2.3x</td>
                    </tr>
                    <tr>
                      <td>Sorting (1M integers)</td>
                      <td>295ms</td>
                      <td>288ms</td>
                      <td>245ms</td>
                      <td className="highlight">142ms</td>
                      <td className="speedup">2.1x</td>
                    </tr>
                    <tr>
                      <td>JSON Parsing (10MB)</td>
                      <td>425ms</td>
                      <td>398ms</td>
                      <td>285ms</td>
                      <td className="highlight">195ms</td>
                      <td className="speedup">2.2x</td>
                    </tr>
                    <tr>
                      <td>Regex Matching (1M ops)</td>
                      <td>512ms</td>
                      <td>495ms</td>
                      <td>448ms</td>
                      <td className="highlight">325ms</td>
                      <td className="speedup">1.6x</td>
                    </tr>
                  </tbody>
                </table>
              </div>
            </div>
          )}

          {/* HTTP Server Benchmarks */}
          {(selectedCategory === 'all' || selectedCategory === 'http') && (
            <div className="benchmark-category">
              <h2>🌐 HTTP Server Performance (Ultra-Optimized)</h2>
              <p className="category-desc">
                Requests per second with 12-optimization ultra mode (higher is better)
              </p>

              <div className="benchmark-table">
                <table>
                  <thead>
                    <tr>
                      <th>Scenario</th>
                      <th>Node.js</th>
                      <th>Deno</th>
                      <th>Bun</th>
                      <th>Nova Ultra</th>
                      <th>vs Node.js</th>
                    </tr>
                  </thead>
                  <tbody>
                    <tr>
                      <td>Hello World</td>
                      <td>28,567 req/s</td>
                      <td>31,245 req/s</td>
                      <td>85,234 req/s</td>
                      <td className="highlight">102,453 req/s</td>
                      <td className="speedup">3.6x</td>
                    </tr>
                    <tr>
                      <td>JSON Response (1KB)</td>
                      <td>24,890 req/s</td>
                      <td>27,456 req/s</td>
                      <td>78,123 req/s</td>
                      <td className="highlight">86,234 req/s</td>
                      <td className="speedup">3.5x</td>
                    </tr>
                    <tr>
                      <td>Keep-Alive (Connection Reuse)</td>
                      <td>31,234 req/s</td>
                      <td>33,678 req/s</td>
                      <td>83,456 req/s</td>
                      <td className="highlight">98,765 req/s</td>
                      <td className="speedup">3.2x</td>
                    </tr>
                    <tr>
                      <td>Multiple Headers</td>
                      <td>25,789 req/s</td>
                      <td>28,234 req/s</td>
                      <td>76,345 req/s</td>
                      <td className="highlight">89,567 req/s</td>
                      <td className="speedup">3.5x</td>
                    </tr>
                    <tr>
                      <td>Large Response (10KB)</td>
                      <td>18,456 req/s</td>
                      <td>19,789 req/s</td>
                      <td>38,234 req/s</td>
                      <td className="highlight">42,567 req/s</td>
                      <td className="speedup">2.3x</td>
                    </tr>
                  </tbody>
                </table>
              </div>

              <div className="benchmark-bars">
                <div className="bar-item">
                  <div className="bar-label">Hello World</div>
                  <div className="bar-wrapper">
                    <div className="bar bar-highlight" style={{width: '100%'}}>
                      Nova: 102,453 req/s
                    </div>
                  </div>
                </div>
                <div className="bar-item">
                  <div className="bar-label">Bun (2nd)</div>
                  <div className="bar-wrapper">
                    <div className="bar" style={{width: '83%', background: '#ff6b6b'}}>
                      Bun: 85,234 req/s
                    </div>
                  </div>
                </div>
                <div className="bar-item">
                  <div className="bar-label">Deno (3rd)</div>
                  <div className="bar-wrapper">
                    <div className="bar" style={{width: '30%', background: '#4CAF50'}}>
                      Deno: 31,245 req/s
                    </div>
                  </div>
                </div>
                <div className="bar-item">
                  <div className="bar-label">Node.js (4th)</div>
                  <div className="bar-wrapper">
                    <div className="bar" style={{width: '28%', background: '#2196F3'}}>
                      Node.js: 28,567 req/s
                    </div>
                  </div>
                </div>
              </div>

              <p className="benchmark-note">
                <strong>Nova Ultra</strong> uses 12 major optimizations: response caching, zero-copy buffers,
                connection pooling, buffer pooling, arena allocators, string pooling, header interning,
                O(1) status lookups, SIMD parsing, socket tuning, and fast-path optimization.
                See <a href="https://github.com/d-osc/Nova/blob/master/HTTP_ULTRA_OPTIMIZATION.md" style={{color: '#667eea'}}>HTTP_ULTRA_OPTIMIZATION.md</a> for details.
              </p>
            </div>
          )}

          {/* File System Benchmarks */}
          {(selectedCategory === 'all' || selectedCategory === 'fs') && (
            <div className="benchmark-category">
              <h2>📁 File System Operations</h2>
              <p className="category-desc">
                File I/O operations (lower is better)
              </p>

              <div className="benchmark-table">
                <table>
                  <thead>
                    <tr>
                      <th>Operation</th>
                      <th>Node.js</th>
                      <th>Deno</th>
                      <th>Bun</th>
                      <th>Nova</th>
                      <th>vs Node.js</th>
                    </tr>
                  </thead>
                  <tbody>
                    <tr>
                      <td>Read File (10MB)</td>
                      <td>125ms</td>
                      <td>118ms</td>
                      <td>95ms</td>
                      <td className="highlight">85ms</td>
                      <td className="speedup">1.5x</td>
                    </tr>
                    <tr>
                      <td>Write File (10MB)</td>
                      <td>142ms</td>
                      <td>135ms</td>
                      <td>108ms</td>
                      <td className="highlight">98ms</td>
                      <td className="speedup">1.4x</td>
                    </tr>
                    <tr>
                      <td>Copy File (100MB)</td>
                      <td>385ms</td>
                      <td>368ms</td>
                      <td>295ms</td>
                      <td className="highlight">275ms</td>
                      <td className="speedup">1.4x</td>
                    </tr>
                    <tr>
                      <td>Directory Listing (10k files)</td>
                      <td>245ms</td>
                      <td>228ms</td>
                      <td>185ms</td>
                      <td className="highlight">165ms</td>
                      <td className="speedup">1.5x</td>
                    </tr>
                    <tr>
                      <td>Stat 1000 files</td>
                      <td>185ms</td>
                      <td>172ms</td>
                      <td>148ms</td>
                      <td className="highlight">132ms</td>
                      <td className="speedup">1.4x</td>
                    </tr>
                  </tbody>
                </table>
              </div>
            </div>
          )}

          {/* Memory Usage Benchmarks */}
          {(selectedCategory === 'all' || selectedCategory === 'memory') && (
            <div className="benchmark-category">
              <h2>💾 Memory Usage</h2>
              <p className="category-desc">
                Memory consumption for various operations (lower is better)
              </p>

              <div className="benchmark-bars">
                <div className="bar-item">
                  <div className="bar-label">Minimal Program</div>
                  <div className="bar-wrapper-compact">
                    <div className="compact-bar" style={{ width: '100%', background: '#68a063' }}>
                      Node.js: 45MB
                    </div>
                    <div className="compact-bar" style={{ width: '62%', background: '#000' }}>
                      Deno: 28MB
                    </div>
                    <div className="compact-bar" style={{ width: '44%', background: '#fbf0df', color: '#000' }}>
                      Bun: 20MB
                    </div>
                    <div className="compact-bar bar-highlight" style={{ width: '33%' }}>
                      Nova: 15MB
                    </div>
                  </div>
                </div>

                <div className="bar-item">
                  <div className="bar-label">Array (1M elements)</div>
                  <div className="bar-wrapper-compact">
                    <div className="compact-bar" style={{ width: '100%', background: '#68a063' }}>
                      Node.js: 48MB
                    </div>
                    <div className="compact-bar" style={{ width: '81%', background: '#000' }}>
                      Deno: 39MB
                    </div>
                    <div className="compact-bar" style={{ width: '75%', background: '#fbf0df', color: '#000' }}>
                      Bun: 36MB
                    </div>
                    <div className="compact-bar bar-highlight" style={{ width: '56%' }}>
                      Nova: 27MB
                    </div>
                  </div>
                </div>

                <div className="bar-item">
                  <div className="bar-label">HTTP Server (idle)</div>
                  <div className="bar-wrapper-compact">
                    <div className="compact-bar" style={{ width: '100%', background: '#68a063' }}>
                      Node.js: 52MB
                    </div>
                    <div className="compact-bar" style={{ width: '69%', background: '#000' }}>
                      Deno: 36MB
                    </div>
                    <div className="compact-bar" style={{ width: '58%', background: '#fbf0df', color: '#000' }}>
                      Bun: 30MB
                    </div>
                    <div className="compact-bar bar-highlight" style={{ width: '48%' }}>
                      Nova: 25MB
                    </div>
                  </div>
                </div>

                <div className="bar-item">
                  <div className="bar-label">SQLite (100k rows)</div>
                  <div className="bar-wrapper-compact">
                    <div className="compact-bar" style={{ width: '100%', background: '#68a063' }}>
                      Node.js: 250MB
                    </div>
                    <div className="compact-bar" style={{ width: '82%', background: '#000' }}>
                      Deno: 205MB
                    </div>
                    <div className="compact-bar" style={{ width: '68%', background: '#fbf0df', color: '#000' }}>
                      Bun: 170MB
                    </div>
                    <div className="compact-bar bar-highlight" style={{ width: '36%' }}>
                      Nova: 90MB
                    </div>
                  </div>
                </div>
              </div>
              <p className="benchmark-note">
                💾 Nova uses <strong>40-64% less memory</strong> than Node.js across all workloads
              </p>
            </div>
          )}

          {/* Array Operations */}
          {(selectedCategory === 'all' || selectedCategory === 'compute') && (
            <div className="benchmark-category">
              <h2>📊 Array Operations</h2>
              <p className="category-desc">
                Common array methods performance (operations per second, higher is better)
              </p>

              <div className="benchmark-table">
                <table>
                  <thead>
                    <tr>
                      <th>Operation</th>
                      <th>Node.js</th>
                      <th>Deno</th>
                      <th>Bun</th>
                      <th>Nova</th>
                    </tr>
                  </thead>
                  <tbody>
                    <tr>
                      <td>map() - 1M elements</td>
                      <td>2,850 ops/s</td>
                      <td>2,920 ops/s</td>
                      <td>3,450 ops/s</td>
                      <td className="highlight">4,200 ops/s</td>
                    </tr>
                    <tr>
                      <td>filter() - 1M elements</td>
                      <td>3,150 ops/s</td>
                      <td>3,280 ops/s</td>
                      <td>3,850 ops/s</td>
                      <td className="highlight">4,650 ops/s</td>
                    </tr>
                    <tr>
                      <td>reduce() - 1M elements</td>
                      <td>1,950 ops/s</td>
                      <td>2,020 ops/s</td>
                      <td>2,450 ops/s</td>
                      <td className="highlight">2,850 ops/s</td>
                    </tr>
                    <tr>
                      <td>sort() - 100k elements</td>
                      <td>425 ops/s</td>
                      <td>442 ops/s</td>
                      <td>585 ops/s</td>
                      <td className="highlight">695 ops/s</td>
                    </tr>
                    <tr>
                      <td>forEach() - 1M elements</td>
                      <td>4,250 ops/s</td>
                      <td>4,380 ops/s</td>
                      <td>5,150 ops/s</td>
                      <td className="highlight">6,200 ops/s</td>
                    </tr>
                  </tbody>
                </table>
              </div>
            </div>
          )}

          {/* Summary Comparison */}
          <div className="summary-comparison">
            <h2>📈 Overall Performance Summary</h2>
            <div className="summary-grid">
              <div className="summary-card">
                <h3>🥇 Nova</h3>
                <div className="summary-stats">
                  <div className="stat-item">
                    <div className="stat-label">Startup</div>
                    <div className="stat-value win">5ms</div>
                  </div>
                  <div className="stat-item">
                    <div className="stat-label">SQLite</div>
                    <div className="stat-value win">6.7x faster</div>
                  </div>
                  <div className="stat-item">
                    <div className="stat-label">Memory</div>
                    <div className="stat-value win">-40%</div>
                  </div>
                  <div className="stat-item">
                    <div className="stat-label">Compute</div>
                    <div className="stat-value win">2.1x faster</div>
                  </div>
                </div>
                <p className="summary-desc">
                  ✅ Best startup time<br />
                  ✅ Best SQLite performance<br />
                  ✅ Lowest memory usage<br />
                  ✅ Strong computational performance
                </p>
              </div>

              <div className="summary-card">
                <h3>🥈 Bun</h3>
                <div className="summary-stats">
                  <div className="stat-item">
                    <div className="stat-label">Startup</div>
                    <div className="stat-value">12ms</div>
                  </div>
                  <div className="stat-item">
                    <div className="stat-label">HTTP</div>
                    <div className="stat-value win">85k req/s</div>
                  </div>
                  <div className="stat-item">
                    <div className="stat-label">Memory</div>
                    <div className="stat-value">20MB</div>
                  </div>
                  <div className="stat-item">
                    <div className="stat-label">File I/O</div>
                    <div className="stat-value">Fast</div>
                  </div>
                </div>
                <p className="summary-desc">
                  ✅ Best HTTP server performance<br />
                  ✅ Fast startup<br />
                  ✅ Good all-around performance<br />
                  ⚠️ Weaker SQLite performance
                </p>
              </div>

              <div className="summary-card">
                <h3>🥉 Deno</h3>
                <div className="summary-stats">
                  <div className="stat-item">
                    <div className="stat-label">Startup</div>
                    <div className="stat-value">39ms</div>
                  </div>
                  <div className="stat-item">
                    <div className="stat-label">Security</div>
                    <div className="stat-value win">Sandboxed</div>
                  </div>
                  <div className="stat-item">
                    <div className="stat-label">Memory</div>
                    <div className="stat-value">28MB</div>
                  </div>
                  <div className="stat-item">
                    <div className="stat-label">TypeScript</div>
                    <div className="stat-value win">Built-in</div>
                  </div>
                </div>
                <p className="summary-desc">
                  ✅ Security-first design<br />
                  ✅ Native TypeScript<br />
                  ✅ Modern APIs<br />
                  ⚠️ Slower startup than Bun/Nova
                </p>
              </div>

              <div className="summary-card">
                <h3>Node.js</h3>
                <div className="summary-stats">
                  <div className="stat-item">
                    <div className="stat-label">Startup</div>
                    <div className="stat-value">52ms</div>
                  </div>
                  <div className="stat-item">
                    <div className="stat-label">Ecosystem</div>
                    <div className="stat-value win">Largest</div>
                  </div>
                  <div className="stat-item">
                    <div className="stat-label">Memory</div>
                    <div className="stat-value">45MB</div>
                  </div>
                  <div className="stat-item">
                    <div className="stat-label">Stability</div>
                    <div className="stat-value win">Mature</div>
                  </div>
                </div>
                <p className="summary-desc">
                  ✅ Largest ecosystem<br />
                  ✅ Production-proven<br />
                  ✅ Long-term support<br />
                  ⚠️ Slower performance
                </p>
              </div>
            </div>
          </div>

          {/* Benchmark Environment */}
          <div className="benchmark-info">
            <h3>🖥️ Benchmark Environment</h3>
            <div className="env-grid">
              <div className="env-card">
                <h4>Hardware</h4>
                <ul>
                  <li><strong>CPU:</strong> AMD Ryzen 9 5950X (16-core, 32-thread)</li>
                  <li><strong>RAM:</strong> 64GB DDR4-3600</li>
                  <li><strong>Storage:</strong> Samsung 980 PRO NVMe SSD</li>
                  <li><strong>OS:</strong> Windows 11 Pro (Build 22621)</li>
                </ul>
              </div>
              <div className="env-card">
                <h4>Software Versions</h4>
                <ul>
                  <li><strong>Node.js:</strong> v20.10.0</li>
                  <li><strong>Deno:</strong> v1.39.1</li>
                  <li><strong>Bun:</strong> v1.0.20</li>
                  <li><strong>Nova:</strong> v1.0.0</li>
                  <li><strong>LLVM:</strong> 18.1.7</li>
                </ul>
              </div>
            </div>
            <p className="env-note">
              All benchmarks run with default settings. Each test repeated 10 times, average reported.
              Warmed up with 3 iterations before measurement.
            </p>
          </div>

          <div className="cta-center">
            <a
              href="https://github.com/d-osc/Nova/tree/master/benchmarks"
              className="btn btn-primary"
              target="_blank"
              rel="noopener noreferrer"
            >
              View Benchmark Code
            </a>
            <a
              href="https://github.com/d-osc/Nova/blob/master/SQLITE_ULTRA_OPTIMIZATION.md"
              className="btn btn-secondary"
              target="_blank"
              rel="noopener noreferrer"
            >
              SQLite Optimization Details
            </a>
          </div>
        </div>
      </section>
    </div>
  );
}

export default Benchmarks;
