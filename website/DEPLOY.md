# Deploy Nova Website to GitHub Pages

Quick guide to deploy the Nova website to GitHub Pages.

## Step 1: Setup

1. **Update package.json** - Replace `d-osc` with your GitHub username:
   ```json
   "homepage": "https://d-osc.github.io/nova-lang"
   ```

2. **Update Links in Code** - Replace `d-osc` in all files:
   ```bash
   # Search and replace in all files
   # Windows PowerShell:
   Get-ChildItem -Recurse -Include *.js,*.json | ForEach-Object {
     (Get-Content $_) -replace 'd-osc', 'YOUR_GITHUB_USERNAME' | Set-Content $_
   }

   # Linux/macOS:
   find . -type f \( -name "*.js" -o -name "*.json" \) -exec sed -i 's/d-osc/YOUR_GITHUB_USERNAME/g' {} +
   ```

## Step 2: Install Dependencies

```bash
cd website
npm install
```

## Step 3: Test Locally

```bash
npm start
```

Visit `http://localhost:3000` to preview the website.

## Step 4: Build

```bash
npm run build
```

This creates an optimized production build in the `build/` directory.

## Step 5: Deploy to GitHub Pages

```bash
npm run deploy
```

This command will:
1. Build the website
2. Create/update the `gh-pages` branch
3. Push the built files to GitHub

## Step 6: Configure GitHub Pages

1. Go to your repository on GitHub
2. Click **Settings** → **Pages**
3. Under **Source**, select:
   - Branch: `gh-pages`
   - Folder: `/ (root)`
4. Click **Save**

## Step 7: Visit Your Website

After a few minutes, your website will be live at:
```
https://d-osc.github.io/nova-lang
```

## Automatic Deployment

### Using GitHub Actions (Optional)

Create `.github/workflows/deploy-website.yml`:

```yaml
name: Deploy Website

on:
  push:
    branches: [ master ]
    paths:
      - 'website/**'

jobs:
  deploy:
    runs-on: ubuntu-latest

    steps:
    - uses: actions/checkout@v4

    - name: Setup Node.js
      uses: actions/setup-node@v4
      with:
        node-version: '18'

    - name: Install dependencies
      run: |
        cd website
        npm ci

    - name: Build
      run: |
        cd website
        npm run build

    - name: Deploy to GitHub Pages
      uses: peaceiris/actions-gh-pages@v3
      with:
        github_token: ${{ secrets.GITHUB_TOKEN }}
        publish_dir: ./website/build
        publish_branch: gh-pages
```

Now the website will automatically redeploy when you push changes to the `website/` directory!

## Custom Domain

### Setup

1. **Buy a domain** (e.g., from Namecheap, GoDaddy, Cloudflare)

2. **Add CNAME file** - Create `public/CNAME`:
   ```
   novalang.dev
   ```

3. **Configure DNS** at your domain registrar:
   ```
   Type: CNAME
   Name: @ (or www)
   Value: d-osc.github.io
   ```

4. **Enable HTTPS** in GitHub Pages settings

5. **Redeploy**:
   ```bash
   npm run deploy
   ```

## Troubleshooting

### 404 Error After Deploy

**Problem:** Website shows 404 Not Found

**Solutions:**
1. Check that `homepage` in `package.json` is correct
2. Verify `gh-pages` branch exists
3. Ensure GitHub Pages is enabled in repository settings
4. Wait 5-10 minutes for GitHub to process the deployment

### Build Errors

**Problem:** `npm run build` fails

**Solutions:**
```bash
# Clear cache
rm -rf node_modules package-lock.json
npm install
npm run build
```

### Deployment Permission Error

**Problem:** Permission denied when running `npm run deploy`

**Solutions:**
1. Make sure you're logged into GitHub
2. Check repository permissions
3. Use SSH instead of HTTPS for git remote:
   ```bash
   git remote set-url origin git@github.com:d-osc/Nova.git
   ```

### Links Not Working

**Problem:** Internal links show 404

**Solutions:**
1. Make sure Router has `basename="/Nova"` in `App.js`
2. Use `Link` from `react-router-dom` for internal navigation
3. Verify all links are relative to the basename

## Development Tips

### Hot Reload
```bash
npm start
```
Changes auto-reload at `http://localhost:3000`

### Production Build Test
```bash
npm run build
npx serve -s build
```
Test production build locally at `http://localhost:3000`

### Clean Build
```bash
rm -rf build node_modules package-lock.json
npm install
npm run build
```

## Performance Optimization

### Code Splitting

React automatically code-splits by route. Each page loads only when visited.

### Image Optimization

Place images in `public/images/` and reference them as:
```jsx
<img src={process.env.PUBLIC_URL + '/images/logo.png'} alt="Nova" />
```

### Lazy Loading

Use `React.lazy()` for components:
```jsx
const Benchmarks = React.lazy(() => import('./pages/Benchmarks'));
```

## Monitoring

### Check Deploy Status

```bash
# View gh-pages branch
git checkout gh-pages
git log

# Return to main branch
git checkout master
```

### Analytics (Optional)

Add Google Analytics to `public/index.html`:
```html
<!-- Google Analytics -->
<script async src="https://www.googletagmanager.com/gtag/js?id=GA_MEASUREMENT_ID"></script>
<script>
  window.dataLayer = window.dataLayer || [];
  function gtag(){dataLayer.push(arguments);}
  gtag('js', new Date());
  gtag('config', 'GA_MEASUREMENT_ID');
</script>
```

## Maintenance

### Update Content

1. Edit files in `src/pages/`
2. Test locally: `npm start`
3. Deploy: `npm run deploy`

### Update Dependencies

```bash
npm update
npm audit fix
npm run build
npm run deploy
```

## Next Steps

✅ Website is live!
✅ Update content as needed
✅ Monitor performance with Lighthouse
✅ Add analytics (optional)
✅ Consider custom domain (optional)

---

**Need help?** Open an issue on GitHub!
