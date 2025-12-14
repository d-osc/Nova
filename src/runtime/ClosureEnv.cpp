#include <cstdlib>
#include <cstring>

extern "C" {

// Allocate a closure environment on the heap
// size: size in bytes of the environment struct
void* nova_alloc_closure_env(size_t size) {
    void* env = malloc(size);
    if (env) {
        // Zero-initialize the memory
        memset(env, 0, size);
    }
    return env;
}

// Free a closure environment
void nova_free_closure_env(void* env) {
    if (env) {
        free(env);
    }
}

}
