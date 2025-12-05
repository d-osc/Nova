#include "nova/runtime/Runtime.h"
#include <cstring>
#include <cstdlib>
#include <string>

namespace nova {
namespace runtime {

Closure* create_closure(FunctionPtr function, void* environment) {
    if (!function) return nullptr;

    // Allocate closure structure
    Closure* closure = static_cast<Closure*>(allocate(sizeof(Closure), TypeId::CLOSURE));

    // Initialize closure
    closure->function = function;
    closure->environment = environment;

    return closure;
}

void* call_closure(Closure* closure, void** args, size_t arg_count) {
    if (!closure || !closure->function) return nullptr;

    return closure->function(closure->environment, args, arg_count);
}

} // namespace runtime
} // namespace nova

// Nova Function structure for tracking function metadata
struct NovaFunction {
    void* funcPtr;          // Function pointer
    char* name;             // Function name
    int64_t length;         // Number of parameters
    char* sourceCode;       // Source code (for toString)
    void* boundThis;        // Bound this value (for bind)
    void** boundArgs;       // Bound arguments (for bind)
    int64_t boundArgCount;  // Number of bound arguments
};

extern "C" {

// Create a function wrapper with metadata
void* nova_function_create(void* funcPtr, const char* name, int64_t length) {
    NovaFunction* func = static_cast<NovaFunction*>(malloc(sizeof(NovaFunction)));
    func->funcPtr = funcPtr;
    func->name = name ? strdup(name) : strdup("");
    func->length = length;
    func->sourceCode = nullptr;
    func->boundThis = nullptr;
    func->boundArgs = nullptr;
    func->boundArgCount = 0;
    return func;
}

// Function.prototype.name - get function name
void* nova_function_get_name(void* funcPtr) {
    if (!funcPtr) {
        char* empty = static_cast<char*>(malloc(1));
        empty[0] = '\0';
        return empty;
    }
    NovaFunction* func = static_cast<NovaFunction*>(funcPtr);
    return strdup(func->name ? func->name : "");
}

// Function.prototype.length - get parameter count
int64_t nova_function_get_length(void* funcPtr) {
    if (!funcPtr) return 0;
    NovaFunction* func = static_cast<NovaFunction*>(funcPtr);
    return func->length;
}

// Function.prototype.toString() - get function source
void* nova_function_toString(void* funcPtr) {
    if (!funcPtr) {
        return strdup("function () { [native code] }");
    }
    NovaFunction* func = static_cast<NovaFunction*>(funcPtr);

    // Build function string
    std::string result = "function ";
    if (func->name && func->name[0] != '\0') {
        result += func->name;
    }
    result += "() { [native code] }";

    return strdup(result.c_str());
}

// Function.prototype.call(thisArg, arg1, arg2, ...) - up to 8 args
int64_t nova_function_call(void* funcPtr, [[maybe_unused]] void* thisArg,
                           int64_t a0, int64_t a1, int64_t a2, int64_t a3,
                           int64_t a4, int64_t a5, int64_t a6, int64_t a7,
                           int64_t argCount) {
    if (!funcPtr) return 0;

    // For now, we support calling function pointers directly
    // thisArg is ignored in this simple implementation (no this binding in Nova yet)
    typedef int64_t (*FuncType0)();
    typedef int64_t (*FuncType1)(int64_t);
    typedef int64_t (*FuncType2)(int64_t, int64_t);
    typedef int64_t (*FuncType3)(int64_t, int64_t, int64_t);
    typedef int64_t (*FuncType4)(int64_t, int64_t, int64_t, int64_t);
    typedef int64_t (*FuncType5)(int64_t, int64_t, int64_t, int64_t, int64_t);
    typedef int64_t (*FuncType6)(int64_t, int64_t, int64_t, int64_t, int64_t, int64_t);
    typedef int64_t (*FuncType7)(int64_t, int64_t, int64_t, int64_t, int64_t, int64_t, int64_t);
    typedef int64_t (*FuncType8)(int64_t, int64_t, int64_t, int64_t, int64_t, int64_t, int64_t, int64_t);

    switch (argCount) {
        case 0: return ((FuncType0)funcPtr)();
        case 1: return ((FuncType1)funcPtr)(a0);
        case 2: return ((FuncType2)funcPtr)(a0, a1);
        case 3: return ((FuncType3)funcPtr)(a0, a1, a2);
        case 4: return ((FuncType4)funcPtr)(a0, a1, a2, a3);
        case 5: return ((FuncType5)funcPtr)(a0, a1, a2, a3, a4);
        case 6: return ((FuncType6)funcPtr)(a0, a1, a2, a3, a4, a5);
        case 7: return ((FuncType7)funcPtr)(a0, a1, a2, a3, a4, a5, a6);
        case 8: return ((FuncType8)funcPtr)(a0, a1, a2, a3, a4, a5, a6, a7);
        default: return ((FuncType0)funcPtr)();
    }
}

// Function.prototype.apply(thisArg, argsArray)
int64_t nova_function_apply(void* funcPtr, void* thisArg, void* argsArray) {
    if (!funcPtr) return 0;

    // Extract arguments from array
    int64_t args[8] = {0};
    int64_t argCount = 0;

    if (argsArray) {
        // Assume argsArray is a Nova array with length and elements
        struct ArrayMeta { char pad[24]; int64_t length; int64_t capacity; int64_t* elements; };
        ArrayMeta* arr = static_cast<ArrayMeta*>(argsArray);
        argCount = arr->length > 8 ? 8 : arr->length;
        for (int64_t i = 0; i < argCount; i++) {
            args[i] = arr->elements[i];
        }
    }

    return nova_function_call(funcPtr, thisArg,
                              args[0], args[1], args[2], args[3],
                              args[4], args[5], args[6], args[7],
                              argCount);
}

// Function.prototype.bind(thisArg, arg1, arg2, ...) - returns bound function
void* nova_function_bind(void* funcPtr, void* thisArg,
                         int64_t a0, int64_t a1, int64_t a2, int64_t a3,
                         int64_t a4, int64_t a5, int64_t a6, int64_t a7,
                         int64_t argCount) {
    if (!funcPtr) return nullptr;

    // Create a new function wrapper that stores bound values
    NovaFunction* boundFunc = static_cast<NovaFunction*>(malloc(sizeof(NovaFunction)));

    // Copy original function info
    NovaFunction* origFunc = static_cast<NovaFunction*>(funcPtr);
    boundFunc->funcPtr = origFunc->funcPtr;

    // Create bound name
    std::string boundName = "bound ";
    boundName += (origFunc->name ? origFunc->name : "");
    boundFunc->name = strdup(boundName.c_str());

    // Adjust length (remaining parameters after bound ones)
    boundFunc->length = origFunc->length > argCount ? origFunc->length - argCount : 0;
    boundFunc->sourceCode = nullptr;

    // Store bound this
    boundFunc->boundThis = thisArg;

    // Store bound arguments
    if (argCount > 0) {
        boundFunc->boundArgs = static_cast<void**>(malloc(argCount * sizeof(void*)));
        int64_t args[] = {a0, a1, a2, a3, a4, a5, a6, a7};
        for (int64_t i = 0; i < argCount && i < 8; i++) {
            boundFunc->boundArgs[i] = reinterpret_cast<void*>(args[i]);
        }
        boundFunc->boundArgCount = argCount;
    } else {
        boundFunc->boundArgs = nullptr;
        boundFunc->boundArgCount = 0;
    }

    return boundFunc;
}

// Call a bound function
int64_t nova_function_call_bound(void* boundFuncPtr,
                                  int64_t a0, int64_t a1, int64_t a2, int64_t a3,
                                  int64_t a4, int64_t a5, int64_t a6, int64_t a7,
                                  int64_t argCount) {
    if (!boundFuncPtr) return 0;

    NovaFunction* boundFunc = static_cast<NovaFunction*>(boundFuncPtr);

    // Combine bound args with new args
    int64_t allArgs[8] = {0};
    int64_t totalArgs = 0;

    // First add bound arguments
    for (int64_t i = 0; i < boundFunc->boundArgCount && totalArgs < 8; i++) {
        allArgs[totalArgs++] = reinterpret_cast<int64_t>(boundFunc->boundArgs[i]);
    }

    // Then add new arguments
    int64_t newArgs[] = {a0, a1, a2, a3, a4, a5, a6, a7};
    for (int64_t i = 0; i < argCount && totalArgs < 8; i++) {
        allArgs[totalArgs++] = newArgs[i];
    }

    return nova_function_call(boundFunc->funcPtr, boundFunc->boundThis,
                              allArgs[0], allArgs[1], allArgs[2], allArgs[3],
                              allArgs[4], allArgs[5], allArgs[6], allArgs[7],
                              totalArgs);
}

// ============================================================
// GeneratorFunction (ES2015) - Dynamic generator function creation
// ============================================================

// NovaGeneratorFunction structure for tracking generator function metadata
struct NovaGeneratorFunction {
    void* generatorPtr;     // Pointer to compiled generator function
    char* name;             // Function name (anonymous for dynamic)
    int64_t length;         // Number of parameters
    char* body;             // Source body for toString
    char** paramNames;      // Parameter names
    int64_t paramCount;     // Number of parameters
};

// GeneratorFunction constructor - creates generator function dynamically
// AOT limitation: Only supports compile-time constant bodies
// Runtime call throws error for dynamic code
void* nova_generator_function_create(const char* body, const char** paramNames, int64_t paramCount) {
    // In AOT compiler, dynamic code generation is not possible
    // This runtime function is called when compile-time parsing fails
    fprintf(stderr, "Error: GeneratorFunction with dynamic body is not supported in AOT compilation.\n");
    fprintf(stderr, "       Use generator function declarations (function* name() {}) instead.\n");

    // Return a stub generator function
    NovaGeneratorFunction* genFunc = static_cast<NovaGeneratorFunction*>(malloc(sizeof(NovaGeneratorFunction)));
    genFunc->generatorPtr = nullptr;
    genFunc->name = strdup("anonymous");
    genFunc->length = paramCount;
    genFunc->body = body ? strdup(body) : strdup("");
    genFunc->paramNames = nullptr;
    genFunc->paramCount = paramCount;

    if (paramCount > 0 && paramNames) {
        genFunc->paramNames = static_cast<char**>(malloc(paramCount * sizeof(char*)));
        for (int64_t i = 0; i < paramCount; i++) {
            genFunc->paramNames[i] = paramNames[i] ? strdup(paramNames[i]) : strdup("");
        }
    }

    return genFunc;
}

// GeneratorFunction.prototype.toString()
void* nova_generator_function_toString(void* genFuncPtr) {
    if (!genFuncPtr) {
        return strdup("function* anonymous() { [native code] }");
    }

    NovaGeneratorFunction* genFunc = static_cast<NovaGeneratorFunction*>(genFuncPtr);

    std::string result = "function* ";
    result += (genFunc->name ? genFunc->name : "anonymous");
    result += "(";

    // Add parameter names
    for (int64_t i = 0; i < genFunc->paramCount; i++) {
        if (i > 0) result += ", ";
        if (genFunc->paramNames && genFunc->paramNames[i]) {
            result += genFunc->paramNames[i];
        }
    }

    result += ") { ";
    if (genFunc->body && genFunc->body[0] != '\0') {
        result += genFunc->body;
    } else {
        result += "[native code]";
    }
    result += " }";

    return strdup(result.c_str());
}

// GeneratorFunction.prototype.length - get parameter count
int64_t nova_generator_function_get_length(void* genFuncPtr) {
    if (!genFuncPtr) return 0;
    NovaGeneratorFunction* genFunc = static_cast<NovaGeneratorFunction*>(genFuncPtr);
    return genFunc->length;
}

// GeneratorFunction.prototype.name - get function name
void* nova_generator_function_get_name(void* genFuncPtr) {
    if (!genFuncPtr) {
        return strdup("anonymous");
    }
    NovaGeneratorFunction* genFunc = static_cast<NovaGeneratorFunction*>(genFuncPtr);
    return strdup(genFunc->name ? genFunc->name : "anonymous");
}

// ============================================================
// AsyncGeneratorFunction (ES2018) - Dynamic async generator function creation
// ============================================================

// NovaAsyncGeneratorFunction structure
struct NovaAsyncGeneratorFunction {
    void* asyncGeneratorPtr;  // Pointer to compiled async generator function
    char* name;               // Function name
    int64_t length;           // Number of parameters
    char* body;               // Source body for toString
    char** paramNames;        // Parameter names
    int64_t paramCount;       // Number of parameters
};

// AsyncGeneratorFunction constructor
void* nova_async_generator_function_create(const char* body, const char** paramNames, int64_t paramCount) {
    // In AOT compiler, dynamic code generation is not possible
    fprintf(stderr, "Error: AsyncGeneratorFunction with dynamic body is not supported in AOT compilation.\n");
    fprintf(stderr, "       Use async generator function declarations (async function* name() {}) instead.\n");

    NovaAsyncGeneratorFunction* asyncGenFunc = static_cast<NovaAsyncGeneratorFunction*>(malloc(sizeof(NovaAsyncGeneratorFunction)));
    asyncGenFunc->asyncGeneratorPtr = nullptr;
    asyncGenFunc->name = strdup("anonymous");
    asyncGenFunc->length = paramCount;
    asyncGenFunc->body = body ? strdup(body) : strdup("");
    asyncGenFunc->paramNames = nullptr;
    asyncGenFunc->paramCount = paramCount;

    if (paramCount > 0 && paramNames) {
        asyncGenFunc->paramNames = static_cast<char**>(malloc(paramCount * sizeof(char*)));
        for (int64_t i = 0; i < paramCount; i++) {
            asyncGenFunc->paramNames[i] = paramNames[i] ? strdup(paramNames[i]) : strdup("");
        }
    }

    return asyncGenFunc;
}

// AsyncGeneratorFunction.prototype.toString()
void* nova_async_generator_function_toString(void* asyncGenFuncPtr) {
    if (!asyncGenFuncPtr) {
        return strdup("async function* anonymous() { [native code] }");
    }

    NovaAsyncGeneratorFunction* asyncGenFunc = static_cast<NovaAsyncGeneratorFunction*>(asyncGenFuncPtr);

    std::string result = "async function* ";
    result += (asyncGenFunc->name ? asyncGenFunc->name : "anonymous");
    result += "(";

    for (int64_t i = 0; i < asyncGenFunc->paramCount; i++) {
        if (i > 0) result += ", ";
        if (asyncGenFunc->paramNames && asyncGenFunc->paramNames[i]) {
            result += asyncGenFunc->paramNames[i];
        }
    }

    result += ") { ";
    if (asyncGenFunc->body && asyncGenFunc->body[0] != '\0') {
        result += asyncGenFunc->body;
    } else {
        result += "[native code]";
    }
    result += " }";

    return strdup(result.c_str());
}

// AsyncGeneratorFunction.prototype.length
int64_t nova_async_generator_function_get_length(void* asyncGenFuncPtr) {
    if (!asyncGenFuncPtr) return 0;
    NovaAsyncGeneratorFunction* asyncGenFunc = static_cast<NovaAsyncGeneratorFunction*>(asyncGenFuncPtr);
    return asyncGenFunc->length;
}

// AsyncGeneratorFunction.prototype.name
void* nova_async_generator_function_get_name(void* asyncGenFuncPtr) {
    if (!asyncGenFuncPtr) {
        return strdup("anonymous");
    }
    NovaAsyncGeneratorFunction* asyncGenFunc = static_cast<NovaAsyncGeneratorFunction*>(asyncGenFuncPtr);
    return strdup(asyncGenFunc->name ? asyncGenFunc->name : "anonymous");
}

} // extern "C"