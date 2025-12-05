import React from 'react';
import { BrowserRouter as Router, Routes, Route } from 'react-router-dom';
import './App.css';
import Navbar from './components/Navbar';
import Footer from './components/Footer';
import Home from './pages/Home';
import Features from './pages/Features';
import Benchmarks from './pages/Benchmarks';
import Docs from './pages/Docs';
import Download from './pages/Download';

function App() {
  return (
    <Router basename="/Nova">
      <div className="App">
        <Navbar />
        <Routes>
          <Route path="/" element={<Home />} />
          <Route path="/features" element={<Features />} />
          <Route path="/benchmarks" element={<Benchmarks />} />
          <Route path="/docs" element={<Docs />} />
          <Route path="/download" element={<Download />} />
        </Routes>
        <Footer />
      </div>
    </Router>
  );
}

export default App;
