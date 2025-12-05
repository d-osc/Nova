const ghPages = require('gh-pages');
const path = require('path');

console.log('Starting deployment to GitHub Pages...');

// Configure options for gh-pages with better error handling
const options = {
  dest: 'dist',
  dotfiles: true,
  // Add specific files to remove that might cause issues
  remove: [
    '.DS_Store',
    'Thumbs.db',
    'node_modules',
    '.git'
  ],
  // Increase memory and timeout to handle large projects
  maxBuffer: 1024 * 1024 * 10, // 10MB buffer
  // Set a custom user agent to avoid some git issues
  user: {
    name: 'Nova Website Bot',
    email: 'noreply@nova-lang.org'
  }
};

// Deploy with error handling
ghPages.publish('build', options, (err) => {
  if (err) {
    console.error('Deployment failed:', err);
    console.log('\nTrying alternative deployment method...');
    
    // Try with minimal options
    ghPages.publish('build', { 
      dest: 'dist',
      dotfiles: true 
    }, (altErr) => {
      if (altErr) {
        console.error('Alternative deployment also failed:', altErr);
        process.exit(1);
      } else {
        console.log('Alternative deployment successful!');
        process.exit(0);
      }
    });
  } else {
    console.log('Deployment successful!');
    process.exit(0);
  }
});