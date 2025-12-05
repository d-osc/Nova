# Nova Compiler - TODO List

> **Last Updated**: 2025-12-06
> **Current Version**: v1.4.0
> **Test Status**: 511 tests passing (100%)

---

## Recent Updates (December 2025)

### ✅ Completed
- **Code Cleanup**: Removed all DEBUG code from compiler source files
  - Disabled NOVA_DEBUG in 6 core files (main.cpp, LLVMCodeGen.cpp, HIRGen.cpp, MIRGen.cpp, HIRBuilder.cpp, Iterator.cpp)
  - Cleaned up 1400+ debug statements for production readiness
- **Package Manager**: Added `nova pm` subcommand for better UX
  - Unified package manager interface (`nova pm install`, `nova pm update`, etc.)
  - Full npm-compatible package management
- **Website Documentation**: Comprehensive documentation added
  - CLI commands reference with detailed examples
  - Configuration guide (nova.config.json)
  - Performance optimization tips
  - Debugging guide and testing framework docs
  - Deployment guides (standalone binary, Docker, systemd)
- **Multi-language Support**: Website now supports Thai (ไทย) and English
- **Installation Scripts**: Added Bun-style one-liner installers for macOS/Linux/Windows

---

## Current Status

The Nova compiler is feature-complete for most TypeScript/JavaScript functionality. All 511 tests pass with 100% success rate.

### Implemented Features

- Variables: let, const, var
- Functions: regular, arrow, async, generators
- Classes: properties, methods, constructors, inheritance, static, getters/setters
- Control flow: if/else, switch, ternary, loops (for, while, do-while, for-of, for-in)
- Error handling: try/catch/finally, throw
- Operators: all arithmetic, logical, bitwise, comparison, assignment
- Arrays: full method support (40+ methods)
- Strings: full method support (25+ methods)
- Objects: keys, values, entries, assign, freeze, seal
- Math: all methods and constants
- Number: all methods and constants
- Console: log, error, warn, info, debug, etc.
- TypedArrays: Int8Array, Uint8Array, etc. with full method support
- Async/Await: async functions, await expressions
- Generators: function*, yield, yield*
- Async Generators: async function*, for-await-of
- Destructuring: array and object
- Spread operator: arrays and objects
- Rest parameters
- Default parameters
- Template literals
- Optional chaining: ?.
- Nullish coalescing: ??
- typeof, instanceof, in, delete operators
- Enums
- using/DisposableStack

---

## Future Improvements

### Performance
- [x] Remove debug overhead (DEBUG code removed)
- [ ] Implement lazy compilation
- [ ] Add caching for repeated compilations
- [ ] Optimize runtime library further

### Language Features
- [ ] Full closure support (capturing variables)
- [ ] Full Promise chain support
- [ ] WeakMap/WeakSet
- [ ] Symbol
- [ ] Proxy/Reflect
- [ ] Full module system (import/export)

### Tooling
- [ ] Source maps
- [ ] Debugger support
- [ ] Language server protocol (LSP)
- [ ] Watch mode for development

### Documentation
- [x] Comprehensive user documentation (website)
- [x] CLI commands reference
- [x] Configuration guide
- [x] Performance and debugging guides
- [x] Deployment documentation
- [ ] API reference (detailed)
- [ ] Internal architecture guide
- [ ] Contributing guide

---

## Known Limitations

1. **Closures**: Arrow functions used as callbacks work, but full closure capturing of outer variables is limited
2. **Promises**: Basic Promise support works, but complex chains may not
3. **Modules**: import/export not fully supported
4. **Type checking**: No static type checking (compiles like JavaScript)

---

## Version History

See [CHANGELOG.md](CHANGELOG.md) for detailed version history.
