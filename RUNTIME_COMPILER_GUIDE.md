# Nova Runtime & Compiler Guide

Nova ถูกแยกออกเป็น 2 executables แล้ว:

## 🚀 Nova Runtime (`nova`)

Runtime สำหรับรันโปรแกรม TypeScript/JavaScript แบบ JIT

### การใช้งาน:

```bash
# รันโปรแกรม
nova script.ts
nova app.js

# Interactive shell (REPL)
nova

# ดู JIT cache stats
nova --cache-stats

# ล้าง cache
nova --clear-cache

# Verbose mode
nova --verbose script.ts
```

### คุณสมบัติ:
- ✅ JIT compilation with caching
- ✅ Fast execution (cached native binaries)
- ✅ Interactive shell (REPL)
- ✅ Automatic optimization
- ✅ Full JavaScript/TypeScript support

---

## 🔧 Nova Compiler (`novac`)

Compiler สำหรับ compile โปรแกรมเป็น native executable หรือ transpile TypeScript

### การใช้งาน:

```bash
# Compile to native executable
novac -c app.ts -o app.exe

# Transpile TypeScript to JavaScript (like tsc)
novac -b src/index.ts --outDir dist
novac -b --watch              # Watch mode
novac -b --minify             # Minified output

# Emit IR stages
novac emit --llvm app.ts      # LLVM IR
novac emit --mir app.ts       # MIR
novac emit --hir app.ts       # HIR
novac emit --asm app.ts       # Assembly

# Type checking
novac check app.ts

# Initialize new project
novac init ts

# Package manager
novac pm install lodash
novac pm list
```

### Optimization levels:
```bash
novac -c app.ts -O0    # No optimization
novac -c app.ts -O1    # Basic optimization
novac -c app.ts -O2    # Default - balanced
novac -c app.ts -O3    # Aggressive optimization
```

---

## 📊 เปรียบเทียบ

| Feature | `nova` (Runtime) | `novac` (Compiler) | `nnpm` (Package Manager) |
|---------|------------------|-------------------|------------------------|
| JIT Execution | ✅ | ❌ | ❌ |
| AOT Compilation | ❌ | ✅ | ❌ |
| Interactive Shell | ✅ | ❌ | ❌ |
| Transpile TS→JS | ❌ | ✅ | ❌ |
| Emit IR | ❌ | ✅ | ❌ |
| Package Manager | ❌ | ❌ | ✅ |
| Native Caching | ✅ | ❌ | ❌ |
| Project Init | ❌ | ❌ | ✅ |
| Run Scripts | ❌ | ❌ | ✅ |

---

## 📦 Nova Package Manager (`nnpm`)

Package manager สำหรับจัดการ dependencies และโปรเจค

### การใช้งาน:

```bash
# สร้างโปรเจคใหม่
nnpm init
nnpm init ts              # With TypeScript

# ติดตั้ง dependencies
nnpm install              # จาก package.json
nnpm install lodash       # ติดตั้ง package
nnpm i express            # แบบย่อ

# ติดตั้ง dev dependencies
nnpm install --save-dev typescript
nnpm i -D @types/node

# อัพเดท packages
nnpm update
nnpm u lodash

# ลบ package
nnpm uninstall lodash
nnpm un express

# รัน scripts
nnpm run dev
nnpm run build
nnpm test
```

---

## 🎯 แนะนำการใช้งาน

### สำหรับ Development:
```bash
# ใช้ nova รัน - เร็วกว่า เพราะมี JIT cache
nova dev.ts
nova test.ts
```

### สำหรับ Production:
```bash
# Compile เป็น native executable
novac -c app.ts -O3 -o app.exe

# หรือ transpile แล้วใช้ Node.js
novac -b src --outDir dist --minify
```

### สำหรับ Build Tools:
```bash
# ใช้ novac สำหรับ transpilation
novac -b --watch --sourceMap
```

---

## 🔄 Workflow Examples

### Example 1: Quick Development
```bash
# เขียนโค้ด
vim app.ts

# รันทันที (JIT)
nova app.ts

# ทดสอบซ้ำ (ใช้ cache - เร็วมาก!)
nova app.ts
```

### Example 2: Production Build
```bash
# Compile เป็น native
novac -c app.ts -O3 -o build/app.exe

# Deploy
./build/app.exe
```

### Example 3: TypeScript Project
```bash
# Initialize
nnpm init ts

# Install dependencies
nnpm install express
nnpm i -D @types/node

# Develop with watch mode
novac -b --watch

# Build for production
novac -b --minify --declaration
```

---

## 💡 Tips

1. **Development**: ใช้ `nova` เพราะ JIT cache ทำให้รันซ้ำเร็วมาก
2. **Production**: ใช้ `novac -c` เพื่อ compile เป็น native executable
3. **CI/CD**: ใช้ `novac -b` เพื่อ transpile และ type check
4. **Testing**: ใช้ `nova` เพื่อรัน test scripts อย่างรวดเร็ว

---

## 📦 Installation

All three executables are built together:

```bash
cmake --build build --config Release
```

Output:
- `build/Release/nova.exe` - Runtime
- `build/Release/novac.exe` - Compiler
- `build/Release/nnpm.exe` - Package Manager

---

## 🌟 New Features

### Nova Runtime:
- Native binary caching for instant re-execution
- Interactive shell with .help, .clear, .exit commands
- Lightweight - only runtime and JIT components

### Nova Compiler:
- Complete build toolchain
- TypeScript transpilation (like tsc)
- IR emission at all stages
- Type checking

### Nova Package Manager:
- nnpm-compatible package manager
- Project initialization (init, init ts)
- Dependency management (install, update, uninstall)
- Script execution (run, test)
- Global package support

---

For more information: https://nova-lang.org/docs
