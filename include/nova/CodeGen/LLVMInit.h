#pragma once

#include <atomic>
#include <mutex>

namespace nova::codegen {

// Singleton for lazy LLVM initialization
// This ensures LLVM targets are only initialized once across all compilations
class LLVMInit {
public:
    static LLVMInit& getInstance() {
        static LLVMInit instance;
        return instance;
    }

    // Ensure LLVM is initialized (thread-safe, only runs once)
    void ensureInitialized();

    // Check if already initialized
    bool isInitialized() const { return initialized.load(std::memory_order_acquire); }

private:
    LLVMInit() = default;
    ~LLVMInit() = default;

    // Non-copyable
    LLVMInit(const LLVMInit&) = delete;
    LLVMInit& operator=(const LLVMInit&) = delete;

    std::atomic<bool> initialized{false};
    std::mutex initMutex;
};

// Convenience function
inline void ensureLLVMInitialized() {
    LLVMInit::getInstance().ensureInitialized();
}

} // namespace nova::codegen
