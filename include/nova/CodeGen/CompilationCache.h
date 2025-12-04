#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <llvm/IR/Module.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/Bitcode/BitcodeReader.h>
#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/Support/MemoryBuffer.h>

namespace nova::codegen {

// Cache entry metadata
struct CacheEntry {
    std::string sourceHash;        // SHA256 of source file
    std::string bitcodePath;       // Path to cached .bc file
    uint64_t sourceModTime;        // Source file modification time
    uint64_t cacheTime;            // When this was cached
    size_t sourceSize;             // Source file size
};

// Compilation cache for storing and retrieving pre-compiled LLVM modules
class CompilationCache {
public:
    CompilationCache();
    ~CompilationCache() = default;

    // Set cache directory (default: .nova-cache)
    void setCacheDir(const std::string& dir);

    // Check if a cached version exists and is valid
    bool hasValidCache(const std::string& sourceFile);

    // Get cached module (returns nullptr if not found or invalid)
    std::unique_ptr<llvm::Module> getCachedModule(
        const std::string& sourceFile,
        llvm::LLVMContext& context
    );

    // Store compiled module in cache
    bool cacheModule(
        const std::string& sourceFile,
        llvm::Module* module
    );

    // Clear all cached entries
    void clearCache();

    // Get cache statistics
    struct CacheStats {
        size_t totalEntries;
        size_t totalSize;
        size_t hitCount;
        size_t missCount;
    };
    CacheStats getStats() const;

    // Enable/disable caching
    void setEnabled(bool enabled) { cacheEnabled = enabled; }
    bool isEnabled() const { return cacheEnabled; }

private:
    std::string cacheDir;
    bool cacheEnabled;

    // Statistics
    mutable size_t hitCount = 0;
    mutable size_t missCount = 0;

    // Get cache file path for a source file
    std::string getCachePath(const std::string& sourceFile) const;

    // Compute hash of source file
    std::string computeSourceHash(const std::string& sourceFile) const;

    // Check if source has been modified since cache was created
    bool isStale(const std::string& sourceFile, const CacheEntry& entry) const;

    // Load cache metadata
    bool loadCacheEntry(const std::string& sourceFile, CacheEntry& entry) const;

    // Save cache metadata
    bool saveCacheEntry(const std::string& sourceFile, const CacheEntry& entry);
};

// Global cache instance
CompilationCache& getGlobalCache();

} // namespace nova::codegen
