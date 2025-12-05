# GitHub Actions CI/CD

## Overview

This workflow automatically builds the Nova compiler for multiple platforms:

- **macOS** (Intel x64 + Apple Silicon arm64)
- **Linux** (x64)
- **Windows** (x64)

## Triggers

The build workflow runs automatically when:

1. **Push to branches**: `master`, `main`, or `develop`
2. **Pull requests** to these branches
3. **Manual trigger** via GitHub Actions UI
4. **Tags** starting with `v` (creates a release)

## How to Use

### 1. Push Code to GitHub

```bash
git add .github/workflows/build.yml
git commit -m "Add GitHub Actions CI/CD for multi-platform builds"
git push origin master
```

### 2. View Build Progress

1. Go to your repository on GitHub
2. Click the **"Actions"** tab
3. You'll see the workflow running with 3 parallel jobs

### 3. Download Built Binaries

After the build completes:

1. Go to the completed workflow run
2. Scroll down to **"Artifacts"** section
3. Download:
   - `nova-macos-x64` (macOS Intel)
   - `nova-macos-arm64` (macOS Apple Silicon)
   - `nova-linux-x64` (Linux)
   - `nova-windows-x64` (Windows)

### 4. Create a Release (Optional)

To create a GitHub Release with all binaries:

```bash
# Tag your commit
git tag v1.0.0
git push origin v1.0.0
```

This will:
- Build all platforms
- Create a GitHub Release
- Attach all binaries to the release

## Build Matrix

| Platform | OS | Architecture | Binary Name |
|----------|----|--------------| ------------|
| macOS    | macos-latest | x86_64 | nova-macos-x64 |
| macOS    | macos-latest | arm64 | nova-macos-arm64 |
| Linux    | ubuntu-latest | x86_64 | nova-linux-x64 |
| Windows  | windows-latest | x86_64 | nova-windows-x64.exe |

## Dependencies Installed

### macOS
- LLVM (via Homebrew)
- CMake
- Ninja

### Linux
- Clang 18
- LLVM 18
- CMake
- Ninja
- libc++

### Windows
- LLVM 18.1.7 (via Chocolatey)
- Ninja (via Chocolatey)
- MSVC Build Tools

## Customization

### Change LLVM Version

Edit the workflow file:

**Linux:**
```yaml
sudo apt-get install -y clang-19 llvm-19 llvm-19-dev
```

**Windows:**
```yaml
choco install llvm --version=19.0.0 -y
```

### Add More Platforms

Add a new job in `.github/workflows/build.yml`:

```yaml
build-linux-arm64:
  runs-on: ubuntu-latest
  steps:
    - name: Build for ARM64
      run: |
        # Cross-compilation setup
```

## Troubleshooting

### Build Fails on macOS

- Check LLVM installation path
- Verify CMake can find LLVM: `brew --prefix llvm`

### Build Fails on Windows

- Ensure LLVM path is correct
- Check if `_ALLOW_COMPILER_AND_STL_VERSION_MISMATCH` flag is set

### Artifacts Not Uploading

- Verify retention-days is within GitHub limits
- Check that binary paths are correct

## Local Testing

To test the workflow locally before pushing:

### Using Act (GitHub Actions locally)

```bash
# Install act
brew install act  # macOS
# or
choco install act-cli  # Windows

# Run the workflow
act push
```

## Performance

Typical build times on GitHub Actions:
- macOS (x64): ~15-20 minutes
- macOS (arm64): ~15-20 minutes
- Linux: ~10-15 minutes
- Windows: ~15-20 minutes

**Total parallel build time**: ~20-25 minutes

## Cost

GitHub Actions is **FREE** for public repositories with:
- Unlimited minutes for public repos
- 2,000 minutes/month for private repos

## Support

For issues with the CI/CD workflow, check:
1. GitHub Actions logs in the "Actions" tab
2. Individual job logs for detailed error messages
3. Build output in the workflow run details
