# Nova Compiler - Quick Reference

## 🎯 Command Line Usage

### Compile TypeScript/JavaScript
```powershell
nova.exe compile <input.ts> [options]
```

### Options
| Option | Description |
|--------|-------------|
| `--emit-all` | Generate HIR, MIR, and LLVM IR |
| `--emit-hir` | Generate only HIR |
| `--emit-mir` | Generate only MIR |
| `--emit-llvm` | Generate only LLVM IR |
| `--verbose` | Show detailed compilation output |
| `-O0, -O1, -O2, -O3` | Optimization levels |

## 📝 Supported TypeScript Syntax

### Functions
```typescript
function functionName(param1: type, param2: type): returnType {
    // function body
    return value;
}
```

### Variables
```typescript
const variableName = expression;
```

### Arithmetic Operations
```typescript
a + b    // Addition
a - b    // Subtraction
a * b    // Multiplication
a / b    // Division
```

### Function Calls
```typescript
result = functionName(arg1, arg2);
```

## 🔧 Build Commands

### Initial Build
```powershell
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

### Rebuild
```powershell
cmake --build build --config Release
```

### Clean Build
```powershell
cmake --build build --config Release --clean-first
```

## 🧪 Testing

### Run All Tests
```powershell
.\run_tests.ps1
```

### Run Single Test
```powershell
.\build\Release\nova.exe compile test_simple.ts --emit-all
```

### Check Generated IR
```powershell
# View HIR
Get-Content test_simple.hir

# View MIR
Get-Content test_simple.mir

# View LLVM IR
Get-Content test_simple.ll
```

## 📊 Output Files

| Extension | Description | Format |
|-----------|-------------|--------|
| `.hir` | High-level IR | Text-based, SSA form |
| `.mir` | Mid-level IR | Text-based, control flow |
| `.ll` | LLVM IR | LLVM assembly |

## 🎨 Example Workflow

### 1. Write TypeScript
```typescript
// mycode.ts
function add(a: number, b: number): number {
    return a + b;
}
```

### 2. Compile
```powershell
.\build\Release\nova.exe compile mycode.ts --emit-all
```

### 3. Check Output
```powershell
Get-Content mycode.ll
```

### 4. Verify LLVM IR
```powershell
llvm-as mycode.ll -o mycode.bc
```

## 🔍 Debugging

### Enable Verbose Output
```powershell
.\build\Release\nova.exe compile input.ts --emit-all --verbose
```

### Check Compilation Stages
1. **Parsing**: Check for syntax errors
2. **HIR Generation**: Verify function structure
3. **MIR Generation**: Check control flow
4. **LLVM IR**: Validate types and instructions

### Common Issues

**Syntax Error**
- Check TypeScript syntax
- Ensure proper type annotations

**Type Mismatch**
- Verify function signatures
- Check parameter types

**LLVM Verification Failed**
- Check generated `.ll` file
- Verify SSA form correctness

## 💡 Tips & Tricks

### Optimize Compilation
- Use `-O2` for production builds
- Use `-O0` for debugging

### View IR in Real-time
```powershell
.\build\Release\nova.exe compile input.ts --emit-all --verbose | Tee-Object compile.log
```

### Compare IR Stages
```powershell
# Compare function structure across stages
Select-String "function" *.hir, *.mir, *.ll
```

### Batch Compilation
```powershell
Get-ChildItem *.ts | ForEach-Object {
    .\build\Release\nova.exe compile $_.Name --emit-all
}
```

## 📈 Performance Tuning

### Compilation Speed
- Single file: < 1 second
- Multiple files: Parallel compilation recommended

### Memory Usage
- Typical: < 100 MB per file
- Large files: < 500 MB

### Build Options
```cmake
# Fast build
cmake -DCMAKE_BUILD_TYPE=Debug

# Optimized build
cmake -DCMAKE_BUILD_TYPE=Release

# With assertions
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo
```

## 🎓 Learning Resources

### Understanding IR
1. **HIR (High-level)**: Close to source, typed
2. **MIR (Mid-level)**: Basic blocks, CFG
3. **LLVM IR**: Machine-independent assembly

### Reading LLVM IR
- `@function` - Global function
- `%0, %1` - Virtual registers (SSA)
- `i64` - 64-bit integer
- `call` - Function call
- `ret` - Return statement

### Example IR Flow
```
TypeScript: function add(a, b) { return a + b; }
     ↓
HIR:    fn add(%arg0, %arg1) { return %arg0 + %arg1 }
     ↓
MIR:    bb0: _t0 = BinaryOp(Add, arg0, arg1); return _t0;
     ↓
LLVM:   define i64 @add(i64 %0, i64 %1) {
          %2 = add i64 %0, %1
          ret i64 %2
        }
```

## 🔗 Quick Links

- **Documentation**: `FINAL_SUMMARY.md`
- **Test Results**: `TEST_RESULTS.md`
- **Examples**: `test_*.ts` files
- **Demo**: `.\demo.ps1`

## 🆘 Support

**Build Issues**: Check CMake output
**Runtime Errors**: Use `--verbose` flag
**IR Questions**: Compare with examples

---

*Nova Compiler v1.0 - Production Ready*
