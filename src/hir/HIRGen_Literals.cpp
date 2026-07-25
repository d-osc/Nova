// HIRGen_Literals.cpp - Literal expression visitors
// Extracted from HIRGen.cpp for better code organization

#include "nova/HIR/HIRGen_Internal.h"
#define NOVA_DEBUG 0

namespace nova::hir {

// Forward declaration - this will be part of the HIRGenerator class
// These methods are separated into this file for better organization

// NumberLiteral - handles numeric constants (integers and floats)
void HIRGenerator::visit(NumberLiteral& node) {
    // Create numeric constant
    // Preserve explicitly decimal/exponent literals as floating point even
    // when their mathematical value is integral. This is required for signed
    // zero and prevents `1.0` from silently becoming an integer HIR value.
    const bool prefixedInteger = node.raw.size() > 2 && node.raw[0] == '0' &&
        (node.raw[1] == 'x' || node.raw[1] == 'X' ||
         node.raw[1] == 'b' || node.raw[1] == 'B' ||
         node.raw[1] == 'o' || node.raw[1] == 'O');
    const bool explicitFloat = !prefixedInteger &&
        node.raw.find_first_of(".eE") != std::string::npos;
    if (!explicitFloat && node.value == static_cast<int64_t>(node.value)) {
        lastValue_ = builder_->createIntConstant(static_cast<int64_t>(node.value));
    } else {
        lastValue_ = builder_->createFloatConstant(node.value);
    }
}

// BigIntLiteral - handles BigInt literals (ES2020)
void HIRGenerator::visit(BigIntLiteral& node) {
    // Create BigInt from string literal (ES2020)
    if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: BigInt literal: " << node.value << "n" << std::endl;

    auto ptrType = std::make_shared<HIRType>(HIRType::Kind::Pointer);

    // Create string constant for the value
    HIRValue* strValue = builder_->createStringConstant(node.value);

    // Call runtime function to create BigInt from string
    std::string runtimeFuncName = "nova_bigint_create_from_string";
    std::vector<HIRTypePtr> paramTypes = {ptrType};

    HIRFunction* runtimeFunc = nullptr;
    auto existingFunc = module_->getFunction(runtimeFuncName);
    if (existingFunc) {
        runtimeFunc = existingFunc.get();
    } else {
        HIRFunctionType* funcType = new HIRFunctionType(paramTypes, ptrType);
        HIRFunctionPtr funcPtr = module_->createFunction(runtimeFuncName, funcType);
        funcPtr->linkage = HIRFunction::Linkage::External;
        runtimeFunc = funcPtr.get();
    }

    std::vector<HIRValue*> args = {strValue};
    lastValue_ = builder_->createCall(runtimeFunc, args, "bigint_literal");
    lastValue_->type = ptrType;
    lastWasBigInt_ = true;
}

// StringLiteral - handles string constants
void HIRGenerator::visit(StringLiteral& node) {
    lastValue_ = builder_->createStringConstant(node.value);
}

// RegexLiteralExpr - handles regular expression literals
void HIRGenerator::visit(RegexLiteralExpr& node) {
    // Create a call to nova_regex_create(pattern, flags) runtime function
    auto patternConst = builder_->createStringConstant(node.pattern);
    auto flagsConst = builder_->createStringConstant(node.flags);

    // Create call to runtime function to create regex object
    std::vector<HIRValue*> args = { patternConst, flagsConst };

    // Get or create the nova_regex_create function
    HIRFunction* regexCreateFunc = nullptr;
    auto& functions = module_->functions;
    for (auto& func : functions) {
        if (func->name == "nova_regex_create") {
            regexCreateFunc = func.get();
            break;
        }
    }
    if (!regexCreateFunc) {
        // Declare the function
        std::vector<HIRTypePtr> paramTypes;
        paramTypes.push_back(std::make_shared<HIRType>(HIRType::Kind::String));
        paramTypes.push_back(std::make_shared<HIRType>(HIRType::Kind::String));
        auto returnType = std::make_shared<HIRType>(HIRType::Kind::Any);  // Returns regex handle

        HIRFunctionType* funcType = new HIRFunctionType(paramTypes, returnType);
        HIRFunctionPtr funcPtr = module_->createFunction("nova_regex_create", funcType);
        funcPtr->linkage = HIRFunction::Linkage::External;
        regexCreateFunc = funcPtr.get();
    }

    lastValue_ = builder_->createCall(regexCreateFunc, args, "regex");
}

// BooleanLiteral - handles true/false constants
void HIRGenerator::visit(BooleanLiteral& node) {
    lastValue_ = builder_->createBoolConstant(node.value);
}

// NullLiteral - handles null constant
void HIRGenerator::visit(NullLiteral& node) {
    (void)node;
    auto nullType = std::make_shared<HIRType>(HIRType::Kind::Unknown);
    lastValue_ = builder_->createNullConstant(nullType.get());
}

// UndefinedLiteral - handles undefined constant
void HIRGenerator::visit(UndefinedLiteral& node) {
    (void)node;
    auto undefType = std::make_shared<HIRType>(HIRType::Kind::Unknown);
    lastValue_ = builder_->createUndefinedConstant(undefType.get());
}

} // namespace nova::hir
