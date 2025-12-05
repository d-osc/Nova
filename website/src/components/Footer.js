import React from 'react';
import './Footer.css';

function Footer() {
  return (
    <footer className="footer">
      <div className="container">
        <div className="footer-content">
          <div className="footer-section">
            <h3 className="gradient-text">Nova</h3>
            <p>TypeScript/JavaScript to Native Code Compiler</p>
            <p className="powered-by">Powered by LLVM 18.1.7</p>
          </div>

          <div className="footer-section">
            <h4>Resources</h4>
            <a href="https://github.com/d-osc/Nova" target="_blank" rel="noopener noreferrer">
              GitHub
            </a>
            <a href="https://github.com/d-osc/Nova/tree/master/docs" target="_blank" rel="noopener noreferrer">
              Documentation
            </a>
            <a href="https://github.com/d-osc/Nova/releases" target="_blank" rel="noopener noreferrer">
              Releases
            </a>
          </div>

          <div className="footer-section">
            <h4>Community</h4>
            <a href="https://github.com/d-osc/Nova/issues" target="_blank" rel="noopener noreferrer">
              Issues
            </a>
            <a href="https://github.com/d-osc/Nova/discussions" target="_blank" rel="noopener noreferrer">
              Discussions
            </a>
            <a href="https://github.com/d-osc/Nova/blob/master/CONTRIBUTING.md" target="_blank" rel="noopener noreferrer">
              Contributing
            </a>
          </div>

          <div className="footer-section">
            <h4>Legal</h4>
            <a href="https://github.com/d-osc/Nova/blob/master/LICENSE" target="_blank" rel="noopener noreferrer">
              MIT License
            </a>
          </div>
        </div>

        <div className="footer-bottom">
          <p>&copy; 2025 Nova Language. All rights reserved.</p>
          <p>Built with ❤️ using React</p>
        </div>
      </div>
    </footer>
  );
}

export default Footer;
