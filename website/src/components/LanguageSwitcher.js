import React from 'react';
import { useLanguage } from '../contexts/LanguageContext';
import './LanguageSwitcher.css';

function LanguageSwitcher() {
  const { language, changeLanguage } = useLanguage();

  return (
    <div className="language-switcher">
      <button
        className={`lang-btn ${language === 'en' ? 'active' : ''}`}
        onClick={() => changeLanguage('en')}
        aria-label="Switch to English"
      >
        EN
      </button>
      <button
        className={`lang-btn ${language === 'th' ? 'active' : ''}`}
        onClick={() => changeLanguage('th')}
        aria-label="Switch to Thai"
      >
        TH
      </button>
    </div>
  );
}

export default LanguageSwitcher;
