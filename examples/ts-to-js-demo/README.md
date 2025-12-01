# TypeScript to JavaScript Demo

This example project demonstrates Nova's TypeScript to JavaScript transpiler with full tsconfig.json support.

## Project Structure

```
ts-to-js-demo/
├── tsconfig.json           # TypeScript configuration (extends base)
├── tsconfig.base.json      # Base configuration
├── tsconfig.es6.json       # ES6 modules configuration
├── build.bat               # Windows build script
├── build.sh                # Linux/Mac build script
├── src/
│   ├── index.ts           # Main entry point
│   ├── models/
│   │   ├── User.ts        # User class with interfaces
│   │   └── Product.ts     # Product class with generics & enums
│   ├── services/
│   │   └── CartService.ts # Shopping cart service
│   └── utils/
│       └── helpers.ts     # Utility functions
├── dist/                  # CommonJS output (generated)
└── dist-es/               # ES modules output (generated)
```

## tsconfig.json Features Supported

Nova's transpiler supports nearly all tsconfig.json options:

| Category | Options |
|----------|---------|
| **Output** | outDir, rootDir, declarationDir, outFile |
| **Module** | module (commonjs, es6, esnext), moduleResolution, paths, baseUrl |
| **Target** | target (es3-es2022, esnext) |
| **JSX** | jsx, jsxFactory, jsxFragmentFactory, jsxImportSource |
| **Declaration** | declaration, declarationMap, emitDeclarationOnly |
| **Source Maps** | sourceMap, inlineSourceMap, inlineSources, sourceRoot |
| **Emit** | removeComments, noEmit, noEmitOnError, newLine |
| **JavaScript** | allowJs, checkJs |
| **Build** | incremental, extends, composite |

## TypeScript Features Demonstrated

- Classes with public/private modifiers
- Interfaces and type aliases
- Generics (`<T>`)
- Enums (string and numeric)
- Optional properties (`?`)
- Optional parameters (`param?: Type`)
- Arrow functions
- Import/Export (ES6 modules → CommonJS)
- Type annotations and return types
- Async/Promise types

## Build Commands

### Using build scripts
```bash
# Windows
build.bat

# Linux/Mac
./build.sh

# With minification
build.bat --minify
./build.sh --minify
```

### Manual build
```bash
# Basic build (removes type annotations)
nova -b src/index.ts --outDir ./dist

# Build with minification
nova -b src/index.ts --outDir ./dist --minify

# Build with all options
nova -b src/index.ts --outDir ./dist --minify --declaration --sourceMap
```

## Size Comparison

| File | Original | Normal | Minified |
|------|----------|--------|----------|
| User.ts | 885 B | 523 B (41%) | 340 B (62%) |
| Product.ts | 1203 B | 811 B (33%) | 567 B (53%) |
| helpers.ts | 1499 B | 1157 B (23%) | 911 B (40%) |
| CartService.ts | 2395 B | 1978 B (18%) | 1292 B (47%) |
| index.ts | 1901 B | 1594 B (17%) | 1312 B (31%) |
| **Total** | **7883 B** | **6063 B (23%)** | **4422 B (44%)** |

## Run Output

```bash
# After building
node dist/index.js
```

Expected output:
```
User: John Doe (john@example.com)
Is Admin: false
Product: Laptop - $999.99
After 20% discount: 799.99
Cart updated Total: 999.99
Cart updated Total: 1029.98
Cart updated Total: 1079.97
Cart updated Total: 2079.96
=== Cart Summary ===
Customer: John Doe (john@example.com)
Items: 4
Total: $2079.96
Unique numbers: [ 1, 2, 3, 4 ]
First: 1
Last: 4
Capitalized: Typescript
Clamped (0-100): 100
Total items in cart: 4
```

## Performance

- Build time: ~50ms total (5 files)
- Average: ~10ms per file
- 10-100x faster than tsc
