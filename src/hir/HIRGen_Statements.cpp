// HIRGen_Statements.cpp - Statement visitors
// Extracted from HIRGen.cpp for better code organization

#include "nova/HIR/HIRGen_Internal.h"
#define NOVA_DEBUG 0

namespace nova::hir {

namespace {
enum class DynamicKind {
    Unknown, Number, String, Boolean, Null, Undefined, Object, Function
};

DynamicKind expressionKind(Expr* expression) {
    if (!expression) return DynamicKind::Undefined;
    if (dynamic_cast<NumberLiteral*>(expression)) return DynamicKind::Number;
    if (dynamic_cast<StringLiteral*>(expression) ||
        dynamic_cast<TemplateLiteralExpr*>(expression)) return DynamicKind::String;
    if (dynamic_cast<BooleanLiteral*>(expression)) return DynamicKind::Boolean;
    if (dynamic_cast<NullLiteral*>(expression)) return DynamicKind::Null;
    if (dynamic_cast<UndefinedLiteral*>(expression)) return DynamicKind::Undefined;
    if (dynamic_cast<ArrayExpr*>(expression) || dynamic_cast<ObjectExpr*>(expression) ||
        dynamic_cast<NewExpr*>(expression) || dynamic_cast<RegexLiteralExpr*>(expression)) {
        return DynamicKind::Object;
    }
    if (dynamic_cast<FunctionExpr*>(expression) ||
        dynamic_cast<ArrowFunctionExpr*>(expression)) return DynamicKind::Function;
    if (auto* parenthesized = dynamic_cast<ParenthesizedExpr*>(expression)) {
        return expressionKind(parenthesized->expression.get());
    }
    if (auto* assertion = dynamic_cast<AsExpr*>(expression)) {
        return expressionKind(assertion->expression.get());
    }
    if (auto* nonNull = dynamic_cast<NonNullExpr*>(expression)) {
        return expressionKind(nonNull->expression.get());
    }
    if (auto* conditional = dynamic_cast<ConditionalExpr*>(expression)) {
        const auto left = expressionKind(conditional->consequent.get());
        const auto right = expressionKind(conditional->alternate.get());
        return left == right ? left : DynamicKind::Unknown;
    }
    if (auto* binary = dynamic_cast<BinaryExpr*>(expression)) {
        using Op = BinaryExpr::Op;
        switch (binary->op) {
            case Op::Equal: case Op::NotEqual: case Op::StrictEqual:
            case Op::StrictNotEqual: case Op::Less: case Op::LessEqual:
            case Op::Greater: case Op::GreaterEqual: case Op::In:
            case Op::Instanceof:
                return DynamicKind::Boolean;
            case Op::Add:
                return expressionKind(binary->left.get()) == DynamicKind::String ||
                       expressionKind(binary->right.get()) == DynamicKind::String
                    ? DynamicKind::String : DynamicKind::Number;
            case Op::LogicalAnd: case Op::LogicalOr: case Op::NullishCoalescing:
                return DynamicKind::Unknown;
            default:
                return DynamicKind::Number;
        }
    }
    if (auto* assignment = dynamic_cast<AssignmentExpr*>(expression)) {
        return expressionKind(assignment->right.get());
    }
    return DynamicKind::Unknown;
}

class DynamicBindingScanner {
public:
    explicit DynamicBindingScanner(bool visitNestedFunctions = false)
        : visitNestedFunctions_(visitNestedFunctions) {}

    std::unordered_set<std::string> result;
    std::unordered_set<int> returnKinds;
    std::unordered_map<std::string, std::vector<std::unordered_set<int>>> callKinds;

    void scanExpression(Expr* expression) {
        if (!expression) return;
        if (auto* assignment = dynamic_cast<AssignmentExpr*>(expression)) {
            scanExpression(assignment->right.get());
            if (assignment->pattern) {
                std::function<void(Pattern*)> mark = [&](Pattern* pattern) {
                    if (!pattern) return;
                    if (auto* identifier =
                            dynamic_cast<IdentifierPattern*>(pattern)) {
                        result.insert(identifier->name);
                    } else if (auto* value =
                                   dynamic_cast<AssignmentPattern*>(pattern)) {
                        mark(value->left.get());
                    } else if (auto* rest =
                                   dynamic_cast<RestElement*>(pattern)) {
                        mark(rest->argument.get());
                    } else if (auto* array =
                                   dynamic_cast<ArrayPattern*>(pattern)) {
                        for (auto& element : array->elements) mark(element.get());
                    } else if (auto* object =
                                   dynamic_cast<ObjectPattern*>(pattern)) {
                        for (auto& property : object->properties) {
                            mark(property.value.get());
                        }
                    }
                };
                mark(assignment->pattern.get());
                return;
            }
            if (auto* identifier = dynamic_cast<Identifier*>(assignment->left.get())) {
                auto found = bindings.find(identifier->name);
                if (found != bindings.end()) {
                    auto assigned = expressionKind(assignment->right.get());
                    if (assignment->op != AssignmentExpr::Op::Assign) {
                        assigned = assignment->op == AssignmentExpr::Op::AddAssign &&
                            (found->second == DynamicKind::String ||
                             assigned == DynamicKind::String)
                            ? DynamicKind::String : DynamicKind::Number;
                    }
                    if (assigned == DynamicKind::Unknown || assigned != found->second) {
                        result.insert(identifier->name);
                    }
                }
            } else {
                scanExpression(assignment->left.get());
            }
        } else if (auto* binary = dynamic_cast<BinaryExpr*>(expression)) {
            scanExpression(binary->left.get()); scanExpression(binary->right.get());
        } else if (auto* unary = dynamic_cast<UnaryExpr*>(expression)) {
            scanExpression(unary->operand.get());
        } else if (auto* update = dynamic_cast<UpdateExpr*>(expression)) {
            scanExpression(update->argument.get());
        } else if (auto* call = dynamic_cast<CallExpr*>(expression)) {
            if (auto* identifier = dynamic_cast<Identifier*>(call->callee.get())) {
                auto& parameterKinds = callKinds[identifier->name];
                if (parameterKinds.size() < call->arguments.size()) {
                    parameterKinds.resize(call->arguments.size());
                }
                for (size_t i = 0; i < call->arguments.size(); ++i) {
                    parameterKinds[i].insert(
                        static_cast<int>(expressionKind(call->arguments[i].get())));
                }
            }
            scanExpression(call->callee.get());
            for (auto& argument : call->arguments) scanExpression(argument.get());
        } else if (auto* member = dynamic_cast<MemberExpr*>(expression)) {
            scanExpression(member->object.get()); scanExpression(member->property.get());
        } else if (auto* conditional = dynamic_cast<ConditionalExpr*>(expression)) {
            scanExpression(conditional->test.get());
            scanExpression(conditional->consequent.get());
            scanExpression(conditional->alternate.get());
        } else if (auto* array = dynamic_cast<ArrayExpr*>(expression)) {
            for (auto& element : array->elements) scanExpression(element.get());
        } else if (auto* object = dynamic_cast<ObjectExpr*>(expression)) {
            for (auto& property : object->properties) scanExpression(property.value.get());
        } else if (auto* parenthesized = dynamic_cast<ParenthesizedExpr*>(expression)) {
            scanExpression(parenthesized->expression.get());
        } else if (auto* sequence = dynamic_cast<SequenceExpr*>(expression)) {
            for (auto& item : sequence->expressions) scanExpression(item.get());
        } else if (auto* spread = dynamic_cast<SpreadExpr*>(expression)) {
            scanExpression(spread->argument.get());
        } else if (auto* awaited = dynamic_cast<AwaitExpr*>(expression)) {
            scanExpression(awaited->argument.get());
        } else if (auto* yielded = dynamic_cast<YieldExpr*>(expression)) {
            scanExpression(yielded->argument.get());
        } else if (auto* assertion = dynamic_cast<AsExpr*>(expression)) {
            scanExpression(assertion->expression.get());
        } else if (auto* satisfies = dynamic_cast<SatisfiesExpr*>(expression)) {
            scanExpression(satisfies->expression.get());
        } else if (auto* nonNull = dynamic_cast<NonNullExpr*>(expression)) {
            scanExpression(nonNull->expression.get());
        }
    }

    void scanStatement(Stmt* statement) {
        if (!statement) return;
        if (auto* block = dynamic_cast<BlockStmt*>(statement)) {
            for (auto& item : block->statements) scanStatement(item.get());
        } else if (auto* expression = dynamic_cast<ExprStmt*>(statement)) {
            scanExpression(expression->expression.get());
        } else if (auto* variables = dynamic_cast<VarDeclStmt*>(statement)) {
            for (auto& declarator : variables->declarations) {
                scanExpression(declarator.init.get());
                if (!declarator.name.empty()) {
                    bindings[declarator.name] = expressionKind(declarator.init.get());
                }
            }
        } else if (auto* conditional = dynamic_cast<IfStmt*>(statement)) {
            scanExpression(conditional->test.get());
            scanStatement(conditional->consequent.get());
            scanStatement(conditional->alternate.get());
        } else if (auto* whileLoop = dynamic_cast<WhileStmt*>(statement)) {
            scanExpression(whileLoop->test.get()); scanStatement(whileLoop->body.get());
        } else if (auto* doWhileLoop = dynamic_cast<DoWhileStmt*>(statement)) {
            scanStatement(doWhileLoop->body.get()); scanExpression(doWhileLoop->test.get());
        } else if (auto* forLoop = dynamic_cast<ForStmt*>(statement)) {
            scanStatement(forLoop->init.get()); scanExpression(forLoop->test.get());
            scanExpression(forLoop->update.get()); scanStatement(forLoop->body.get());
        } else if (auto* forInLoop = dynamic_cast<ForInStmt*>(statement)) {
            scanExpression(forInLoop->right.get()); scanStatement(forInLoop->body.get());
        } else if (auto* forOfLoop = dynamic_cast<ForOfStmt*>(statement)) {
            scanExpression(forOfLoop->right.get()); scanStatement(forOfLoop->body.get());
        } else if (auto* returned = dynamic_cast<ReturnStmt*>(statement)) {
            scanExpression(returned->argument.get());
            returnKinds.insert(static_cast<int>(expressionKind(returned->argument.get())));
        } else if (auto* thrown = dynamic_cast<ThrowStmt*>(statement)) {
            scanExpression(thrown->argument.get());
        } else if (auto* labeled = dynamic_cast<LabeledStmt*>(statement)) {
            scanStatement(labeled->statement.get());
        } else if (auto* switchStatement = dynamic_cast<SwitchStmt*>(statement)) {
            scanExpression(switchStatement->discriminant.get());
            for (auto& item : switchStatement->cases) {
                scanExpression(item->test.get());
                for (auto& consequent : item->consequent) scanStatement(consequent.get());
            }
        } else if (auto* tryStatement = dynamic_cast<TryStmt*>(statement)) {
            scanStatement(tryStatement->block.get());
            if (tryStatement->handler) scanStatement(tryStatement->handler->body.get());
            scanStatement(tryStatement->finalizer.get());
        } else if (visitNestedFunctions_) {
            if (auto* declaration = dynamic_cast<DeclStmt*>(statement)) {
                if (auto* function = dynamic_cast<FunctionDecl*>(declaration->declaration.get())) {
                    scanStatement(function->body.get());
                }
            }
        }
    }

private:
    bool visitNestedFunctions_;
    std::unordered_map<std::string, DynamicKind> bindings;
};
} // namespace

std::unordered_set<std::string> HIRGenerator::analyzeDynamicBindings(Stmt* statement) {
    DynamicBindingScanner scanner;
    scanner.scanStatement(statement);
    return scanner.result;
}

bool HIRGenerator::hasHeterogeneousReturns(Stmt* statement) {
    DynamicBindingScanner scanner;
    scanner.scanStatement(statement);
    return scanner.returnKinds.size() > 1;
}

std::unordered_map<std::string, std::vector<HIRType::Kind>>
HIRGenerator::analyzeFunctionParameterTypes(Program& program) {
    DynamicBindingScanner scanner(true);
    for (auto& statement : program.body) {
        scanner.scanStatement(statement.get());
    }

    std::unordered_map<std::string, std::vector<HIRType::Kind>> inferred;
    for (const auto& [functionName, observedParameters] : scanner.callKinds) {
        auto& parameterTypes = inferred[functionName];
        parameterTypes.reserve(observedParameters.size());

        for (const auto& observedKinds : observedParameters) {
            HIRType::Kind type = HIRType::Kind::JSValue;
            if (observedKinds.size() == 1) {
                switch (static_cast<DynamicKind>(*observedKinds.begin())) {
                    case DynamicKind::Number: type = HIRType::Kind::I64; break;
                    case DynamicKind::String: type = HIRType::Kind::String; break;
                    case DynamicKind::Boolean: type = HIRType::Kind::Bool; break;
                    case DynamicKind::Object: type = HIRType::Kind::Pointer; break;
                    case DynamicKind::Function: type = HIRType::Kind::Function; break;
                    case DynamicKind::Unknown:
                    case DynamicKind::Null:
                    case DynamicKind::Undefined:
                        type = HIRType::Kind::JSValue;
                        break;
                }
            }
            parameterTypes.push_back(type);
        }
    }
    return inferred;
}

void HIRGenerator::visit(BlockStmt& node) {
        for (auto& stmt : node.statements) {
            stmt->accept(*this);
        }
    }
    
void HIRGenerator::visit(ExprStmt& node) {
        if (node.expression) {
            node.expression->accept(*this);
        }
    }

void HIRGenerator::appendPatternParameterTypes(
    Pattern* pattern, std::vector<HIRTypePtr>& types) {
    if (auto* assignment = dynamic_cast<AssignmentPattern*>(pattern)) {
        appendPatternParameterTypes(assignment->left.get(), types);
        return;
    }
    if (auto* rest = dynamic_cast<RestElement*>(pattern)) {
        appendPatternParameterTypes(rest->argument.get(), types);
        return;
    }
    if (auto* object = dynamic_cast<ObjectPattern*>(pattern)) {
        for (auto& property : object->properties) {
            appendPatternParameterTypes(property.value.get(), types);
        }
        if (object->rest) {
            types.push_back(std::make_shared<HIRType>(HIRType::Kind::Pointer));
        }
        return;
    }
    types.push_back(std::make_shared<HIRType>(HIRType::Kind::JSValue));
}

void HIRGenerator::bindPatternParameters(
    Pattern* pattern, const std::vector<HIRParameter*>& parameters,
    size_t& cursor, Expr* defaultValue) {
    if (!pattern) return;
    if (auto* assignment = dynamic_cast<AssignmentPattern*>(pattern)) {
        bindPatternParameters(
            assignment->left.get(), parameters, cursor,
            assignment->right.get());
        return;
    }
    if (auto* object = dynamic_cast<ObjectPattern*>(pattern)) {
        for (auto& property : object->properties) {
            bindPatternParameters(
                property.value.get(), parameters, cursor,
                property.defaultValue.get());
        }
        if (object->rest && cursor < parameters.size()) {
            bindDestructuringPattern(
                object->rest.get(), parameters[cursor++]);
            if (auto* identifier = dynamic_cast<IdentifierPattern*>(
                    object->rest.get())) {
                dynamicObjectVars_.insert(identifier->name);
            }
        }
        return;
    }
    if (cursor >= parameters.size()) return;
    bindDestructuringPattern(pattern, parameters[cursor++], defaultValue);
}

void HIRGenerator::appendPatternArguments(
    Pattern* pattern, HIRValue* value, std::vector<HIRValue*>& arguments) {
    if (!pattern) {
        arguments.push_back(value);
        return;
    }
    if (auto* assignment = dynamic_cast<AssignmentPattern*>(pattern)) {
        appendPatternArguments(assignment->left.get(), value, arguments);
        return;
    }
    if (auto* object = dynamic_cast<ObjectPattern*>(pattern)) {
        HIRType* sourceCandidate = value && value->type
            ? value->type.get() : nullptr;
        if (auto* sourcePointer = dynamic_cast<HIRPointerType*>(sourceCandidate);
            sourcePointer && sourcePointer->pointeeType) {
            sourceCandidate = sourcePointer->pointeeType.get();
        }
        auto* sourceStruct = dynamic_cast<HIRStructType*>(sourceCandidate);
        for (auto& property : object->properties) {
            HIRValue* propertyValue = nullptr;
            if (sourceStruct) {
                for (size_t index = 0; index < sourceStruct->fields.size(); ++index) {
                    if (sourceStruct->fields[index].name == property.key) {
                        propertyValue = builder_->createGetField(
                            value, static_cast<uint32_t>(index), property.key);
                        break;
                    }
                }
            }
            if (!propertyValue) {
                auto unknownType = std::make_shared<HIRType>(HIRType::Kind::Unknown);
                propertyValue = builder_->createUndefinedConstant(
                    unknownType.get());
            }
            appendPatternArguments(
                property.value.get(), propertyValue, arguments);
        }
        if (object->rest) {
            auto pointerType = std::make_shared<HIRType>(HIRType::Kind::Pointer);
            auto voidType = std::make_shared<HIRType>(HIRType::Kind::Void);
            auto stringType = std::make_shared<HIRType>(HIRType::Kind::String);
            auto jsValueType = std::make_shared<HIRType>(HIRType::Kind::JSValue);

            auto createExisting = module_->getFunction(
                "nova_dynamic_object_create");
            HIRFunction* createFunction = createExisting
                ? createExisting.get() : nullptr;
            if (!createFunction) {
                auto* type = new HIRFunctionType({}, pointerType);
                auto created = module_->createFunction(
                    "nova_dynamic_object_create", type);
                created->linkage = HIRFunction::Linkage::External;
                createFunction = created.get();
            }
            auto* restObject = builder_->createCall(
                createFunction, {}, "pattern.object.rest");

            auto setExisting = module_->getFunction(
                "nova_dynamic_object_set_tagged");
            HIRFunction* setFunction = setExisting
                ? setExisting.get() : nullptr;
            if (!setFunction) {
                auto* type = new HIRFunctionType(
                    {pointerType, stringType, jsValueType}, voidType);
                auto created = module_->createFunction(
                    "nova_dynamic_object_set_tagged", type);
                created->linkage = HIRFunction::Linkage::External;
                setFunction = created.get();
            }

            std::unordered_set<std::string> excluded;
            for (auto& property : object->properties) {
                excluded.insert(property.key);
            }
            if (sourceStruct) {
                for (size_t index = 0; index < sourceStruct->fields.size(); ++index) {
                    const auto& field = sourceStruct->fields[index];
                    if (excluded.count(field.name)) continue;
                    auto* fieldValue = builder_->createGetField(
                        value, static_cast<uint32_t>(index), field.name);
                    builder_->createCall(setFunction, {
                        restObject,
                        builder_->createStringConstant(field.name),
                        toJSValue(fieldValue)
                    });
                }
            }
            arguments.push_back(restObject);
        }
        return;
    }
    arguments.push_back(toJSValue(value));
}

HIRValue* HIRGenerator::applyDestructuringDefault(
    HIRValue* value, Expr* defaultValue) {
    if (!defaultValue) return value;

    if (auto* constant = dynamic_cast<HIRConstant*>(value);
        constant && constant->kind == HIRConstant::Kind::Undefined) {
        defaultValue->accept(*this);
        return lastValue_;
    }
    if (!value || !value->type ||
        value->type->kind != HIRType::Kind::JSValue) {
        return value;
    }

    auto intType = std::make_shared<HIRType>(HIRType::Kind::I64);
    auto existing = module_->getFunction("nova_value_is_undefined");
    HIRFunction* isUndefined = existing ? existing.get() : nullptr;
    if (!isUndefined) {
        auto* functionType = new HIRFunctionType({value->type}, intType);
        auto created = module_->createFunction(
            "nova_value_is_undefined", functionType);
        created->linkage = HIRFunction::Linkage::External;
        isUndefined = created.get();
    }

    auto* result = builder_->createAlloca(
        value->type.get(), "destructure.default.result");
    builder_->createStore(value, result);
    auto* evaluateDefault = currentFunction_->createBasicBlock(
        "destructure.default").get();
    auto* merge = currentFunction_->createBasicBlock(
        "destructure.default.merge").get();
    auto* condition = builder_->createCall(
        isUndefined, {value}, "destructure.is_undefined");
    builder_->createCondBr(condition, evaluateDefault, merge);

    builder_->setInsertPoint(evaluateDefault);
    defaultValue->accept(*this);
    builder_->createStore(toJSValue(lastValue_), result);
    builder_->createBr(merge);

    builder_->setInsertPoint(merge);
    return builder_->createLoad(result, "destructure.default.value");
}

void HIRGenerator::bindDestructuringPattern(
    Pattern* pattern, HIRValue* value, Expr* defaultValue) {
    if (!pattern) return;

    if (auto* assignment = dynamic_cast<AssignmentPattern*>(pattern)) {
        bindDestructuringPattern(
            assignment->left.get(), value, assignment->right.get());
        return;
    }

    value = applyDestructuringDefault(value, defaultValue);
    if (!value) {
        auto unknownType = std::make_shared<HIRType>(HIRType::Kind::Unknown);
        value = builder_->createUndefinedConstant(unknownType.get());
    }

    if (auto* identifier = dynamic_cast<IdentifierPattern*>(pattern)) {
        HIRValue* storedValue = value;
        if (value->type && value->type->kind == HIRType::Kind::Unknown) {
            storedValue = toJSValue(value);
        }
        auto* storage = builder_->createAlloca(
            storedValue->type.get(), identifier->name);
        builder_->createStore(storedValue, storage);
        symbolTable_[identifier->name] = storage;
        if (storedValue->type->kind == HIRType::Kind::JSValue) {
            dynamicBindingNames_.insert(identifier->name);
        }
        return;
    }

    if (auto* rest = dynamic_cast<RestElement*>(pattern)) {
        bindDestructuringPattern(rest->argument.get(), value);
        return;
    }

    if (auto* arrayPattern = dynamic_cast<ArrayPattern*>(pattern)) {
        HIRValue* arrayValue = value;
        HIRArrayType* arrayType = nullptr;
        if (value->type) {
            HIRType* candidate = value->type.get();
            if (auto* pointer = dynamic_cast<HIRPointerType*>(candidate);
                pointer && pointer->pointeeType) {
                candidate = pointer->pointeeType.get();
            }
            arrayType = dynamic_cast<HIRArrayType*>(candidate);
        }

        if (value->type && value->type->kind == HIRType::Kind::JSValue) {
            auto pointerType = std::make_shared<HIRType>(HIRType::Kind::Pointer);
            auto existing = module_->getFunction("nova_value_to_object");
            HIRFunction* toObject = existing ? existing.get() : nullptr;
            if (!toObject) {
                auto* functionType = new HIRFunctionType(
                    {value->type}, pointerType);
                auto created = module_->createFunction(
                    "nova_value_to_object", functionType);
                created->linkage = HIRFunction::Linkage::External;
                toObject = created.get();
            }
            arrayValue = builder_->createCall(
                toObject, {value}, "destructure.array.object");
        }

        auto pointerType = std::make_shared<HIRType>(HIRType::Kind::Pointer);
        auto intType = std::make_shared<HIRType>(HIRType::Kind::I64);
        auto jsValueType = std::make_shared<HIRType>(HIRType::Kind::JSValue);

        for (size_t index = 0; index < arrayPattern->elements.size(); ++index) {
            auto& element = arrayPattern->elements[index];
            if (!element) continue;

            HIRValue* elementValue = nullptr;
            if (arrayType && arrayType->size > index) {
                elementValue = builder_->createGetElement(
                    arrayValue,
                    builder_->createIntConstant(static_cast<int64_t>(index)),
                    "destructure.element");
            } else {
                auto existing = module_->getFunction("nova_value_array_at_tagged");
                HIRFunction* atTagged = existing ? existing.get() : nullptr;
                if (!atTagged) {
                    auto* functionType = new HIRFunctionType(
                        {pointerType, intType, intType}, jsValueType);
                    auto created = module_->createFunction(
                        "nova_value_array_at_tagged", functionType);
                    created->linkage = HIRFunction::Linkage::External;
                    atTagged = created.get();
                }

                int64_t elementKind = 0;
                if (arrayType && arrayType->elementType) {
                    switch (arrayType->elementType->kind) {
                        case HIRType::Kind::F64: elementKind = 1; break;
                        case HIRType::Kind::String: elementKind = 2; break;
                        case HIRType::Kind::Bool: elementKind = 3; break;
                        case HIRType::Kind::Pointer:
                        case HIRType::Kind::Array:
                        case HIRType::Kind::Struct: elementKind = 4; break;
                        case HIRType::Kind::JSValue: elementKind = 5; break;
                        default: break;
                    }
                }
                elementValue = builder_->createCall(atTagged, {
                    arrayValue,
                    builder_->createIntConstant(static_cast<int64_t>(index)),
                    builder_->createIntConstant(elementKind)
                }, "destructure.element");
                elementValue->type = jsValueType;
            }
            bindDestructuringPattern(element.get(), elementValue);
        }

        if (arrayPattern->rest) {
            auto lengthExisting = module_->getFunction("nova_value_array_length");
            HIRFunction* lengthFunction = lengthExisting
                ? lengthExisting.get() : nullptr;
            if (!lengthFunction) {
                auto* functionType = new HIRFunctionType({pointerType}, intType);
                auto created = module_->createFunction(
                    "nova_value_array_length", functionType);
                created->linkage = HIRFunction::Linkage::External;
                lengthFunction = created.get();
            }
            auto* length = builder_->createCall(
                lengthFunction, {arrayValue}, "destructure.rest.length");

            HIRTypePtr restElementType = jsValueType;
            if (arrayType && arrayType->elementType) {
                restElementType = arrayType->elementType;
            }
            auto dynamicArray = std::make_shared<HIRArrayType>(
                restElementType, 0);
            HIRTypePtr restType = std::make_shared<HIRPointerType>(
                dynamicArray, true);
            auto sliceExisting = module_->getFunction("nova_value_array_slice");
            HIRFunction* sliceFunction = sliceExisting
                ? sliceExisting.get() : nullptr;
            if (!sliceFunction) {
                auto* functionType = new HIRFunctionType(
                    {pointerType, intType, intType}, restType);
                auto created = module_->createFunction(
                    "nova_value_array_slice", functionType);
                created->linkage = HIRFunction::Linkage::External;
                sliceFunction = created.get();
            }
            auto* restValue = builder_->createCall(sliceFunction, {
                arrayValue,
                builder_->createIntConstant(
                    static_cast<int64_t>(arrayPattern->elements.size())),
                length
            }, "destructure.rest");
            restValue->type = restType;
            bindDestructuringPattern(arrayPattern->rest.get(), restValue);
        }
        return;
    }

    if (auto* objectPattern = dynamic_cast<ObjectPattern*>(pattern)) {
        HIRStructType* structType = nullptr;
        if (value->type) {
            HIRType* candidate = value->type.get();
            if (auto* pointer = dynamic_cast<HIRPointerType*>(candidate);
                pointer && pointer->pointeeType) {
                candidate = pointer->pointeeType.get();
            }
            structType = dynamic_cast<HIRStructType*>(candidate);
        }

        std::unordered_set<std::string> excluded;
        for (auto& property : objectPattern->properties) {
            excluded.insert(property.key);
            HIRValue* propertyValue = nullptr;
            if (structType) {
                for (size_t index = 0; index < structType->fields.size(); ++index) {
                    if (structType->fields[index].name == property.key) {
                        propertyValue = builder_->createGetField(
                            value, static_cast<uint32_t>(index), property.key);
                        break;
                    }
                }
            }
            if (!propertyValue) {
                auto unknownType = std::make_shared<HIRType>(HIRType::Kind::Unknown);
                propertyValue = builder_->createUndefinedConstant(unknownType.get());
            }
            bindDestructuringPattern(
                property.value.get(), propertyValue, property.defaultValue.get());
        }

        if (objectPattern->rest) {
            static uint64_t restObjectCounter = 0;
            auto* restStruct = module_->createStructType(
                "__destructure_rest_" + std::to_string(restObjectCounter++));
            std::vector<HIRValue*> restValues;
            if (structType) {
                for (size_t index = 0; index < structType->fields.size(); ++index) {
                    if (excluded.count(structType->fields[index].name)) continue;
                    restStruct->fields.push_back(structType->fields[index]);
                    restValues.push_back(builder_->createGetField(
                        value, static_cast<uint32_t>(index),
                        structType->fields[index].name));
                }
            }
            auto* restValue = builder_->createStructConstruct(
                restStruct, restValues, "destructure.object.rest");
            bindDestructuringPattern(objectPattern->rest.get(), restValue);
        }
    }
}

void HIRGenerator::assignDestructuringPattern(
    Pattern* pattern, HIRValue* value) {
    std::vector<std::string> names;
    std::function<void(Pattern*)> collect = [&](Pattern* current) {
        if (!current) return;
        if (auto* identifier = dynamic_cast<IdentifierPattern*>(current)) {
            names.push_back(identifier->name);
        } else if (auto* assignment = dynamic_cast<AssignmentPattern*>(current)) {
            collect(assignment->left.get());
        } else if (auto* rest = dynamic_cast<RestElement*>(current)) {
            collect(rest->argument.get());
        } else if (auto* array = dynamic_cast<ArrayPattern*>(current)) {
            for (auto& element : array->elements) collect(element.get());
            collect(array->rest.get());
        } else if (auto* object = dynamic_cast<ObjectPattern*>(current)) {
            for (auto& property : object->properties) {
                collect(property.value.get());
            }
            collect(object->rest.get());
        }
    };
    collect(pattern);

    std::unordered_map<std::string, HIRValue*> previous;
    for (const auto& name : names) {
        previous[name] = lookupVariable(name);
    }

    bindDestructuringPattern(pattern, value);
    for (const auto& name : names) {
        auto generated = symbolTable_.find(name);
        auto old = previous.find(name);
        if (generated == symbolTable_.end() || old == previous.end() ||
            !old->second) continue;
        HIRValue* assignedValue = builder_->createLoad(
            generated->second, name + ".destructure.assignment");
        if (auto* storageType = dynamic_cast<HIRPointerType*>(
                old->second->type.get());
            storageType && storageType->pointeeType &&
            storageType->pointeeType->kind == HIRType::Kind::JSValue &&
            assignedValue->type->kind != HIRType::Kind::JSValue) {
            assignedValue = toJSValue(assignedValue);
        } else if (storageType && storageType->pointeeType &&
                   storageType->pointeeType->kind == HIRType::Kind::Pointer &&
                   assignedValue->type->kind == HIRType::Kind::Pointer) {
            // Opaque LLVM pointers have a stable physical ABI, but HIR must
            // retain the newly assigned array/object pointee metadata so later
            // element/property reads decode tagged slots correctly.
            storageType->pointeeType = assignedValue->type;
        }
        builder_->createStore(assignedValue, old->second);
        symbolTable_[name] = old->second;
    }
}
    
void HIRGenerator::visit(VarDeclStmt& node) {
        if(NOVA_DEBUG) {
            printf("  ENTER VARDECL: kind=%d count=%zu\n", (int)node.kind, node.declarations.size());
            fflush(stdout);
        }
        for (auto& decl : node.declarations) {
            lastIntegrityObjectName_.clear();

            if(NOVA_DEBUG) {
                std::cerr << "  VARDECL INNER: name='" << decl.name << "' init=" << (decl.init ? "yes" : "no")
                          << " lastWasSet_=" << lastWasSet_
                          << " lastWasMap_=" << lastWasMap_
                          << " lastWasWeakMap_=" << lastWasWeakMap_
                          << " lastWasWeakSet_=" << lastWasWeakSet_
                          << std::endl;
            }

            // Evaluate the initializer first to get its type
            HIRValue* initValue = nullptr;
            if (decl.init) {
                decl.init->accept(*this);
                initValue = lastValue_;
            }

            if(NOVA_DEBUG) {
                std::cerr << "  VARDECL AFTER INIT: lastWasSet_=" << lastWasSet_ << std::endl;
            }

            if (!decl.name.empty()) {
                auto* constant = dynamic_cast<HIRConstant*>(initValue);
                if (constant &&
                    (constant->kind == HIRConstant::Kind::Null ||
                     constant->kind == HIRConstant::Kind::Undefined)) {
                    staticNullishVariables_[decl.name] = constant->kind;
                } else {
                    staticNullishVariables_.erase(decl.name);
                }
            }

            // Preserve object integrity state through simple aliases and through
            // Object.freeze/seal/preventExtensions return values.
            std::string integritySource = lastIntegrityObjectName_;
            if (integritySource.empty()) {
                if (auto* sourceIdentifier = dynamic_cast<Identifier*>(decl.init.get())) {
                    integritySource = sourceIdentifier->name;
                }
            }
            if (!integritySource.empty()) {
                if (frozenObjectVars_.count(integritySource) > 0) {
                    frozenObjectVars_.insert(decl.name);
                }
                if (sealedObjectVars_.count(integritySource) > 0) {
                    sealedObjectVars_.insert(decl.name);
                }
                if (nonExtensibleObjectVars_.count(integritySource) > 0) {
                    nonExtensibleObjectVars_.insert(decl.name);
                }
                if (propertyWritable_.count(integritySource) > 0) {
                    propertyWritable_[decl.name] = propertyWritable_[integritySource];
                }
                if (propertyEnumerable_.count(integritySource) > 0) {
                    propertyEnumerable_[decl.name] = propertyEnumerable_[integritySource];
                }
                if (propertyConfigurable_.count(integritySource) > 0) {
                    propertyConfigurable_[decl.name] = propertyConfigurable_[integritySource];
                }
            }
            lastIntegrityObjectName_.clear();

            // Check if this is a destructuring pattern
            if (decl.pattern) {
                bindDestructuringPattern(decl.pattern.get(), initValue);
                continue;  // Don't process as normal variable
            }

            // A closure expression assigned to a local variable needs its
            // environment immediately; waiting until a return statement leaves
            // the variable holding only the generated function-name string.
            const bool directClosureExpression =
                dynamic_cast<FunctionExpr*>(decl.init.get()) != nullptr ||
                dynamic_cast<ArrowFunctionExpr*>(decl.init.get()) != nullptr;
            if (directClosureExpression && !lastFunctionName_.empty() && currentFunction_ &&
                lastFunctionName_ != currentFunction_->name &&
                module_->closureEnvironments.count(lastFunctionName_) != 0) {
                if (auto* environment = materializeClosureEnvironment(
                        lastFunctionName_)) {
                    initValue = environment;
                }
            }

            // Check if this is a function reference assignment
            if(NOVA_DEBUG) {
                std::cerr << "DEBUG HIRGen: Checking function reference for '" << decl.name
                          << "', lastFunctionName_ = '" << lastFunctionName_ << "'" << std::endl;
            }
            // lastFunctionName_ also tracks the currently generated containing
            // function. Only a different name denotes a newly produced
            // function/closure value for this initializer.
            // Prefer the initializer's semantic relationship over the mutable
            // lastFunctionName_ cursor.  Nested function generation and callee
            // visitation can restore that cursor before the declaration is
            // bound, while closureReturnedBy is stable module metadata.
            if (auto* call = dynamic_cast<CallExpr*>(decl.init.get())) {
                if (auto* callee = dynamic_cast<Identifier*>(call->callee.get())) {
                    auto returnedClosure =
                        module_->closureReturnedBy.find(callee->name);
                    if (returnedClosure != module_->closureReturnedBy.end()) {
                        functionReferences_[decl.name] = returnedClosure->second;
                        lastFunctionName_.clear();
                    }
                }
            }
            if (!pendingBoundFunction_.empty()) {
                functionReferences_[decl.name] = pendingBoundFunction_;
                boundFunctionArguments_[decl.name] = pendingBoundArguments_;
                if (pendingBoundThis_) {
                    boundFunctionThis_[decl.name] = pendingBoundThis_;
                }
                pendingBoundFunction_.clear();
                pendingBoundArguments_.clear();
                pendingBoundThis_ = nullptr;
                lastFunctionName_.clear();
            }

            if (functionReferences_.count(decl.name) == 0 &&
                !lastFunctionName_.empty() &&
                (!currentFunction_ ||
                 lastFunctionName_ != currentFunction_->name)) {
                // Register this variable as holding a function reference
                functionReferences_[decl.name] = lastFunctionName_;
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Registered function reference: " << decl.name
                          << " -> " << lastFunctionName_ << std::endl;
                lastFunctionName_.clear();  // Clear for next declaration
            }

            // Check if this is a class expression assignment
            if (!lastClassName_.empty()) {
                // Register this variable as holding a class reference
                classReferences_[decl.name] = lastClassName_;
                classNames_.insert(decl.name);  // Register the variable name as a class name
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Registered class reference: " << decl.name
                          << " -> " << lastClassName_ << std::endl;
                lastClassName_.clear();  // Clear for next declaration
            }

            // Check if this is an object with methods
            if (!currentObjectName_.empty()) {
                // Transfer method mappings from object ID to variable name
                if (objectMethodFunctions_.find(currentObjectName_) != objectMethodFunctions_.end()) {
                    objectMethodFunctions_[decl.name] = objectMethodFunctions_[currentObjectName_];
                    objectMethodProperties_[decl.name] = objectMethodProperties_[currentObjectName_];

                    if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Associated object methods with variable '"
                                              << decl.name << "'" << std::endl;
                }
                // Transfer field names for for-in loop support
                if (objectFieldNames_.find(currentObjectName_) != objectFieldNames_.end()) {
                    objectFieldNames_[decl.name] = objectFieldNames_[currentObjectName_];
                }
                currentObjectName_.clear();  // Clear for next declaration
            }

            // Check if this is a TypedArray assignment
            if (!lastTypedArrayType_.empty()) {
                typedArrayTypes_[decl.name] = lastTypedArrayType_;
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Registered TypedArray type: " << decl.name
                          << " -> " << lastTypedArrayType_ << std::endl;
                lastTypedArrayType_.clear();  // Clear for next declaration
            }

            // Track variable kind for instanceof resolution
            if (!lastVariableKind_.empty()) {
                variableKinds_[decl.name] = lastVariableKind_;
                lastVariableKind_.clear();
            }

            // Track typed-array element types from explicit annotations
            // (e.g. `let arr: string[] = ...`) so arr[i] can return a String.
            if (decl.type && decl.type->kind == Type::Kind::Array &&
                decl.type->elementType) {
                switch (decl.type->elementType->kind) {
                    case Type::Kind::String:
                        variableArrayElementTypes_[decl.name] = "String"; break;
                    case Type::Kind::Number:
                        variableArrayElementTypes_[decl.name] = "Number"; break;
                    case Type::Kind::Boolean:
                        variableArrayElementTypes_[decl.name] = "Bool"; break;
                    default: break;
                }
            }

            // Check if this is an ArrayBuffer assignment
            if (lastWasArrayBuffer_) {
                arrayBufferVars_.insert(decl.name);
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Registered ArrayBuffer variable: " << decl.name << std::endl;
                lastWasArrayBuffer_ = false;  // Clear for next declaration
            }

            // Check if this is a SharedArrayBuffer assignment (ES2017)
            if (lastWasSharedArrayBuffer_) {
                sharedArrayBufferVars_.insert(decl.name);
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Registered SharedArrayBuffer variable: " << decl.name << std::endl;
                lastWasSharedArrayBuffer_ = false;  // Clear for next declaration
            }

            // Check if this is a BigInt assignment (ES2020)
            if (lastWasBigInt_) {
                bigIntVars_.insert(decl.name);
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Registered BigInt variable: " << decl.name << std::endl;
                lastWasBigInt_ = false;  // Clear for next declaration
            }

            // Check if this is a DataView assignment
            if (lastWasDataView_) {
                dataViewVars_.insert(decl.name);
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Registered DataView variable: " << decl.name << std::endl;
                lastWasDataView_ = false;  // Clear for next declaration
            }

            // Check if this is a Date assignment
            if (lastWasDate_) {
                dateVars_.insert(decl.name);
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Registered Date variable: " << decl.name << std::endl;
                lastWasDate_ = false;  // Clear for next declaration
            }

            // Check if this is a DisposableStack assignment
            if (lastWasDisposableStack_) {
                disposableStackVars_.insert(decl.name);
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Registered DisposableStack variable: " << decl.name << std::endl;
                lastWasDisposableStack_ = false;  // Clear for next declaration
            }

            // Check if this is an AsyncDisposableStack assignment
            if (lastWasAsyncDisposableStack_) {
                asyncDisposableStackVars_.insert(decl.name);
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Registered AsyncDisposableStack variable: " << decl.name << std::endl;
                lastWasAsyncDisposableStack_ = false;  // Clear for next declaration
            }

            // Check if this is a FinalizationRegistry assignment
            if (lastWasFinalizationRegistry_) {
                finalizationRegistryVars_.insert(decl.name);
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Registered FinalizationRegistry variable: " << decl.name << std::endl;
                lastWasFinalizationRegistry_ = false;  // Clear for next declaration
            }

            // Check if this is a Promise assignment
            if (lastWasPromise_) {
                promiseVars_.insert(decl.name);
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Registered Promise variable: " << decl.name << std::endl;
                lastWasPromise_ = false;  // Clear for next declaration
            }

            // Check if this is a Generator assignment
            if (lastWasGenerator_) {
                generatorVars_.insert(decl.name);
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Registered Generator variable: " << decl.name << std::endl;
                lastWasGenerator_ = false;  // Clear for next declaration
            }

            // Check if this is an Error assignment
            if (lastWasError_) {
                errorVars_.insert(decl.name);
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Registered Error variable: " << decl.name << std::endl;
                lastWasError_ = false;  // Clear for next declaration
            }

            // Check if this is a SuppressedError assignment
            if (lastWasSuppressedError_) {
                suppressedErrorVars_.insert(decl.name);
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Registered SuppressedError variable: " << decl.name << std::endl;
                lastWasSuppressedError_ = false;  // Clear for next declaration
            }

            // Check if this is a Symbol assignment
            if (lastWasSymbol_) {
                symbolVars_.insert(decl.name);
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Registered Symbol variable: " << decl.name << std::endl;
                lastWasSymbol_ = false;  // Clear for next declaration
            }

            // Check if this is an AsyncGenerator assignment
            if (lastWasAsyncGenerator_) {
                asyncGeneratorVars_.insert(decl.name);
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Registered AsyncGenerator variable: " << decl.name << std::endl;
                lastWasAsyncGenerator_ = false;  // Clear for next declaration
            }

            // Check if this is an IteratorResult assignment (from gen.next())
            if (lastWasIteratorResult_) {
                iteratorResultVars_.insert(decl.name);
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Registered IteratorResult variable: " << decl.name << std::endl;
                lastWasIteratorResult_ = false;  // Clear for next declaration
            }

            // Check if this is a runtime array assignment (from keys(), values(), entries())
            if (lastWasRuntimeArray_) {
                runtimeArrayVars_.insert(decl.name);
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Registered runtime array variable: " << decl.name << std::endl;
                lastWasRuntimeArray_ = false;  // Clear for next declaration
            }

            // Check if this is an Intl.* assignment
            if (lastWasNumberFormat_) {
                numberFormatVars_.insert(decl.name);
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Registered NumberFormat variable: " << decl.name << std::endl;
                lastWasNumberFormat_ = false;
            }
            if (lastWasDateTimeFormat_) {
                dateTimeFormatVars_.insert(decl.name);
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Registered DateTimeFormat variable: " << decl.name << std::endl;
                lastWasDateTimeFormat_ = false;
            }
            if (lastWasCollator_) {
                collatorVars_.insert(decl.name);
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Registered Collator variable: " << decl.name << std::endl;
                lastWasCollator_ = false;
            }
            if (lastWasPluralRules_) {
                pluralRulesVars_.insert(decl.name);
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Registered PluralRules variable: " << decl.name << std::endl;
                lastWasPluralRules_ = false;
            }
            if (lastWasRelativeTimeFormat_) {
                relativeTimeFormatVars_.insert(decl.name);
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Registered RelativeTimeFormat variable: " << decl.name << std::endl;
                lastWasRelativeTimeFormat_ = false;
            }
            if (lastWasListFormat_) {
                listFormatVars_.insert(decl.name);
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Registered ListFormat variable: " << decl.name << std::endl;
                lastWasListFormat_ = false;
            }
            if (lastWasDisplayNames_) {
                displayNamesVars_.insert(decl.name);
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Registered DisplayNames variable: " << decl.name << std::endl;
                lastWasDisplayNames_ = false;
            }
            if (lastWasLocale_) {
                localeVars_.insert(decl.name);
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Registered Locale variable: " << decl.name << std::endl;
                lastWasLocale_ = false;
            }
            if (lastWasSegmenter_) {
                segmenterVars_.insert(decl.name);
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Registered Segmenter variable: " << decl.name << std::endl;
                lastWasSegmenter_ = false;
            }
            if (lastWasIterator_) {
                iteratorVars_.insert(decl.name);
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Registered Iterator variable: " << decl.name << std::endl;
                lastWasIterator_ = false;
            }
            if (lastWasMap_) {
                mapVars_.insert(decl.name);
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Registered Map variable: " << decl.name << std::endl;
                lastWasMap_ = false;
            }
            if (lastWasSet_) {
                setVars_.insert(decl.name);
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Registered Set variable: " << decl.name << std::endl;
                lastWasSet_ = false;
            }
            if (lastWasWeakMap_) {
                weakMapVars_.insert(decl.name);
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Registered WeakMap variable: " << decl.name << std::endl;
                lastWasWeakMap_ = false;
            }
            if (lastWasWeakRef_) {
                weakRefVars_.insert(decl.name);
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Registered WeakRef variable: " << decl.name << std::endl;
                lastWasWeakRef_ = false;
            }
            if (lastWasWeakSet_) {
                weakSetVars_.insert(decl.name);
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Registered WeakSet variable: " << decl.name << std::endl;
                lastWasWeakSet_ = false;
            }
            // Web API types tracking
            if (lastWasURL_) {
                urlVars_.insert(decl.name);
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Registered URL variable: " << decl.name << std::endl;
                lastWasURL_ = false;
            }
            if (lastWasURLSearchParams_) {
                urlSearchParamsVars_.insert(decl.name);
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Registered URLSearchParams variable: " << decl.name << std::endl;
                lastWasURLSearchParams_ = false;
            }
            if (lastWasTextEncoder_) {
                textEncoderVars_.insert(decl.name);
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Registered TextEncoder variable: " << decl.name << std::endl;
                lastWasTextEncoder_ = false;
            }
            if (lastWasTextDecoder_) {
                textDecoderVars_.insert(decl.name);
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Registered TextDecoder variable: " << decl.name << std::endl;
                lastWasTextDecoder_ = false;
            }
            if (lastWasHeaders_) {
                headersVars_.insert(decl.name);
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Registered Headers variable: " << decl.name << std::endl;
                lastWasHeaders_ = false;
            }
            if (lastWasRequest_) {
                requestVars_.insert(decl.name);
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Registered Request variable: " << decl.name << std::endl;
                lastWasRequest_ = false;
            }
            if (lastWasResponse_) {
                responseVars_.insert(decl.name);
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Registered Response variable: " << decl.name << std::endl;
                lastWasResponse_ = false;
            }

            // Check if this is a builtin object type assignment (stream, events, http, etc.)
            if (!lastBuiltinObjectType_.empty()) {
                variableObjectTypes_[decl.name] = lastBuiltinObjectType_;
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Registered builtin object type: " << decl.name
                          << " -> " << lastBuiltinObjectType_ << std::endl;
                lastBuiltinObjectType_.clear();  // Clear for next declaration
            }

            // Inside generators, use generator local storage for variables that may cross yield boundaries
            if (currentGeneratorPtr_ && generatorStoreLocalFunc_) {
                // Assign a slot index for this variable
                int slotIndex = generatorNextLocalSlot_++;
                generatorVarSlots_[decl.name] = slotIndex;
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Generator variable '" << decl.name << "' assigned to slot " << slotIndex << std::endl;

                // Store initial value to generator local slot
                if (initValue) {
                    auto* genPtr = builder_->createLoad(currentGeneratorPtr_);
                    auto* slotConst = builder_->createIntConstant(slotIndex);
                    std::vector<HIRValue*> storeArgs = {genPtr, slotConst, initValue};
                    builder_->createCall(generatorStoreLocalFunc_, storeArgs);
                }

                // Also create a normal alloca for within-block access (optimization)
                auto i64Type = std::make_shared<HIRType>(HIRType::Kind::I64);
                auto alloca = builder_->createAlloca(i64Type.get(), decl.name);
                symbolTable_[decl.name] = alloca;
                if (initValue) {
                    builder_->createStore(initValue, alloca);
                }
            } else {
                // Normal (non-generator) variable handling
                if (initValue && dynamicBindingNames_.count(decl.name) > 0 &&
                    bigIntVars_.count(decl.name) == 0) {
                    initValue = toJSValue(initValue);
                    staticNullishVariables_.erase(decl.name);
                }
                // A JavaScript declaration without an initializer evaluates to
                // undefined. Use a real tagged value instead of an uninitialized
                // i64 slot (whose pointee type also used to dangle here).
                if (!initValue) {
                    auto jsType = std::make_shared<HIRType>(HIRType::Kind::JSValue);
                    initValue = new HIRConstant(
                        jsType, HIRConstant::Kind::Integer,
                        static_cast<int64_t>(0x7ff9000000000000ULL));
                }
                HIRType* allocaType = initValue->type.get();

                // Allocate storage with the correct type
                auto alloca = builder_->createAlloca(allocaType, decl.name);
                symbolTable_[decl.name] = alloca;

                // Store the initializer value if present
                builder_->createStore(initValue, alloca);
            }
        }
    }
    
void HIRGenerator::visit(DeclStmt& node) {
        // Process the declaration within this statement
        if (node.declaration) {
            node.declaration->accept(*this);
        }
    }

void HIRGenerator::visit(LabeledStmt& node) {
        // Labeled statement - store label for potential break/continue targets
        if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Processing labeled statement: " << node.label << std::endl;

        // Track the label for potential labeled break/continue
        // The label applies to the next statement (usually a loop)
        std::string savedLabel = currentLabel_;
        currentLabel_ = node.label;

        if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: About to visit labeled statement body" << std::endl;
        if (node.statement) {
            node.statement->accept(*this);
        } else {
            if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: WARNING - labeled statement has null body" << std::endl;
        }

        currentLabel_ = savedLabel;
        if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Exiting labeled statement: " << node.label << std::endl;
    }

void HIRGenerator::visit(WithStmt& node) {
        // 'with' statement is deprecated in JavaScript and forbidden in strict mode
        if (NOVA_DEBUG) std::cerr << "WARNING: 'with' statement is deprecated and not recommended" << std::endl;

        // Still evaluate the object expression (may have side effects)
        if (node.object) {
            node.object->accept(*this);
        }

        // Execute the body
        if (node.body) {
            node.body->accept(*this);
        }
    }
    
void HIRGenerator::visit(DebuggerStmt& node) {
        (void)node;
        // debugger statement - no-op in HIR
    }
    
void HIRGenerator::visit(EmptyStmt& node) {
        (void)node;
        // empty statement - no-op
    }

void HIRGenerator::visit(UsingStmt& node) {
        // ES2024 'using' statement for Explicit Resource Management
        // Creates a const binding that will be disposed when scope exits
        // For now, we implement it as a const binding - full dispose support needs runtime

        std::string name = node.name;

        // Evaluate the initializer first to get its type
        HIRValue* initValue = nullptr;
        if (node.init) {
            node.init->accept(*this);
            initValue = lastValue_;
        }

        // Use the initializer's type for the alloca, or default to Any
        HIRType* allocaType = nullptr;
        if (initValue && initValue->type) {
            allocaType = initValue->type.get();
        } else {
            auto anyType = std::make_shared<HIRType>(HIRType::Kind::Any);
            allocaType = anyType.get();
        }

        // Allocate storage with the correct type
        auto alloca = builder_->createAlloca(allocaType, name);
        symbolTable_[name] = alloca;

        // Store the initializer value if present
        if (initValue) {
            builder_->createStore(initValue, alloca);
        }

        // Note: Full implementation would track this for disposal at scope exit
        // This would require block-level resource tracking for [Symbol.dispose]()
        // For now, the resource is created but disposal must be done manually

        if (node.isAwait) {
            // await using - would use [Symbol.asyncDispose]() at scope exit
            // This requires async context and Promise handling
            (void)node.isAwait;  // Silence unused warning
        }
    }

} // namespace nova::hir
