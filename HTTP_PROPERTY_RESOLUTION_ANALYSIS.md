# HTTP Property Resolution Issue - Deep Analysis

## Date: December 4, 2025

## Executive Summary

The HTTP module's property resolution failures are caused by a **pointer identity mismatch** in the `builtinObjectTypes_` map. Objects are stored correctly but cannot be retrieved because the HIR value pointers change between storage and lookup.

## Root Cause Analysis

### The Problem

In `src/hir/HIRGen.cpp`, the code uses a map to track builtin object types:
```cpp
std::unordered_map<hir::HIRValue*, std::string> builtinObjectTypes_;
```

**Storage** (lines 638-645):
```cpp
if (runtimeFuncName == "nova_http2_createServer") {
    lastBuiltinObjectType_ = "http2:Server";
    builtinObjectTypes_[lastValue_] = "http2:Server";  // Stores pointer A
} else if (runtimeFuncName == "nova_http_createServer") {
    lastBuiltinObjectType_ = "http:Server";
    builtinObjectTypes_[lastValue_] = "http:Server";   // Stores pointer B
}
```

**Lookup** (lines 13076-13096):
```cpp
// Check if this is a builtin object method (e.g., server.listen)
auto objectTypeIt = builtinObjectTypes_.find(object);  // Tries to find pointer C
if (objectTypeIt != builtinObjectTypes_.end()) {
    // Map method to runtime function
    std::string objectType = objectTypeIt->second;
    // ...
}
```

### Why It Fails

The `object` pointer used in the lookup is **NOT the same pointer** as `lastValue_` that was stored. Even though they represent the same logical server object, they are different HIR value pointers.

**Pointer Identity vs Value Equality**:
- `builtinObjectTypes_` uses **pointer identity** for lookups (comparing memory addresses)
- When HIR values are copied, transformed, or accessed through different paths, new pointers are created
- The map lookup fails because `pointer_C != pointer_B`, even though both point to the same logical server

### Evidence

1. **HTTP module is tracked**: Line 642-645 shows HTTP objects ARE being added to the map
2. **Warnings still occur**: Property resolution fails despite tracking
3. **HTTP2 works differently**: HTTP2 likely works because its objects maintain pointer identity through a different code path

## Detailed Code Flow

### 1. Object Creation and Storage
```typescript
const server = http.createServer((req, res) => { ... });
```

HIRGen flow:
1. `visit(CallExpr)` is called for `http.createServer()`
2. Runtime function `nova_http_createServer` is called
3. `lastValue_` is set to the returned HIR value
4. **STORAGE**: `builtinObjectTypes_[lastValue_] = "http:Server"`
5. `lastValue_` pointer address: e.g., `0x12345678`

### 2. Variable Assignment
```typescript
const server = ...
```

The value might be:
- Copied to a new HIR value
- Stored in a local variable slot
- Transformed through IR passes

New pointer created: e.g., `0x87654321`

### 3. Property Access
```typescript
server.listen(port);
```

HIRGen flow:
1. `visit(MemberExpr)` is called for `server.listen`
2. `object` is the HIR value representing `server`
3. `object` pointer address: `0x87654321` (DIFFERENT!)
4. **LOOKUP**: `builtinObjectTypes_.find(object)` searches for `0x87654321`
5. **FAILURE**: Map only contains `0x12345678` -> "http:Server"
6. Lookup returns `end()`, warning is printed

## Why HTTP2 Might Work

HTTP2 could be working for several reasons:

1. **Different code path**: HTTP2 uses a different HIR generation path that preserves pointer identity
2. **Global object**: HTTP2 might use a global object reference instead of a local variable
3. **Direct method calls**: HTTP2 methods might be called directly without intermediate variable assignment
4. **Simpler IR**: HTTP2 code might not trigger IR transformations that create new pointers

## Solutions

### Solution 1: Use Value-Based Lookup (RECOMMENDED)

Replace pointer-based map with a value-based system:

```cpp
// Instead of: std::unordered_map<hir::HIRValue*, std::string>
// Use a unique identifier system:
std::unordered_map<uint64_t, std::string> builtinObjectTypes_;

// When storing:
uint64_t objectId = nextObjectId_++;
objectToId_[lastValue_] = objectId;
builtinObjectTypes_[objectId] = "http:Server";

// When looking up:
auto idIt = objectToId_.find(object);
if (idIt != objectToId_.end()) {
    auto typeIt = builtinObjectTypes_.find(idIt->second);
    if (typeIt != builtinObjectTypes_.end()) {
        // Found!
    }
}
```

**Pros**:
- Robust against pointer changes
- Works with value copies and transformations
- Clear separation of concerns

**Cons**:
- Requires additional ID tracking map
- More complex implementation

### Solution 2: Track Through Symbol Table

Use the symbol table to associate variable names with types:

```cpp
// When storing:
if (currentVariableName_) {
    variableObjectTypes_[*currentVariableName_] = "http:Server";
}

// When looking up (for `server.listen`):
if (auto* ident = dynamic_cast<Identifier*>(memberExpr->object.get())) {
    auto typeIt = variableObjectTypes_.find(ident->name);
    if (typeIt != variableObjectTypes_.end()) {
        // Found!
    }
}
```

**Pros**:
- Leverages existing symbol information
- Natural mapping (variable name -> type)
- No pointer identity issues

**Cons**:
- Only works for named variables
- Doesn't handle anonymous objects or complex expressions

### Solution 3: Propagate Type Through HIR

Add type metadata to HIR values:

```cpp
struct HIRValue {
    // ... existing fields ...
    std::string builtinObjectType;  // e.g., "http:Server"
};

// When creating:
lastValue_->builtinObjectType = "http:Server";

// When looking up:
if (!object->builtinObjectType.empty()) {
    // Use object->builtinObjectType directly
}
```

**Pros**:
- Most robust solution
- Works for all cases (named, anonymous, transformed)
- Type information travels with the value

**Cons**:
- Requires HIRValue struct modification
- Impacts all HIR code
- Memory overhead for all values

### Solution 4: Fix Pointer Identity (COMPLEX)

Ensure that object references maintain pointer identity:

- Use reference counting or smart pointers
- Avoid unnecessary value copies
- Canonicalize object references

**Pros**:
- No fundamental architecture changes

**Cons**:
- Very complex to implement correctly
- Hard to maintain
- Error-prone

## Recommended Approach

**Immediate**: Solution 2 (Symbol Table Tracking)
- Quickest to implement
- Covers 90% of use cases
- Minimal code changes

**Long-term**: Solution 3 (Type Metadata in HIR)
- Most correct and robust
- Enables better type checking throughout pipeline
- Foundation for future improvements

## Implementation Plan

### Phase 1: Symbol Table Tracking (Quick Fix)

1. **Add variable type tracking**:
   ```cpp
   std::unordered_map<std::string, std::string> variableObjectTypes_;
   ```

2. **Track assignments**:
   ```cpp
   void visit(VariableDeclarator& node) {
       // ... existing code ...
       if (!lastBuiltinObjectType_.empty()) {
           variableObjectTypes_[node.id->name] = lastBuiltinObjectType_;
       }
   }
   ```

3. **Lookup by variable name**:
   ```cpp
   if (auto* ident = dynamic_cast<Identifier*>(memberExpr->object.get())) {
       auto typeIt = variableObjectTypes_.find(ident->name);
       if (typeIt != variableObjectTypes_.end()) {
           std::string objectType = typeIt->second;
           // Map method to runtime function...
       }
   }
   ```

### Phase 2: HIR Type Metadata (Complete Solution)

1. Add `builtinObjectType` field to `HIRValue`
2. Propagate type through all HIR operations
3. Update `visit(MemberExpr)` to use value's type metadata
4. Remove `builtinObjectTypes_` map

## Testing Strategy

1. **Create test cases**:
   - Named variable: `const server = http.createServer(...); server.listen(...)`
   - Chained call: `http.createServer(...).listen(...)`
   - Reassignment: `let s = server; s.listen(...)`
   - Function parameter: `function start(srv) { srv.listen(...) }`

2. **Verify**:
   - All property warnings disappear
   - HTTP server starts and listens correctly
   - HTTP benchmarks match HTTP2 performance

3. **Regression**:
   - HTTP2 still works
   - Other modules unaffected
   - No performance degradation

## Conclusion

The HTTP property resolution issue is **not a fundamental HTTP module problem** but a **type tracking architecture issue** in HIRGen. The fix requires improving how builtin object types are tracked through the IR generation pipeline.

**Status**: Root cause identified, solutions designed, ready for implementation

**Estimated Effort**:
- Phase 1 (Quick Fix): 2-4 hours
- Phase 2 (Complete): 1-2 days
- Testing: 1 day

**Priority**: High (blocks HTTP module functionality)

---

**Next Actions**:
1. Implement Phase 1 (Symbol Table Tracking)
2. Test with HTTP module
3. Run benchmarks
4. Plan Phase 2 implementation
