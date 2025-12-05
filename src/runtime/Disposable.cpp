// Nova Runtime - DisposableStack and AsyncDisposableStack Implementation
// ES2024 Explicit Resource Management

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <vector>
#include <functional>

extern "C" {

// Forward declarations for console functions
void nova_console_log_string(const char* str);
void nova_console_error_string(const char* str);

// ============================================================================
// Disposable Resource Entry
// ============================================================================
struct DisposableEntry {
    enum class Type {
        USE,      // use() - calls [Symbol.dispose]() on value
        ADOPT,    // adopt() - calls onDispose callback with value
        DEFER     // defer() - calls callback with no arguments
    };

    Type type;
    void* value;           // The resource value (for USE and ADOPT)
    void* callback;        // Function pointer for dispose/callback
    bool isAsync;          // Whether this is an async disposable
};

// ============================================================================
// DisposableStack Structure
// ============================================================================
struct NovaDisposableStack {
    std::vector<DisposableEntry> entries;
    bool disposed;
    bool moved;  // True if ownership was transferred via move()
};

// ============================================================================
// DisposableStack Creation
// ============================================================================

// new DisposableStack()
void* nova_disposablestack_create() {
    NovaDisposableStack* stack = new NovaDisposableStack();
    stack->disposed = false;
    stack->moved = false;
    return stack;
}

// DisposableStack.prototype.disposed getter
int64_t nova_disposablestack_get_disposed(void* stackPtr) {
    if (!stackPtr) return 1;
    NovaDisposableStack* stack = static_cast<NovaDisposableStack*>(stackPtr);
    return stack->disposed ? 1 : 0;
}

// ============================================================================
// DisposableStack.prototype.use(value)
// Adds a resource with [Symbol.dispose]() to be disposed
// Returns the value for chaining
// ============================================================================
void* nova_disposablestack_use(void* stackPtr, void* value, void* disposeFunc) {
    if (!stackPtr) return value;
    NovaDisposableStack* stack = static_cast<NovaDisposableStack*>(stackPtr);

    if (stack->disposed) {
        nova_console_error_string("DisposableStack: Cannot use() - stack already disposed");
        return value;
    }

    if (stack->moved) {
        nova_console_error_string("DisposableStack: Cannot use() - stack was moved");
        return value;
    }

    DisposableEntry entry;
    entry.type = DisposableEntry::Type::USE;
    entry.value = value;
    entry.callback = disposeFunc;
    entry.isAsync = false;

    stack->entries.push_back(entry);
    return value;
}

// ============================================================================
// DisposableStack.prototype.adopt(value, onDispose)
// Adds a value with a custom dispose callback
// onDispose is called with the value when disposing
// ============================================================================
void* nova_disposablestack_adopt(void* stackPtr, void* value, void* onDispose) {
    if (!stackPtr) return value;
    NovaDisposableStack* stack = static_cast<NovaDisposableStack*>(stackPtr);

    if (stack->disposed) {
        nova_console_error_string("DisposableStack: Cannot adopt() - stack already disposed");
        return value;
    }

    if (stack->moved) {
        nova_console_error_string("DisposableStack: Cannot adopt() - stack was moved");
        return value;
    }

    DisposableEntry entry;
    entry.type = DisposableEntry::Type::ADOPT;
    entry.value = value;
    entry.callback = onDispose;
    entry.isAsync = false;

    stack->entries.push_back(entry);
    return value;
}

// ============================================================================
// DisposableStack.prototype.defer(onDispose)
// Adds a callback to be called when disposing (no value)
// ============================================================================
void nova_disposablestack_defer(void* stackPtr, void* onDispose) {
    if (!stackPtr) return;
    NovaDisposableStack* stack = static_cast<NovaDisposableStack*>(stackPtr);

    if (stack->disposed) {
        nova_console_error_string("DisposableStack: Cannot defer() - stack already disposed");
        return;
    }

    if (stack->moved) {
        nova_console_error_string("DisposableStack: Cannot defer() - stack was moved");
        return;
    }

    DisposableEntry entry;
    entry.type = DisposableEntry::Type::DEFER;
    entry.value = nullptr;
    entry.callback = onDispose;
    entry.isAsync = false;

    stack->entries.push_back(entry);
}

// ============================================================================
// DisposableStack.prototype.dispose()
// Disposes all resources in reverse order (LIFO)
// ============================================================================
void nova_disposablestack_dispose(void* stackPtr) {
    if (!stackPtr) return;
    NovaDisposableStack* stack = static_cast<NovaDisposableStack*>(stackPtr);

    if (stack->disposed) {
        return;  // Already disposed, no-op
    }

    if (stack->moved) {
        nova_console_error_string("DisposableStack: Cannot dispose() - stack was moved");
        return;
    }

    stack->disposed = true;

    // Dispose in reverse order (LIFO)
    for (auto it = stack->entries.rbegin(); it != stack->entries.rend(); ++it) {
        DisposableEntry& entry = *it;

        if (entry.callback) {
            typedef void (*NoArgCallback)();
            typedef void (*OneArgCallback)(void*);

            switch (entry.type) {
                case DisposableEntry::Type::USE:
                case DisposableEntry::Type::ADOPT:
                    // Call callback with value
                    reinterpret_cast<OneArgCallback>(entry.callback)(entry.value);
                    break;

                case DisposableEntry::Type::DEFER:
                    // Call callback with no arguments
                    reinterpret_cast<NoArgCallback>(entry.callback)();
                    break;
            }
        }
    }

    stack->entries.clear();
}

// ============================================================================
// DisposableStack.prototype.move()
// Transfers ownership to a new DisposableStack
// Returns the new stack, original becomes empty and unusable
// ============================================================================
void* nova_disposablestack_move(void* stackPtr) {
    if (!stackPtr) return nova_disposablestack_create();
    NovaDisposableStack* stack = static_cast<NovaDisposableStack*>(stackPtr);

    if (stack->disposed) {
        nova_console_error_string("DisposableStack: Cannot move() - stack already disposed");
        return nova_disposablestack_create();
    }

    if (stack->moved) {
        nova_console_error_string("DisposableStack: Cannot move() - stack was already moved");
        return nova_disposablestack_create();
    }

    // Create new stack with moved entries
    NovaDisposableStack* newStack = new NovaDisposableStack();
    newStack->entries = std::move(stack->entries);
    newStack->disposed = false;
    newStack->moved = false;

    // Mark original as moved
    stack->moved = true;
    stack->entries.clear();

    return newStack;
}

// ============================================================================
// DisposableStack[Symbol.dispose]()
// Same as dispose() - allows using DisposableStack with 'using'
// ============================================================================
void nova_disposablestack_symbol_dispose(void* stackPtr) {
    nova_disposablestack_dispose(stackPtr);
}

// ============================================================================
// AsyncDisposableStack Structure (same as DisposableStack but async)
// ============================================================================
struct NovaAsyncDisposableStack {
    std::vector<DisposableEntry> entries;
    bool disposed;
    bool moved;
};

// ============================================================================
// AsyncDisposableStack Creation
// ============================================================================

// new AsyncDisposableStack()
void* nova_asyncdisposablestack_create() {
    NovaAsyncDisposableStack* stack = new NovaAsyncDisposableStack();
    stack->disposed = false;
    stack->moved = false;
    return stack;
}

// AsyncDisposableStack.prototype.disposed getter
int64_t nova_asyncdisposablestack_get_disposed(void* stackPtr) {
    if (!stackPtr) return 1;
    NovaAsyncDisposableStack* stack = static_cast<NovaAsyncDisposableStack*>(stackPtr);
    return stack->disposed ? 1 : 0;
}

// ============================================================================
// AsyncDisposableStack.prototype.use(value)
// ============================================================================
void* nova_asyncdisposablestack_use(void* stackPtr, void* value, void* disposeFunc) {
    if (!stackPtr) return value;
    NovaAsyncDisposableStack* stack = static_cast<NovaAsyncDisposableStack*>(stackPtr);

    if (stack->disposed) {
        nova_console_error_string("AsyncDisposableStack: Cannot use() - stack already disposed");
        return value;
    }

    if (stack->moved) {
        nova_console_error_string("AsyncDisposableStack: Cannot use() - stack was moved");
        return value;
    }

    DisposableEntry entry;
    entry.type = DisposableEntry::Type::USE;
    entry.value = value;
    entry.callback = disposeFunc;
    entry.isAsync = true;

    stack->entries.push_back(entry);
    return value;
}

// ============================================================================
// AsyncDisposableStack.prototype.adopt(value, onDispose)
// ============================================================================
void* nova_asyncdisposablestack_adopt(void* stackPtr, void* value, void* onDispose) {
    if (!stackPtr) return value;
    NovaAsyncDisposableStack* stack = static_cast<NovaAsyncDisposableStack*>(stackPtr);

    if (stack->disposed) {
        nova_console_error_string("AsyncDisposableStack: Cannot adopt() - stack already disposed");
        return value;
    }

    if (stack->moved) {
        nova_console_error_string("AsyncDisposableStack: Cannot adopt() - stack was moved");
        return value;
    }

    DisposableEntry entry;
    entry.type = DisposableEntry::Type::ADOPT;
    entry.value = value;
    entry.callback = onDispose;
    entry.isAsync = true;

    stack->entries.push_back(entry);
    return value;
}

// ============================================================================
// AsyncDisposableStack.prototype.defer(onDispose)
// ============================================================================
void nova_asyncdisposablestack_defer(void* stackPtr, void* onDispose) {
    if (!stackPtr) return;
    NovaAsyncDisposableStack* stack = static_cast<NovaAsyncDisposableStack*>(stackPtr);

    if (stack->disposed) {
        nova_console_error_string("AsyncDisposableStack: Cannot defer() - stack already disposed");
        return;
    }

    if (stack->moved) {
        nova_console_error_string("AsyncDisposableStack: Cannot defer() - stack was moved");
        return;
    }

    DisposableEntry entry;
    entry.type = DisposableEntry::Type::DEFER;
    entry.value = nullptr;
    entry.callback = onDispose;
    entry.isAsync = true;

    stack->entries.push_back(entry);
}

// ============================================================================
// AsyncDisposableStack.prototype.disposeAsync()
// Note: In synchronous context, we run callbacks synchronously
// Full async support would require Promise integration
// ============================================================================
void nova_asyncdisposablestack_disposeAsync(void* stackPtr) {
    if (!stackPtr) return;
    NovaAsyncDisposableStack* stack = static_cast<NovaAsyncDisposableStack*>(stackPtr);

    if (stack->disposed) {
        return;
    }

    if (stack->moved) {
        nova_console_error_string("AsyncDisposableStack: Cannot disposeAsync() - stack was moved");
        return;
    }

    stack->disposed = true;

    // Dispose in reverse order (LIFO)
    // Note: In a full async implementation, we would await each callback
    for (auto it = stack->entries.rbegin(); it != stack->entries.rend(); ++it) {
        DisposableEntry& entry = *it;

        if (entry.callback) {
            typedef void (*NoArgCallback)();
            typedef void (*OneArgCallback)(void*);

            switch (entry.type) {
                case DisposableEntry::Type::USE:
                case DisposableEntry::Type::ADOPT:
                    reinterpret_cast<OneArgCallback>(entry.callback)(entry.value);
                    break;

                case DisposableEntry::Type::DEFER:
                    reinterpret_cast<NoArgCallback>(entry.callback)();
                    break;
            }
        }
    }

    stack->entries.clear();
}

// ============================================================================
// AsyncDisposableStack.prototype.move()
// ============================================================================
void* nova_asyncdisposablestack_move(void* stackPtr) {
    if (!stackPtr) return nova_asyncdisposablestack_create();
    NovaAsyncDisposableStack* stack = static_cast<NovaAsyncDisposableStack*>(stackPtr);

    if (stack->disposed) {
        nova_console_error_string("AsyncDisposableStack: Cannot move() - stack already disposed");
        return nova_asyncdisposablestack_create();
    }

    if (stack->moved) {
        nova_console_error_string("AsyncDisposableStack: Cannot move() - stack was already moved");
        return nova_asyncdisposablestack_create();
    }

    NovaAsyncDisposableStack* newStack = new NovaAsyncDisposableStack();
    newStack->entries = std::move(stack->entries);
    newStack->disposed = false;
    newStack->moved = false;

    stack->moved = true;
    stack->entries.clear();

    return newStack;
}

// ============================================================================
// AsyncDisposableStack[Symbol.asyncDispose]()
// ============================================================================
void nova_asyncdisposablestack_symbol_asyncDispose(void* stackPtr) {
    nova_asyncdisposablestack_disposeAsync(stackPtr);
}

// ============================================================================
// Symbol.dispose and Symbol.asyncDispose support
// These are well-known symbols for resource management
// ============================================================================

// Check if an object has [Symbol.dispose]
int64_t nova_has_symbol_dispose([[maybe_unused]] void* objPtr) {
    // For now, we return 0 (false) as we don't have full object introspection
    // In a full implementation, this would check the object's properties
    return 0;
}

// Check if an object has [Symbol.asyncDispose]
int64_t nova_has_symbol_asyncDispose([[maybe_unused]] void* objPtr) {
    return 0;
}

// Get Symbol.dispose value (returns a unique identifier)
int64_t nova_symbol_dispose() {
    // Return a unique constant representing Symbol.dispose
    return 0x44495350;  // "DISP" in hex
}

// Get Symbol.asyncDispose value
int64_t nova_symbol_asyncDispose() {
    // Return a unique constant representing Symbol.asyncDispose
    return 0x41444950;  // "ADIP" in hex
}

// ============================================================================
// SuppressedError for aggregating dispose errors (ES2024)
// ============================================================================
struct NovaSuppressedError {
    void* error;       // The error that was thrown
    void* suppressed;  // The error that was suppressed
    const char* message;
    const char* name;  // Always "SuppressedError"
    const char* stack; // Stack trace
};

void* nova_suppressederror_create(void* error, void* suppressed, const char* message) {
    NovaSuppressedError* err = new NovaSuppressedError();
    err->error = error;
    err->suppressed = suppressed;
    err->message = message ? message : "";
    err->name = "SuppressedError";

    // Generate stack trace (simplified)
    err->stack = "SuppressedError\n    at <anonymous>";

    return err;
}

void* nova_suppressederror_get_error(void* errPtr) {
    if (!errPtr) return nullptr;
    return static_cast<NovaSuppressedError*>(errPtr)->error;
}

void* nova_suppressederror_get_suppressed(void* errPtr) {
    if (!errPtr) return nullptr;
    return static_cast<NovaSuppressedError*>(errPtr)->suppressed;
}

const char* nova_suppressederror_get_message(void* errPtr) {
    if (!errPtr) return "";
    return static_cast<NovaSuppressedError*>(errPtr)->message;
}

// name property - returns "SuppressedError"
const char* nova_suppressederror_get_name(void* errPtr) {
    if (!errPtr) return "SuppressedError";
    return static_cast<NovaSuppressedError*>(errPtr)->name;
}

// stack property - returns stack trace
const char* nova_suppressederror_get_stack(void* errPtr) {
    if (!errPtr) return "";
    return static_cast<NovaSuppressedError*>(errPtr)->stack;
}

// toString() - returns "SuppressedError: message"
const char* nova_suppressederror_toString(void* errPtr) {
    if (!errPtr) return "SuppressedError";

    NovaSuppressedError* err = static_cast<NovaSuppressedError*>(errPtr);
    const char* msg = err->message ? err->message : "";

    if (strlen(msg) == 0) {
        return "SuppressedError";
    }

    // Build "SuppressedError: message"
    size_t len = strlen("SuppressedError: ") + strlen(msg) + 1;
    char* result = static_cast<char*>(malloc(len));
    if (!result) return "SuppressedError";

    snprintf(result, len, "SuppressedError: %s", msg);
    return result;
}

} // extern "C"
