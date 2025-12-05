import React, { useState } from 'react';
import { Link } from 'react-router-dom';
import './Navbar.css';

function Navbar() {
  const [isOpen, setIsOpen] = useState(false);

  return (
    <nav className="navbar">
      <div className="container">
        <div className="nav-wrapper">
          <Link to="/" className="logo">
            <span className="logo-text gradient-text">Nova</span>
          </Link>

          <div className={`nav-menu ${isOpen ? 'active' : ''}`}>
            <Link to="/" className="nav-link" onClick={() => setIsOpen(false)}>
              Home
            </Link>
            <Link to="/features" className="nav-link" onClick={() => setIsOpen(false)}>
              Features
            </Link>
            <Link to="/benchmarks" className="nav-link" onClick={() => setIsOpen(false)}>
              Benchmarks
            </Link>
            <Link to="/docs" className="nav-link" onClick={() => setIsOpen(false)}>
              Docs
            </Link>
            <Link to="/download" className="nav-link" onClick={() => setIsOpen(false)}>
              Download
            </Link>
            <a
              href="https://github.com/d-osc/nova-lang"
              className="nav-link"
              target="_blank"
              rel="noopener noreferrer"
            >
              GitHub
            </a>
          </div>

          <button
            className="hamburger"
            onClick={() => setIsOpen(!isOpen)}
            aria-label="Toggle menu"
          >
            <span className={isOpen ? 'active' : ''}></span>
            <span className={isOpen ? 'active' : ''}></span>
            <span className={isOpen ? 'active' : ''}></span>
          </button>
        </div>
      </div>
    </nav>
  );
}

export default Navbar;
