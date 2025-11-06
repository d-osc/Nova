# Nova Compiler - Next Development Steps

## Immediate Priorities (Next Development Cycle)

### 1. Fix Loop Body Operations

**Problem:** Loop control flow structures are generated correctly, but operations inside loop bodies are not executing.

**Debugging Steps:**
1. **Examine HIR Generation**
   - Add debug output to trace HIR generation for loop bodies
   - Verify that variable assignments and operations are being added to the correct blocks

2. **Check MIR Generation**
   - Add debug output to trace MIR generation for loop bodies
   - Verify that MIR operations are being generated for loop body operations

3. **Investigate LLVM Code Generation**
   - Check if optimization passes are removing loop body operations
   - Try generating LLVM IR with optimizations disabled
   - Add debug output to trace LLVM IR generation

**Expected Implementation:**
```cpp
// In HIRGen.cpp - While loop generation
void visit(WhileStmt& node) override {
    // Create entry block
    auto entryBlock = builder_->createBasicBlock("while.entry");
    
    // Create condition block
    auto condBlock = builder_->createBasicBlock("while.cond");
    
    // Create body block
    auto bodyBlock = builder_->createBasicBlock("while.body");
    
    // Create end block
    auto endBlock = builder_->createBasicBlock("while.end");
    
    // Connect blocks
    builder_->createBr(entryBlock);
    builder_->setInsertPoint(entryBlock);
    builder_->createBr(condBlock);
    
    builder_->setInsertPoint(condBlock);
    node.condition->accept(*this);
    builder_->createCondBr(lastValue_, bodyBlock, endBlock);
    
    builder_->setInsertPoint(bodyBlock);
    node.body->accept(*this);  // This should generate the loop body operations
    builder_->createBr(condBlock);
    
    builder_->setInsertPoint(endBlock);
}
```

### 2. Implement Break/Continue Statements

**Current State:** Basic structure exists but returns from function instead of breaking/continuing loop.

**Required Implementation:**
1. **Loop Context Tracking**
   ```cpp
   // In HIRGen.h
   class HIRGenerator {
   private:
       struct LoopContext {
           HIRBasicBlock* breakTarget;
           HIRBasicBlock* continueTarget;
           std::shared_ptr<LoopContext> parent;
       };
       
       std::shared_ptr<LoopContext> currentLoopContext_;
   };
   ```

2. **Update Loop Generation**
   ```cpp
   // In HIRGen.cpp - While loop generation
   void visit(WhileStmt& node) override {
       // Create blocks
       auto entryBlock = builder_->createBasicBlock("while.entry");
       auto condBlock = builder_->createBasicBlock("while.cond");
       auto bodyBlock = builder_->createBasicBlock("while.body");
       auto endBlock = builder_->createBasicBlock("while.end");
       
       // Set up loop context
       auto loopContext = std::make_shared<LoopContext>();
       loopContext->breakTarget = endBlock.get();
       loopContext->continueTarget = condBlock.get();
       loopContext->parent = currentLoopContext_;
       currentLoopContext_ = loopContext;
       
       // Generate loop structure...
       
       // Restore previous context
       currentLoopContext_ = loopContext->parent;
   }
   ```

3. **Implement Break/Continue Generation**
   ```cpp
   // In HIRGen.cpp
   void visit(BreakStmt& node) override {
       if (currentLoopContext_) {
           builder_->createBr(currentLoopContext_->breakTarget);
       } else {
           // Error: break outside loop
       }
   }
   
   void visit(ContinueStmt& node) override {
       if (currentLoopContext_) {
           builder_->createBr(currentLoopContext_->continueTarget);
       } else {
           // Error: continue outside loop
       }
   }
   ```

## Medium-term Priorities

### 1. Implement Array Support

**Required Components:**
1. **AST Extensions**
   - Array type support
   - Array literal syntax
   - Array indexing operations

2. **HIR Extensions**
   - Array type representation
   - Array allocation operations
   - Array access operations

3. **LLVM Code Generation**
   - Map array types to LLVM types
   - Generate memory allocation for arrays
   - Generate array bounds checking (optional)

**Expected Implementation:**
```cpp
// In HIR.h - Array type
class HIRArrayType : public HIRType {
public:
    HIRTypePtr elementType;
    size_t size;
    
    HIRArrayType(HIRTypePtr elemType, size_t sz) 
        : HIRType(Kind::Array), elementType(elemType), size(sz) {}
};

// In HIRGen.cpp - Array indexing
void visit(IndexExpr& node) override {
    node.object->accept(*this);
    auto arrayValue = lastValue_;
    
    node.index->accept(*this);
    auto indexValue = lastValue_;
    
    // Create array access operation
    auto accessInst = builder_->createArrayAccess(arrayValue, indexValue);
    lastValue_ = accessInst;
}
```

### 2. Implement Struct Support

**Required Components:**
1. **AST Extensions**
   - Struct type definitions
   - Field access operations

2. **HIR Extensions**
   - Struct type representation
   - Field access operations

3. **LLVM Code Generation**
   - Map struct types to LLVM types
   - Generate field access operations

### 3. Improve Error Handling

**Required Components:**
1. **Error Reporting**
   - Detailed error messages
   - Source location information
   - Error recovery mechanisms

2. **Type Checking**
   - Comprehensive type checking
   - Type error reporting
   - Type inference improvements

## Long-term Priorities

### 1. Optimization Passes

**Required Components:**
1. **HIR Optimizations**
   - Constant folding
   - Dead code elimination
   - Algebraic simplifications

2. **MIR Optimizations**
   - Copy propagation
   - Register allocation
   - Instruction scheduling

3. **LLVM Integration**
   - Custom optimization passes
   - Target-specific optimizations

### 2. Standard Library

**Required Components:**
1. **Basic I/O Operations**
   - Console output
   - Console input
   - File operations

2. **String Operations**
   - String concatenation
   - String comparison
   - String manipulation

3. **Mathematical Functions**
   - Basic math functions
   - Trigonometric functions
   - Random number generation

## Implementation Strategy

### 1. Incremental Development

1. **Focus on One Feature at a Time**
   - Implement loop body operations first
   - Then implement break/continue statements
   - Add array support incrementally

2. **Test-Driven Development**
   - Write tests for each feature
   - Ensure existing tests continue to pass
   - Add regression tests for fixed bugs

3. **Documentation**
   - Document each feature as it's implemented
   - Update architecture documentation
   - Add examples and tutorials

### 2. Debugging Strategy

1. **Comprehensive Logging**
   - Add debug output to trace compilation stages
   - Use conditional compilation for debug code
   - Implement logging levels

2. **IR Dumping**
   - Add options to dump HIR, MIR, and LLVM IR
   - Implement pretty-printing for IRs
   - Add comparison utilities for testing

3. **Unit Testing**
   - Test individual components
   - Test compilation stages in isolation
   - Test error conditions

### 3. Performance Considerations

1. **Compilation Speed**
   - Optimize AST traversal
   - Implement efficient data structures
   - Consider parallel compilation

2. **Code Quality**
   - Optimize generated code
   - Implement specific optimizations
   - Benchmark against other compilers

## Conclusion

The Nova Compiler has a solid foundation with a well-designed architecture. The immediate priorities are to fix loop body operations and implement break/continue statements. These improvements will make the compiler more useful for practical programming tasks.

The modular design of the compiler makes it easy to add new features incrementally. By following the incremental development strategy and maintaining comprehensive tests, we can ensure that each new feature is implemented correctly without breaking existing functionality.

The long-term vision for the Nova Compiler includes support for advanced data types, comprehensive optimization, and a rich standard library. This will make Nova a practical and powerful programming language suitable for a wide range of applications.