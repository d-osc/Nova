import React from 'react';
import { Link } from 'react-router-dom';
import { useLanguage } from '../contexts/LanguageContext';
import './Home.css';

function Home() {
  const { t } = useLanguage();

  return (
    <div className="home">
      {/* Hero Section */}
      <section className="hero">
        <div className="container">
          <div className="hero-content">
            <h1 className="hero-title">
              <span className="gradient-text">{t('home.hero.title')}</span>
              <br />
              {t('home.hero.subtitle')}
            </h1>
            <p className="hero-subtitle">
              {t('home.hero.description')}
              <br />
              <strong>{t('home.hero.descriptionHighlight')}</strong>, {t('home.hero.descriptionEnd')}
            </p>
            <div className="hero-buttons">
              <a href="#install" className="btn btn-primary">
                {t('home.hero.getStarted')}
              </a>
              <Link to="/docs" className="btn btn-secondary">
                {t('home.hero.documentation')}
              </Link>
            </div>
            <div className="hero-stats">
              <div className="stat">
                <div className="stat-value">5-10x</div>
                <div className="stat-label">{t('home.hero.stats.sqlite')}</div>
              </div>
              <div className="stat">
                <div className="stat-value">511</div>
                <div className="stat-label">{t('home.hero.stats.tests')}</div>
              </div>
              <div className="stat">
                <div className="stat-value">40+</div>
                <div className="stat-label">{t('home.hero.stats.modules')}</div>
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
            {t('home.features.title')} <span className="gradient-text">{t('home.hero.title')}</span>?
          </h2>
          <div className="features-grid">
            <div className="feature-card">
              <div className="feature-icon">⚡</div>
              <h3>{t('home.features.ultraPerformance.title')}</h3>
              <p>
                {t('home.features.ultraPerformance.description')}
              </p>
            </div>

            <div className="feature-card">
              <div className="feature-icon">🔧</div>
              <h3>{t('home.features.nodejsCompatible.title')}</h3>
              <p>
                {t('home.features.nodejsCompatible.description')}
              </p>
            </div>

            <div className="feature-card">
              <div className="feature-icon">📦</div>
              <h3>{t('home.features.packageManager.title')}</h3>
              <p>
                {t('home.features.packageManager.description')}
              </p>
            </div>

            <div className="feature-card">
              <div className="feature-icon">💾</div>
              <h3>{t('home.features.lowMemory.title')}</h3>
              <p>
                {t('home.features.lowMemory.description')}
              </p>
            </div>

            <div className="feature-card">
              <div className="feature-icon">🚀</div>
              <h3>{t('home.features.fastStartup.title')}</h3>
              <p>
                {t('home.features.fastStartup.description')}
              </p>
            </div>

            <div className="feature-card">
              <div className="feature-icon">✅</div>
              <h3>{t('home.features.battleTested.title')}</h3>
              <p>
                {t('home.features.battleTested.description')}
              </p>
            </div>
          </div>
        </div>
      </section>

      {/* Performance Highlights */}
      <section className="performance section">
        <div className="container">
          <h2 className="section-title">
            <span className="gradient-text">{t('home.performance.title')}</span> {t('home.performance.subtitle')}
          </h2>
          <div className="perf-grid">
            <div className="perf-card">
              <h3>{t('home.performance.sqlite.title')}</h3>
              <div className="perf-bar">
                <div className="perf-fill" style={{ width: '15%' }}>
                  <span>{t('home.performance.sqlite.nova')}</span>
                </div>
              </div>
              <div className="perf-bar baseline">
                <div className="perf-fill-baseline" style={{ width: '100%' }}>
                  <span>{t('home.performance.sqlite.nodejs')}</span>
                </div>
              </div>
              <p className="perf-result">{t('home.performance.sqlite.result')}</p>
            </div>

            <div className="perf-card">
              <h3>{t('home.performance.memory.title')}</h3>
              <div className="perf-bar">
                <div className="perf-fill" style={{ width: '36%' }}>
                  <span>{t('home.performance.memory.nova')}</span>
                </div>
              </div>
              <div className="perf-bar baseline">
                <div className="perf-fill-baseline" style={{ width: '100%' }}>
                  <span>{t('home.performance.memory.nodejs')}</span>
                </div>
              </div>
              <p className="perf-result">{t('home.performance.memory.result')}</p>
            </div>

            <div className="perf-card">
              <h3>{t('home.performance.startup.title')}</h3>
              <div className="perf-bar">
                <div className="perf-fill" style={{ width: '25%' }}>
                  <span>{t('home.performance.startup.nova')}</span>
                </div>
              </div>
              <div className="perf-bar baseline">
                <div className="perf-fill-baseline" style={{ width: '100%' }}>
                  <span>{t('home.performance.startup.nodejs')}</span>
                </div>
              </div>
              <p className="perf-result">{t('home.performance.startup.result')}</p>
            </div>
          </div>
          <div className="cta-center">
            <Link to="/benchmarks" className="btn btn-primary">
              {t('home.performance.viewBenchmarks')}
            </Link>
          </div>
        </div>
      </section>

      {/* Quick Start */}
      <section id="install" className="quick-start section">
        <div className="container">
          <h2 className="section-title">{t('home.quickStart.title')}</h2>

          <div className="install-commands">
            <div className="install-section">
              <h3>macOS / Linux</h3>
              <div className="code-block">
                <code>curl -fsSL https://raw.githubusercontent.com/d-osc/Nova/master/scripts/install.sh | bash</code>
              </div>
            </div>

            <div className="install-section">
              <h3>Windows</h3>
              <div className="code-block">
                <code>powershell -c "irm https://raw.githubusercontent.com/d-osc/Nova/master/scripts/install.ps1 | iex"</code>
              </div>
            </div>
          </div>

          <div className="quick-start-grid">
            <div className="quick-start-step">
              <div className="step-number">1</div>
              <h3>{t('home.quickStart.step1.title')}</h3>
              <p>One-liner installation - no dependencies needed</p>
              <code>curl ... | bash</code>
            </div>

            <div className="quick-start-step">
              <div className="step-number">2</div>
              <h3>Verify</h3>
              <p>Check Nova is installed correctly</p>
              <code>nova --version</code>
            </div>

            <div className="quick-start-step">
              <div className="step-number">3</div>
              <h3>{t('home.quickStart.step3.title')}</h3>
              <p>{t('home.quickStart.step3.description')}</p>
              <code>nova run app.ts</code>
            </div>
          </div>
        </div>
      </section>

      {/* CTA Section */}
      <section className="cta-section">
        <div className="container">
          <h2>{t('home.cta.title')}</h2>
          <p>{t('home.cta.description')}</p>
          <div className="cta-buttons">
            <a href="#install" className="btn btn-primary">
              {t('home.cta.download')}
            </a>
            <a
              href="https://github.com/d-osc/Nova"
              className="btn btn-secondary"
              target="_blank"
              rel="noopener noreferrer"
            >
              {t('home.cta.viewGithub')}
            </a>
          </div>
        </div>
      </section>
    </div>
  );
}

export default Home;
