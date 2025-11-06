#include "nova/runtime/Runtime.h"

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