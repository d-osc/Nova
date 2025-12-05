import React, { useState } from 'react';
import { Link } from 'react-router-dom';
import { useLanguage } from '../contexts/LanguageContext';
import LanguageSwitcher from './LanguageSwitcher';
import './Navbar.css';

function Navbar() {
  const [isOpen, setIsOpen] = useState(false);
  const { t } = useLanguage();

  return (
    <nav className="navbar">
      <div className="container">
        <div className="nav-wrapper">
          <Link to="/" className="logo">
            <span className="logo-text gradient-text">Nova</span>
          </Link>

          <div className={`nav-menu ${isOpen ? 'active' : ''}`}>
            <Link to="/" className="nav-link" onClick={() => setIsOpen(false)}>
              {t('nav.home')}
            </Link>
            <Link to="/features" className="nav-link" onClick={() => setIsOpen(false)}>
              {t('nav.features')}
            </Link>
            <Link to="/benchmarks" className="nav-link" onClick={() => setIsOpen(false)}>
              {t('nav.benchmarks')}
            </Link>
            <Link to="/docs" className="nav-link" onClick={() => setIsOpen(false)}>
              {t('nav.docs')}
            </Link>
            <a
              href="https://github.com/d-osc/Nova"
              className="nav-link"
              target="_blank"
              rel="noopener noreferrer"
            >
              GitHub
            </a>
            <LanguageSwitcher />
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
