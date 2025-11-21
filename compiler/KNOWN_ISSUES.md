# Nova Compiler - Known Issues

## 🐛 Active Bugs

### 1. Class Field Assignment Not Working
**Status:** 🔴 Critical  
**Test:** `test_class_simple.ts`  
**Expected:** 30  
**Actual:** 0  

**Problem:**  
Constructor field assignments (`this.field = value`) are not storing values correctly. The generated LLVM IR stores 0 instead of the actual field values.

**Root Cause:**  
In the constructor, field assignments generate:
```llvm
store i64 0, ptr %var, align 4    ; Should store the actual value
```

Instead of proper GEP (GetElementPtr) instructions to access struct fields.

**Impact:** Classes cannot store or retrieve instance data.

**Example:**
```typescript
class Person {
    age: number;
    constructor(age: number) {
        this.age = age;  // Not working - stores 0
    }
    getAge(): number {
        return this.age;  // Returns 0
    }
}
```

---

## ✅ Working Features (Verified)

All other major features are working correctly:
- ✅ Control flow (if/else, loops, switch/case)
- ✅ Break/continue statements
- ✅ All operators (arithmetic, logical, bitwise)
- ✅ Arrow functions
- ✅ Arrays and objects (literal syntax)
- ✅ Type annotations
- ✅ Template literals

---

## 📋 Implementation Priority

1. **High Priority:**  
   - Fix class field assignments (affects OOP paradigm)
   
2. **Medium Priority:**  
   - Fix string operations
   
3. **Low Priority:**  
   - Optimization improvements
   - Better error messages

---

**Last Updated:** 2025-11-21  
**Version:** v0.24.0
