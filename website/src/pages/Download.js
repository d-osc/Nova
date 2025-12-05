import React from 'react';
import './Download.css';

function Download() {
  return (
    <div className="download-page">
      <section className="page-hero">
        <div className="container">
          <h1 className="page-title">
            <span className="gradient-text">Download</span> Nova
          </h1>
          <p className="page-subtitle">
            Get started with Nova on your platform
          </p>
        </div>
      </section>

      <section className="section">
        <div className="container">
          <div className="download-grid">
            <div className="download-card">
              <div className="platform-icon">🪟</div>
              <h3>Windows</h3>
              <p>Windows 10/11 (x64)</p>
              <div className="download-info">
                <span className="version">v1.0.0</span>
                <span className="size">~27 MB</span>
              </div>
              <a
                href="https://github.com/d-osc/nova-lang/releases/latest/download/nova-windows-x64.exe"
                className="btn btn-primary download-btn"
                download
              >
                Download for Windows
              </a>
            </div>

            <div className="download-card">
              <div className="platform-icon">🍎</div>
              <h3>macOS</h3>
              <p>macOS 11+ (Intel & Apple Silicon)</p>
              <div className="download-info">
                <span className="version">v1.0.0</span>
                <span className="size">~34 MB</span>
              </div>
              <a
                href="https://github.com/d-osc/nova-lang/releases/latest/download/nova-macos-universal.tar.gz"
                className="btn btn-primary download-btn"
                download
              >
                Download for macOS
              </a>
            </div>

            <div className="download-card">
              <div className="platform-icon">🐧</div>
              <h3>Linux</h3>
              <p>Ubuntu 20.04+ / Debian 11+ (x64)</p>
              <div className="download-info">
                <span className="version">v1.0.0</span>
                <span className="size">~34 MB</span>
              </div>
              <a
                href="https://github.com/d-osc/nova-lang/releases/latest/download/nova-linux-x64"
                className="btn btn-primary download-btn"
                download
              >
                Download for Linux
              </a>
            </div>
          </div>

          <div className="quick-install">
            <h2>🚀 Quick Install (Recommended)</h2>
            <p className="section-intro">
              Automated one-command installation for all platforms. Installs dependencies, builds from source, and sets up PATH automatically.
            </p>

            <div className="install-tabs">
              <div className="instruction-section featured">
                <h3>🪟 Windows (PowerShell as Administrator)</h3>
                <div className="code-block">
                  <pre>
                    <code>{`# Clone repository
git clone https://github.com/d-osc/nova-lang.git
cd Nova

# Run automated installer
.\\install-windows.ps1

# Verify installation
nova --version`}</code>
                  </pre>
                </div>
                <div className="install-features">
                  <span className="feature-badge">✓ Installs Visual Studio Build Tools</span>
                  <span className="feature-badge">✓ Installs LLVM 18</span>
                  <span className="feature-badge">✓ Auto-configures PATH</span>
                </div>
              </div>

              <div className="instruction-section featured">
                <h3>🍎 macOS</h3>
                <div className="code-block">
                  <pre>
                    <code>{`# Clone repository
git clone https://github.com/d-osc/nova-lang.git
cd Nova

# Run automated installer
./install.sh
# Or specifically: ./install-macos.sh

# Verify installation
nova --version`}</code>
                  </pre>
                </div>
                <div className="install-features">
                  <span className="feature-badge">✓ Installs Homebrew if needed</span>
                  <span className="feature-badge">✓ Installs LLVM 18</span>
                  <span className="feature-badge">✓ Works on Intel & Apple Silicon</span>
                </div>
              </div>

              <div className="instruction-section featured">
                <h3>🐧 Linux</h3>
                <div className="code-block">
                  <pre>
                    <code>{`# Clone repository
git clone https://github.com/d-osc/nova-lang.git
cd Nova

# Run automated installer
./install.sh
# Or specifically: ./install-linux.sh

# Verify installation
nova --version`}</code>
                  </pre>
                </div>
                <div className="install-features">
                  <span className="feature-badge">✓ Supports Ubuntu, Debian, Fedora, Arch</span>
                  <span className="feature-badge">✓ Auto-detects package manager</span>
                  <span className="feature-badge">✓ Installs LLVM 18</span>
                </div>
              </div>
            </div>
          </div>

          <div className="install-instructions">
            <h2>Alternative Installation Methods</h2>

            <div className="instruction-section">
              <h3>📦 Binary Downloads</h3>
              <p>Pre-built binaries for quick installation without building from source:</p>
              <div className="code-block">
                <pre>
                  <code>{`# Windows
Invoke-WebRequest -Uri "https://github.com/d-osc/nova-lang/releases/latest/download/nova-windows-x64.exe" -OutFile "nova.exe"

# macOS
curl -L https://github.com/d-osc/nova-lang/releases/latest/download/nova-macos-universal.tar.gz | tar xz
sudo mv nova /usr/local/bin/

# Linux
wget https://github.com/d-osc/nova-lang/releases/latest/download/nova-linux-x64
chmod +x nova-linux-x64
sudo mv nova-linux-x64 /usr/local/bin/nova`}</code>
                </pre>
              </div>
            </div>

            <div className="instruction-section">
              <h3>🍺 Package Managers</h3>
              <div className="code-block">
                <pre>
                  <code>{`# macOS - Homebrew (coming soon)
brew tap d-osc/nova-lang
brew install nova

# Linux - Snap (coming soon)
snap install nova

# Windows - Chocolatey (coming soon)
choco install nova`}</code>
                </pre>
              </div>
            </div>
          </div>

          <div className="build-from-source">
            <h2>Build from Source</h2>

            <div className="build-methods">
              <div className="build-method recommended">
                <h3>🎯 Automated Build (Recommended)</h3>
                <p>Our installer scripts handle everything automatically:</p>
                <div className="code-block">
                  <pre>
                    <code>{`# Clone repository
git clone https://github.com/d-osc/nova-lang.git
cd Nova

# Run installer (detects OS automatically)
./install.sh              # Linux/macOS
.\\install-windows.ps1     # Windows

# What it does:
# ✓ Installs all dependencies (LLVM, CMake, compilers)
# ✓ Configures build with optimal settings
# ✓ Builds Nova with Release optimizations
# ✓ Installs to system PATH
# ✓ Verifies installation`}</code>
                  </pre>
                </div>
                <p className="note">
                  <strong>Installation time:</strong> 15-30 minutes (includes downloading dependencies)
                </p>
              </div>

              <div className="build-method">
                <h3>⚙️ Manual Build</h3>
                <p>For advanced users who want full control:</p>
                <div className="code-block">
                  <pre>
                    <code>{`# Prerequisites:
# - LLVM 18.1.7
# - CMake 3.20+
# - C++20 compiler (GCC 11+, Clang 14+, MSVC 2019+)
# - Ninja build system

# Clone and build
git clone https://github.com/d-osc/nova-lang.git
cd Nova

# Configure
cmake -B build -G Ninja \\
  -DCMAKE_BUILD_TYPE=Release \\
  -DCMAKE_CXX_STANDARD=20

# Build (use -j for parallel compilation)
cmake --build build --config Release -j$(nproc)

# Install (optional)
sudo cmake --install build

# Binary location:
# Linux/macOS: build/nova
# Windows: build/Release/nova.exe`}</code>
                  </pre>
                </div>
              </div>
            </div>

            <div className="build-resources">
              <h3>📚 Build Resources</h3>
              <div className="resource-links">
                <a
                  href="https://github.com/d-osc/nova-lang/blob/master/INSTALL.md"
                  className="resource-link"
                  target="_blank"
                  rel="noopener noreferrer"
                >
                  📖 Complete Installation Guide
                </a>
                <a
                  href="https://github.com/d-osc/nova-lang/blob/master/docs/BUILD.md"
                  className="resource-link"
                  target="_blank"
                  rel="noopener noreferrer"
                >
                  🔧 Detailed Build Instructions
                </a>
                <a
                  href="https://github.com/d-osc/nova-lang/blob/master/CONTRIBUTING.md"
                  className="resource-link"
                  target="_blank"
                  rel="noopener noreferrer"
                >
                  🤝 Contributing Guide
                </a>
              </div>
            </div>
          </div>

          <div className="system-requirements">
            <h2>System Requirements</h2>

            <div className="requirements-grid">
              <div className="requirement-card">
                <h4>Minimum Requirements</h4>
                <ul>
                  <li><strong>OS:</strong> Windows 10, macOS 11, Ubuntu 20.04</li>
                  <li><strong>CPU:</strong> 64-bit x86 or ARM</li>
                  <li><strong>RAM:</strong> 2 GB</li>
                  <li><strong>Disk:</strong> 100 MB free space</li>
                </ul>
              </div>

              <div className="requirement-card">
                <h4>Recommended Requirements</h4>
                <ul>
                  <li><strong>OS:</strong> Windows 11, macOS 13+, Ubuntu 22.04+</li>
                  <li><strong>CPU:</strong> Multi-core processor</li>
                  <li><strong>RAM:</strong> 4 GB or more</li>
                  <li><strong>Disk:</strong> 500 MB free space</li>
                </ul>
              </div>
            </div>
          </div>

          <div className="next-steps">
            <h2>Next Steps</h2>
            <div className="steps-grid">
              <div className="step-card">
                <div className="step-number">1</div>
                <h4>Read the Documentation</h4>
                <p>Learn about Nova's features and how to use them effectively</p>
                <a href="/docs" className="step-link">
                  Go to Docs →
                </a>
              </div>

              <div className="step-card">
                <div className="step-number">2</div>
                <h4>Try Examples</h4>
                <p>Explore sample projects to see Nova in action</p>
                <a
                  href="https://github.com/d-osc/nova-lang/tree/master/examples"
                  className="step-link"
                  target="_blank"
                  rel="noopener noreferrer"
                >
                  View Examples →
                </a>
              </div>

              <div className="step-card">
                <div className="step-number">3</div>
                <h4>Join the Community</h4>
                <p>Connect with other Nova developers and contributors</p>
                <a
                  href="https://github.com/d-osc/nova-lang/discussions"
                  className="step-link"
                  target="_blank"
                  rel="noopener noreferrer"
                >
                  Join Discussion →
                </a>
              </div>
            </div>
          </div>

          <div className="download-cta">
            <h2>Need Help?</h2>
            <p>Check out our documentation or open an issue on GitHub</p>
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
                href="https://github.com/d-osc/nova-lang/issues"
                className="btn btn-secondary"
                target="_blank"
                rel="noopener noreferrer"
              >
                Report Issue
              </a>
            </div>
          </div>
        </div>
      </section>
    </div>
  );
}

export default Download;
