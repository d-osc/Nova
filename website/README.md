# Nova Runtime Website

Official React website for Nova - TypeScript/JavaScript to Native Code Compiler

## Features

- 🎨 Modern, responsive design
- ⚡ Fast and lightweight
- 📱 Mobile-friendly
- 🌐 GitHub Pages ready
- 🎯 Performance benchmarks showcase
- 📚 Comprehensive documentation
- 💾 Download page with install instructions

## Pages

- **Home** - Hero section, features overview, performance highlights
- **Features** - Detailed feature showcase with code examples
- **Benchmarks** - Performance comparisons with Node.js and Bun
- **Docs** - Documentation links and quick reference
- **Download** - Download links and installation instructions

## Local Development

### Prerequisites

- Node.js 16+ and npm

### Installation

```bash
# Navigate to website directory
cd website

# Install dependencies
npm install

# Start development server
npm start
```

The website will open at `http://localhost:3000`

## Building for Production

```bash
# Build the website
npm run build

# The build files will be in the `build/` directory
```

## Deploying to GitHub Pages

### One-Time Setup

1. **Update `package.json`** - Change the `homepage` field:
   ```json
   "homepage": "https://d-osc.github.io/nova-lang"
   ```

2. **Install gh-pages** (if not already installed):
   ```bash
   npm install --save-dev gh-pages
   ```

### Deploy

```bash
# Deploy to GitHub Pages
npm run deploy
```

This will:
1. Build the website
2. Push the build to the `gh-pages` branch
3. GitHub will automatically host it

### Configure GitHub Pages

1. Go to your repository on GitHub
2. Navigate to **Settings** → **Pages**
3. Under **Source**, select branch `gh-pages` and folder `/ (root)`
4. Save the settings

Your website will be available at: `https://d-osc.github.io/nova-lang`

## Custom Domain (Optional)

To use a custom domain:

1. Create a file `public/CNAME` with your domain:
   ```
   novalang.dev
   ```

2. Configure DNS records at your domain registrar:
   ```
   Type: CNAME
   Name: @
   Value: d-osc.github.io
   ```

3. Enable HTTPS in GitHub Pages settings

## Project Structure

```
website/
├── public/               # Static files
│   ├── index.html       # HTML template
│   └── manifest.json    # PWA manifest
├── src/
│   ├── components/      # React components
│   │   ├── Navbar.js    # Navigation bar
│   │   ├── Navbar.css
│   │   ├── Footer.js    # Footer component
│   │   └── Footer.css
│   ├── pages/           # Page components
│   │   ├── Home.js      # Landing page
│   │   ├── Home.css
│   │   ├── Features.js  # Features page
│   │   ├── Features.css
│   │   ├── Benchmarks.js # Benchmarks page
│   │   ├── Benchmarks.css
│   │   ├── Docs.js      # Documentation page
│   │   ├── Docs.css
│   │   ├── Download.js  # Download page
│   │   └── Download.css
│   ├── App.js           # Main app component
│   ├── App.css          # Global styles
│   ├── index.js         # Entry point
│   └── index.css        # Base styles
├── package.json         # Dependencies and scripts
└── README.md           # This file
```

## Customization

### Update Links

Replace `d-osc` with your actual GitHub username in:
- `package.json` (homepage field)
- `src/components/Navbar.js`
- `src/components/Footer.js`
- All page components (`src/pages/*.js`)

### Update Content

Edit the page components in `src/pages/` to update content:
- `Home.js` - Hero section, features, performance
- `Features.js` - Detailed features
- `Benchmarks.js` - Performance data
- `Docs.js` - Documentation links
- `Download.js` - Download links

### Styling

- Global styles: `src/index.css`, `src/App.css`
- Component styles: `src/components/*.css`
- Page styles: `src/pages/*.css`

## Color Scheme

- Primary gradient: `#667eea` → `#764ba2`
- Background: `#0a0a0a`
- Text: `#ffffff`
- Secondary text: `#aaa`
- Success: `#27c93f`

## Performance

- Lighthouse Score: 95+
- First Contentful Paint: < 1s
- Time to Interactive: < 2s

## Browser Support

- Chrome (latest)
- Firefox (latest)
- Safari (latest)
- Edge (latest)
- Mobile browsers

## Troubleshooting

### Deployment Issues

**Problem:** Website shows 404 after deployment

**Solution:**
- Check that `gh-pages` branch exists
- Verify GitHub Pages is enabled in repository settings
- Ensure `homepage` in `package.json` matches your GitHub Pages URL

**Problem:** Images or links not working

**Solution:**
- Make sure all links are relative or use `basename` in Router
- Check that `homepage` in `package.json` is correct

### Build Issues

**Problem:** `npm run build` fails

**Solution:**
```bash
# Clear cache and reinstall
rm -rf node_modules package-lock.json
npm install
npm run build
```

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test locally with `npm start`
5. Build with `npm run build`
6. Submit a pull request

## License

MIT License - Same as Nova project

## Support

For issues or questions:
- Open an issue on GitHub
- Check the [Nova documentation](https://github.com/d-osc/Nova)

---

**Built with React** ⚛️
**Deployed on GitHub Pages** 🚀
