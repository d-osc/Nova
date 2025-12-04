// LLVM Lazy Initialization - Production Optimization
// Ensures LLVM targets are only initialized once per process

#include "nova/CodeGen/LLVMInit.h"
#include <llvm/Support/TargetSelect.h>

namespace nova::codegen {

void LLVMInit::ensureInitialized() {
    // Fast path: already initialized
    if (initialized.load(std::memory_order_acquire)) {
        return;
    }

    // Slow path: need to initialize (thread-safe)
    std::lock_guard<std::mutex> lock(initMutex);

    // Double-check after acquiring lock
    if (initialized.load(std::memory_order_relaxed)) {
        return;
    }

    // Initialize LLVM targets (only once per process)
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();

    // Mark as initialized
    initialized.store(true, std::memory_order_release);
}

} // namespace nova::codegen
