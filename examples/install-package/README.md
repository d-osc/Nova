# Nova Install Demo

This example demonstrates the `nova install` command for fast package installation with caching.

## Quick Start

```bash
# Install dependencies (with caching)
nova install
# or
nova i

# Build TypeScript to JavaScript
nova -b

# Run the demo
node dist/index.js
```

## Features Demonstrated

### 1. package-lock.json Support
Nova uses npm's `package-lock.json` for exact versions and resolved URLs:
```bash
# Generate lockfile (one-time, using npm)
npm install --package-lock-only

# Install with nova (uses lockfile)
nova i
# [nova] Using package-lock.json...
# [nova] Found 9 packages in lockfile
```

### 2. Fast Package Installation
```bash
# First run - downloads packages
nova i
# [nova] 9 packages downloaded (xxx KB)

# Second run - uses cache (instant)
nova i
# [nova] 9 packages from cache (instant)
```

### 3. Central Cache
Packages are cached at:
- Windows: `%LOCALAPPDATA%\nova\cache\`
- Linux/Mac: `~/.nova/cache/`

Cache structure:
```
nova/cache/
├── lodash/
│   └── latest/
│       └── 4.17.21.tar.gz
├── chalk/
│   └── latest/
│       └── 4.1.2.tar.gz
└── uuid/
    └── latest/
        └── 9.0.0.tar.gz
```

### 4. Dependencies Used

| Package | Description |
|---------|-------------|
| lodash | Utility library (array, object, string functions) |
| chalk | Terminal string styling |
| uuid | Generate RFC-compliant UUIDs |

## Project Structure

```
install-package/
├── package.json       # Dependencies
├── package-lock.json  # Lockfile (exact versions)
├── tsconfig.json      # TypeScript config
├── src/
│   └── index.ts       # Demo source code
├── dist/              # Compiled output (after build)
└── node_modules/      # Installed packages (after install)
```

## Commands

```bash
# Install all dependencies
nova install

# Install specific package
nova i lodash

# Build project
nova -b

# Build with watch mode
nova -b --watch
```
