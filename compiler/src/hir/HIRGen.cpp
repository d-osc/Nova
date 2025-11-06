#include "nova/HIR/HIRGen.h"
#include "nova/HIR/HIR.h"
#include "nova/Frontend/AST.h"
#include <memory>
#include <unordered_map>
#include <variant>
#include <functional>

namespace nova::hir {

class HIRGenerator : public ASTVisitor {
public:
    explicit HIRGenerator(HIRModule* module)
        : module_(module), builder_(nullptr), currentFunction_(nullptr) {}
    
    HIRModule* getModule() { return module_; }
    
    // Expressions
    void visit(NumberLiteral& node) override {
        // Create numeric constant
        // Check if the value is an integer
        if (node.value == static_cast<int64_t>(node.value)) {
            lastValue_ = builder_->createIntConstant(static_cast<int64_t>(node.value));
        } else {
            lastValue_ = builder_->createFloatConstant(node.value);
        }
    }
    
    void visit(StringLiteral& node) override {
        lastValue_ = builder_->createStringConstant(node.value);
    }
    
    void visit(BooleanLiteral& node) override {
        lastValue_ = builder_->createBoolConstant(node.value);
    }
    
    void visit(NullLiteral& node) override {
        (void)node;
        auto nullType = std::make_shared<HIRType>(HIRType::Kind::Any);
        lastValue_ = builder_->createNullConstant(nullType.get());
    }
    
    void visit(UndefinedLiteral& node) override {
        (void)node;
        auto undefType = std::make_shared<HIRType>(HIRType::Kind::Unknown);
        lastValue_ = builder_->createNullConstant(undefType.get());
    }
    
    void visit(Identifier& node) override {
        // Look up variable in symbol table
        auto it = symbolTable_.find(node.name);
        if (it != symbolTable_.end()) {
            lastValue_ = it->second;
        }
    }
    
    void visit(BinaryExpr& node) override {
        // Generate left operand
        node.left->accept(*this);
        auto lhs = lastValue_;
        
        // Generate right operand
        node.right->accept(*this);
        auto rhs = lastValue_;
        
        // Generate operation based on operator
        using Op = BinaryExpr::Op;
        switch (node.op) {
            case Op::Add:
                lastValue_ = builder_->createAdd(lhs, rhs);
                break;
            case Op::Sub:
                lastValue_ = builder_->createSub(lhs, rhs);
                break;
            case Op::Mul:
                lastValue_ = builder_->createMul(lhs, rhs);
                break;
            case Op::Div:
                lastValue_ = builder_->createDiv(lhs, rhs);
                break;
            case Op::Equal:
                lastValue_ = builder_->createEq(lhs, rhs);
                break;
            case Op::NotEqual:
                lastValue_ = builder_->createNe(lhs, rhs);
                break;
            case Op::Less:
                lastValue_ = builder_->createLt(lhs, rhs);
                break;
            case Op::LessEqual:
                lastValue_ = builder_->createLe(lhs, rhs);
                break;
            case Op::Greater:
                lastValue_ = builder_->createGt(lhs, rhs);
                break;
            case Op::GreaterEqual:
                lastValue_ = builder_->createGe(lhs, rhs);
                break;
            default:
                // Add more operators as needed
                break;
        }
    }
    
    void visit(UnaryExpr& node) override {
        node.operand->accept(*this);
        auto operand = lastValue_;
        
        using Op = UnaryExpr::Op;
        switch (node.op) {
            case Op::Minus:
                // Negate
                {
                    auto zero = builder_->createIntConstant(0);
                    lastValue_ = builder_->createSub(zero, operand);
                }
                break;
            case Op::Not:
                // Logical not - compare with false
                {
                    auto falsVal = builder_->createBoolConstant(false);
                    lastValue_ = builder_->createEq(operand, falsVal);
                }
                break;
            default:
                // Other operators
                break;
        }
    }
    
    void visit(UpdateExpr& node) override {
        // ++x or x++ implementation
        node.argument->accept(*this);
    }
    
    void visit(CallExpr& node) override {
        // Generate callee
        node.callee->accept(*this);
        
        // Generate arguments
        std::vector<HIRValue*> args;
        for (auto& arg : node.arguments) {
            arg->accept(*this);
            args.push_back(lastValue_);
        }
        
        // Lookup function
        if (auto* id = dynamic_cast<Identifier*>(node.callee.get())) {
            auto func = module_->getFunction(id->name);
            if (func) {
                lastValue_ = builder_->createCall(func.get(), args);
            }
        }
    }
    
    void visit(MemberExpr& node) override {
        node.object->accept(*this);
        // Generate field access
    }
    
    void visit(ConditionalExpr& node) override {
        // Ternary operator: test ? consequent : alternate
        node.test->accept(*this);
    }
    
    void visit(ArrayExpr& node) override {
        (void)node;
        // Array literal construction
    }
    
    void visit(ObjectExpr& node) override {
        (void)node;
        // Object literal construction  
    }
    
    void visit(FunctionExpr& node) override {
        (void)node;
        // Anonymous function expression
    }
    
    void visit(ArrowFunctionExpr& node) override {
        (void)node;
        // Arrow function expression
    }
    
    void visit(ClassExpr& node) override {
        (void)node;
        // Class expression
    }
    
    void visit(NewExpr& node) override {
        (void)node;
        // new expression
    }
    
    void visit(ThisExpr& node) override {
        (void)node;
        // this reference
    }
    
    void visit(SuperExpr& node) override {
        (void)node;
        // super reference
    }
    
    void visit(SpreadExpr& node) override {
        (void)node;
        // Spread operator
    }
    
    void visit(TemplateLiteralExpr& node) override {
        (void)node;
        // Template literal
    }
    
    void visit(AwaitExpr& node) override {
        // await expression
        node.argument->accept(*this);
    }
    
    void visit(YieldExpr& node) override {
        // yield expression
        if (node.argument) {
            node.argument->accept(*this);
        }
    }
    
    void visit(AsExpr& node) override {
        // Type assertion - just evaluate expression
        node.expression->accept(*this);
    }
    
    void visit(SatisfiesExpr& node) override {
        // Satisfies operator - just evaluate expression
        node.expression->accept(*this);
    }
    
    void visit(NonNullExpr& node) override {
        // Non-null assertion - just evaluate expression
        node.expression->accept(*this);
    }
    
    void visit(TaggedTemplateExpr& node) override {
        (void)node;
        // Tagged template
    }
    
    void visit(SequenceExpr& node) override {
        // Comma operator - evaluate all, return last
        for (auto& expr : node.expressions) {
            expr->accept(*this);
        }
    }
    
    void visit(AssignmentExpr& node) override {
        // Generate right side
        node.right->accept(*this);
        auto value = lastValue_;
        
        // Store to left side
        if (auto* id = dynamic_cast<Identifier*>(node.left.get())) {
            auto it = symbolTable_.find(id->name);
            if (it != symbolTable_.end()) {
                builder_->createStore(value, it->second);
            }
        }
    }
    
    void visit(ParenthesizedExpr& node) override {
        node.expression->accept(*this);
    }
    
    void visit(MetaProperty& node) override {
        (void)node;
        // new.target or import.meta
    }
    
    void visit(ImportExpr& node) override {
        (void)node;
        // import() expression
    }
    
    void visit(Decorator& node) override {
        (void)node;
        // Decorator - metadata only
    }
    
    // JSX/TSX Expressions
    void visit(JSXElement& node) override {
        (void)node;
        // JSX element - translate to runtime createElement call
        // For now, treat as opaque object creation
        // TODO: Implement JSX transformation to React.createElement or similar
        lastValue_ = builder_->createNullConstant(
            std::make_shared<HIRType>(HIRType::Kind::Any).get()
        );
    }
    
    void visit(JSXFragment& node) override {
        (void)node;
        // JSX fragment - translate to Fragment component
        lastValue_ = builder_->createNullConstant(
            std::make_shared<HIRType>(HIRType::Kind::Any).get()
        );
    }
    
    void visit(JSXText& node) override {
        // JSX text node - convert to string constant
        lastValue_ = builder_->createStringConstant(node.value);
    }
    
    void visit(JSXExpressionContainer& node) override {
        // JSX expression container - just evaluate inner expression
        node.expression->accept(*this);
    }
    
    void visit(JSXAttribute& node) override {
        // JSX attribute - not yet implemented
        (void)node;
    }
    
    void visit(JSXSpreadAttribute& node) override {
        // JSX spread attribute - not yet implemented
        (void)node;
    }
    
    // Patterns (for destructuring)
    void visit(ObjectPattern& node) override {
        (void)node;
        // Object destructuring pattern
        // This is used in variable declarations and function parameters
        // TODO: Implement proper destructuring logic
    }
    
    void visit(ArrayPattern& node) override {
        (void)node;
        // Array destructuring pattern
        // TODO: Implement proper destructuring logic
    }
    
    void visit(AssignmentPattern& node) override {
        (void)node;
        // Pattern with default value
        // TODO: Implement default value assignment
    }
    
    void visit(RestElement& node) override {
        (void)node;
        // Rest element in destructuring (...rest)
        // TODO: Implement rest element collection
    }
    
    void visit(IdentifierPattern& node) override {
        // Simple identifier pattern - look up in symbol table
        auto it = symbolTable_.find(node.name);
        if (it != symbolTable_.end()) {
            lastValue_ = it->second;
        }
    }
    
    // Statements
    void visit(BlockStmt& node) override {
        for (auto& stmt : node.statements) {
            stmt->accept(*this);
        }
    }
    
    void visit(ExprStmt& node) override {
        node.expression->accept(*this);
    }
    
    void visit(VarDeclStmt& node) override {
        for (auto& decl : node.declarations) {
            // Allocate storage
            auto i64Type = std::make_shared<HIRType>(HIRType::Kind::I64);
            auto alloca = builder_->createAlloca(i64Type.get(), decl.name);
            symbolTable_[decl.name] = alloca;
            
            // Initialize if there's an initializer
            if (decl.init) {
                decl.init->accept(*this);
                builder_->createStore(lastValue_, alloca);
            }
        }
    }
    
    void visit(DeclStmt& node) override {
        // Process the declaration within this statement
        if (node.declaration) {
            node.declaration->accept(*this);
        }
    }
    
    void visit(IfStmt& node) override {
        // Generate condition
        node.test->accept(*this);
        auto cond = lastValue_;
        
        // Create blocks
        auto* thenBlock = currentFunction_->createBasicBlock("if.then").get();
        auto* elseBlock = node.alternate ? 
            currentFunction_->createBasicBlock("if.else").get() : nullptr;
        auto* endBlock = currentFunction_->createBasicBlock("if.end").get();
        
        // Branch on condition
        if (elseBlock) {
            builder_->createCondBr(cond, thenBlock, elseBlock);
        } else {
            builder_->createCondBr(cond, thenBlock, endBlock);
        }
        
        // Generate then block
        builder_->setInsertPoint(thenBlock);
        node.consequent->accept(*this);
        
        // Only add branch to end block if the then block doesn't end with a return, break, or continue
        if (thenBlock->instructions.empty() || 
            (thenBlock->instructions.back()->opcode != hir::HIRInstruction::Opcode::Return &&
             thenBlock->instructions.back()->opcode != hir::HIRInstruction::Opcode::Break &&
             thenBlock->instructions.back()->opcode != hir::HIRInstruction::Opcode::Continue)) {
            builder_->createBr(endBlock);
        }
        
        // Generate else block
        if (elseBlock) {
            builder_->setInsertPoint(elseBlock);
            node.alternate->accept(*this);
            
            // Only add branch to end block if the else block doesn't end with a return, break, or continue
            if (elseBlock->instructions.empty() || 
                (elseBlock->instructions.back()->opcode != hir::HIRInstruction::Opcode::Return &&
                 elseBlock->instructions.back()->opcode != hir::HIRInstruction::Opcode::Break &&
                 elseBlock->instructions.back()->opcode != hir::HIRInstruction::Opcode::Continue)) {
                builder_->createBr(endBlock);
            }
        }
        
        // Continue at end block
        builder_->setInsertPoint(endBlock);
        
        // If end block is empty (both branches had returns), add unreachable
        if (builder_->getInsertBlock()->instructions.empty()) {
            // Create a dummy return instruction
            auto dummyConst = builder_->createIntConstant(0);
            builder_->createReturn(dummyConst);
        }
    }
    
    void visit(WhileStmt& node) override {
        std::cerr << "DEBUG: Entering WhileStmt generation" << std::endl;
        
        auto* condBlock = currentFunction_->createBasicBlock("while.cond").get();
        auto* bodyBlock = currentFunction_->createBasicBlock("while.body").get();
        auto* endBlock = currentFunction_->createBasicBlock("while.end").get();
        
        std::cerr << "DEBUG: Created while loop blocks: cond=" << condBlock << ", body=" << bodyBlock << ", end=" << endBlock << std::endl;
        
        // Jump to condition
        builder_->createBr(condBlock);
        
        // Condition block
        builder_->setInsertPoint(condBlock);
        std::cerr << "DEBUG: Evaluating while condition" << std::endl;
        node.test->accept(*this);
        std::cerr << "DEBUG: While condition evaluated, lastValue_=" << lastValue_ << std::endl;
        builder_->createCondBr(lastValue_, bodyBlock, endBlock);
        
        // Body block
        builder_->setInsertPoint(bodyBlock);
        std::cerr << "DEBUG: Executing while body" << std::endl;
        node.body->accept(*this);
        std::cerr << "DEBUG: While body executed" << std::endl;
        
        // Check if the body or any of its successors contain any break or continue instructions
        bool hasBreakOrContinue = bodyBlock->hasBreakOrContinue;
        
        // Check all successors of the body block
        std::function<void(hir::HIRBasicBlock*, bool&)> checkSuccessors = [&](hir::HIRBasicBlock* block, bool& found) {
            if (found) return;
            if (block->hasBreakOrContinue) {
                found = true;
                return;
            }
            for (const auto& succ : block->successors) {
                checkSuccessors(succ.get(), found);
            }
        };
        
        checkSuccessors(bodyBlock, hasBreakOrContinue);
        
        // Only add branch back to condition if the body doesn't contain break/continue and doesn't end with a return
        if (!hasBreakOrContinue && (bodyBlock->instructions.empty() || 
            bodyBlock->instructions.back()->opcode != hir::HIRInstruction::Opcode::Return)) {
            std::cerr << "DEBUG: Creating branch back to condition" << std::endl;
            builder_->createBr(condBlock);
        } else {
            std::cerr << "DEBUG: Not creating branch back to condition because body or its successors contain break/continue or body ends with return" << std::endl;
        }
        
        // End block
        builder_->setInsertPoint(endBlock);
        std::cerr << "DEBUG: While loop generation completed" << std::endl;
    }
    
    void visit(DoWhileStmt& node) override {
        // Create basic blocks for the do-while loop
        auto* bodyBlock = currentFunction_->createBasicBlock("do-while.body").get();
        auto* condBlock = currentFunction_->createBasicBlock("do-while.cond").get();
        auto* endBlock = currentFunction_->createBasicBlock("do-while.end").get();
        
        // Jump to body block (do-while always executes at least once)
        builder_->createBr(bodyBlock);
        
        // Body block
        builder_->setInsertPoint(bodyBlock);
        node.body->accept(*this);
        
        // Check if the body or any of its successors contain any break or continue instructions
        bool hasBreakOrContinue = bodyBlock->hasBreakOrContinue;
        
        // Check all successors of the body block
        std::function<void(hir::HIRBasicBlock*, bool&)> checkSuccessors = [&](hir::HIRBasicBlock* block, bool& found) {
            if (found) return;
            if (block->hasBreakOrContinue) {
                found = true;
                return;
            }
            for (const auto& succ : block->successors) {
                checkSuccessors(succ.get(), found);
            }
        };
        
        checkSuccessors(bodyBlock, hasBreakOrContinue);
        
        // Only add branch to condition if the body doesn't contain break/continue and doesn't end with a return
        if (!hasBreakOrContinue && (bodyBlock->instructions.empty() || 
            bodyBlock->instructions.back()->opcode != hir::HIRInstruction::Opcode::Return)) {
            // Branch to condition after body
            builder_->createBr(condBlock);
        }
        
        // Condition block
        builder_->setInsertPoint(condBlock);
        node.test->accept(*this);
        auto* condition = lastValue_;
        // If condition is true, branch back to body, otherwise go to end
        builder_->createCondBr(condition, bodyBlock, endBlock);
        
        // End block
        builder_->setInsertPoint(endBlock);
    }
    
    void visit(ForStmt& node) override {
        std::cerr << "DEBUG: Entering ForStmt generation" << std::endl;
        
        // Create basic blocks for the for loop
        auto* initBlock = currentFunction_->createBasicBlock("for.init").get();
        auto* condBlock = currentFunction_->createBasicBlock("for.cond").get();
        auto* bodyBlock = currentFunction_->createBasicBlock("for.body").get();
        auto* updateBlock = currentFunction_->createBasicBlock("for.update").get();
        auto* endBlock = currentFunction_->createBasicBlock("for.end").get();
        
        std::cerr << "DEBUG: Created for loop blocks: init=" << initBlock << ", cond=" << condBlock 
                  << ", body=" << bodyBlock << ", update=" << updateBlock << ", end=" << endBlock << std::endl;
        
        // Branch to init block
        builder_->createBr(initBlock);
        
        // Init block - execute initializer
        builder_->setInsertPoint(initBlock);
        std::cerr << "DEBUG: Executing for init" << std::endl;
        if (node.init) {
            if (auto* varDeclStmt = dynamic_cast<VarDeclStmt*>(node.init.get())) {
                varDeclStmt->accept(*this);
            } else if (auto* exprStmt = dynamic_cast<ExprStmt*>(node.init.get())) {
                exprStmt->accept(*this);
            } else {
                // For expression initializers, wrap in an expression statement
                node.init->accept(*this);
            }
        }
        std::cerr << "DEBUG: For init executed" << std::endl;
        // Branch to condition
        builder_->createBr(condBlock);
        
        // Condition block
        builder_->setInsertPoint(condBlock);
        std::cerr << "DEBUG: Evaluating for condition" << std::endl;
        if (node.test) {
            node.test->accept(*this);
            auto* condition = lastValue_;
            std::cerr << "DEBUG: For condition evaluated, condition=" << condition << std::endl;
            builder_->createCondBr(condition, bodyBlock, endBlock);
        } else {
            // No condition means infinite loop
            std::cerr << "DEBUG: No for condition, creating infinite loop" << std::endl;
            builder_->createBr(bodyBlock);
        }
        
        // Body block
        builder_->setInsertPoint(bodyBlock);
        std::cerr << "DEBUG: Executing for body" << std::endl;
        node.body->accept(*this);
        std::cerr << "DEBUG: For body executed" << std::endl;
        
        // Check if the body or any of its successors contain any break or continue instructions
        bool hasBreakOrContinue = bodyBlock->hasBreakOrContinue;
        
        // Check all successors of the body block
        std::function<void(hir::HIRBasicBlock*, bool&)> checkSuccessors = [&](hir::HIRBasicBlock* block, bool& found) {
            if (found) return;
            if (block->hasBreakOrContinue) {
                found = true;
                return;
            }
            for (const auto& succ : block->successors) {
                checkSuccessors(succ.get(), found);
            }
        };
        
        checkSuccessors(bodyBlock, hasBreakOrContinue);
        
        // Only add branch to update block if the body doesn't contain break/continue and doesn't end with a return
        if (!hasBreakOrContinue && (bodyBlock->instructions.empty() || 
            bodyBlock->instructions.back()->opcode != hir::HIRInstruction::Opcode::Return)) {
            // Branch to update block
            builder_->createBr(updateBlock);
        }
        
        // Update block
        builder_->setInsertPoint(updateBlock);
        std::cerr << "DEBUG: Executing for update" << std::endl;
        if (node.update) {
            node.update->accept(*this);
            // Result of update expression is ignored
        }
        std::cerr << "DEBUG: For update executed" << std::endl;
        // Branch back to condition
        builder_->createBr(condBlock);
        
        // End block
        builder_->setInsertPoint(endBlock);
        std::cerr << "DEBUG: For loop generation completed" << std::endl;
    }
    
    void visit(ForInStmt& node) override {
        (void)node;
        // for-in loop
    }
    
    void visit(ForOfStmt& node) override {
        (void)node;
        // for-of loop
    }
    
    void visit(ReturnStmt& node) override {
        if (node.argument) {
            node.argument->accept(*this);
            builder_->createReturn(lastValue_);
        } else {
            builder_->createReturn(nullptr);
        }
    }
    
    void visit(BreakStmt& node) override {
        (void)node;
        // Create break instruction
        auto voidType = std::make_shared<HIRType>(HIRType::Kind::Void);
        auto breakInst = std::make_unique<HIRInstruction>(
            HIRInstruction::Opcode::Break,
            voidType,
            ""
        );
        auto* currentBlock = builder_->getInsertBlock();
        currentBlock->addInstruction(std::move(breakInst));
        currentBlock->hasBreakOrContinue = true;
    }
    
    void visit(ContinueStmt& node) override {
        (void)node;
        // Create continue instruction
        auto voidType = std::make_shared<HIRType>(HIRType::Kind::Void);
        auto continueInst = std::make_unique<HIRInstruction>(
            HIRInstruction::Opcode::Continue,
            voidType,
            ""
        );
        auto* currentBlock = builder_->getInsertBlock();
        currentBlock->addInstruction(std::move(continueInst));
        currentBlock->hasBreakOrContinue = true;
    }
    
    void visit(ThrowStmt& node) override {
        // throw statement
        node.argument->accept(*this);
    }
    
    void visit(TryStmt& node) override {
        (void)node;
        // try-catch-finally
    }
    
    void visit(SwitchStmt& node) override {
        (void)node;
        // switch statement
    }
    
    void visit(LabeledStmt& node) override {
        // labeled statement
        node.statement->accept(*this);
    }
    
    void visit(WithStmt& node) override {
        (void)node;
        // with statement
    }
    
    void visit(DebuggerStmt& node) override {
        (void)node;
        // debugger statement - no-op in HIR
    }
    
    void visit(EmptyStmt& node) override {
        (void)node;
        // empty statement - no-op
    }
    
    // Declarations
    void visit(FunctionDecl& node) override {
        // Create function type
        std::vector<HIRTypePtr> paramTypes;
        for (size_t i = 0; i < node.params.size(); ++i) {
            paramTypes.push_back(std::make_shared<HIRType>(HIRType::Kind::Any));
        }
        auto retType = std::make_shared<HIRType>(HIRType::Kind::Any);
        
        auto funcType = new HIRFunctionType(paramTypes, retType);
        
        // Create function
        auto func = module_->createFunction(node.name, funcType);
        func->isAsync = node.isAsync;
        func->isGenerator = node.isGenerator;
        
        currentFunction_ = func.get();
        
        // Create entry block
        auto entryBlock = func->createBasicBlock("entry");
        
        // Create builder
        builder_ = std::make_unique<HIRBuilder>(module_, func.get());
        builder_->setInsertPoint(entryBlock.get());
        
        // Add parameters to symbol table
        for (size_t i = 0; i < node.params.size(); ++i) {
            symbolTable_[node.params[i]] = func->parameters[i];
        }
        
        // Generate function body
        if (node.body) {
            node.body->accept(*this);
        }
        
        // Add implicit return if needed
        if (!entryBlock->hasTerminator()) {
            builder_->createReturn(nullptr);
        }
        
        currentFunction_ = nullptr;
    }
    
    void visit(ClassDecl& node) override {
        // Create struct type for class
        (void)module_->createStructType(node.name);
    }
    
    void visit(InterfaceDecl& node) override {
        (void)node;
        // Interface - type information only
    }
    
    void visit(TypeAliasDecl& node) override {
        (void)node;
        // Type alias - type information only
    }
    
    void visit(EnumDecl& node) override {
        (void)node;
        // Enum declaration
    }
    
    void visit(ImportDecl& node) override {
        (void)node;
        // Import declaration - module system
    }
    
    void visit(ExportDecl& node) override {
        (void)node;
        // Export declaration
    }
    
    void visit(Program& node) override {
        for (auto& stmt : node.body) {
            stmt->accept(*this);
        }
    }
    
private:
    HIRModule* module_;
    std::unique_ptr<HIRBuilder> builder_;
    HIRFunction* currentFunction_;
    HIRValue* lastValue_ = nullptr;
    std::unordered_map<std::string, HIRValue*> symbolTable_;
};

// Public API to generate HIR from AST
HIRModule* generateHIR(Program& program, const std::string& moduleName) {
    auto* module = new HIRModule(moduleName);
    HIRGenerator generator(module);
    program.accept(generator);
    return module;
}

} // namespace nova::hir
