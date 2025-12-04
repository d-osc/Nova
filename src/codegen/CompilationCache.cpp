// Compilation Cache - Production Optimization
// Caches compiled LLVM bitcode to avoid recompilation

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4100 4127 4244 4245 4267 4310 4324 4456 4458 4459 4624)
#endif

#include "nova/CodeGen/CompilationCache.h"
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/SourceMgr.h>
#include <sstream>
#include <iomanip>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/stat.h>
#endif

namespace nova::codegen {

// Simple hash function (FNV-1a)
static std::string fnv1aHash(const std::string& data) {
    uint64_t hash = 14695981039346656037ULL;
    for (char c : data) {
        hash ^= static_cast<uint64_t>(c);
        hash *= 1099511628211ULL;
    }
    std::stringstream ss;
    ss << std::hex << std::setfill('0') << std::setw(16) << hash;
    return ss.str();
}

CompilationCache::CompilationCache()
    : cacheDir(".nova-cache"), cacheEnabled(true) {
    // Create cache directory if it doesn't exist
    std::filesystem::create_directories(cacheDir);
}

void CompilationCache::setCacheDir(const std::string& dir) {
    cacheDir = dir;
    std::filesystem::create_directories(cacheDir);
}

std::string CompilationCache::getCachePath(const std::string& sourceFile) const {
    // Create a unique path based on the source file path
    std::string normalized = std::filesystem::absolute(sourceFile).string();
    std::string hash = fnv1aHash(normalized);
    return cacheDir + "/" + hash + ".bc";
}

std::string CompilationCache::computeSourceHash(const std::string& sourceFile) const {
    std::ifstream file(sourceFile, std::ios::binary);
    if (!file) return "";

    std::stringstream buffer;
    buffer << file.rdbuf();
    return fnv1aHash(buffer.str());
}

bool CompilationCache::isStale(const std::string& sourceFile, const CacheEntry& entry) const {
    try {
        auto lastWrite = std::filesystem::last_write_time(sourceFile);
        auto modTime = std::chrono::duration_cast<std::chrono::seconds>(
            lastWrite.time_since_epoch()
        ).count();

        // If source file is newer than cache, it's stale
        if (static_cast<uint64_t>(modTime) > entry.cacheTime) {
            return true;
        }

        // Also check hash for safety
        std::string currentHash = computeSourceHash(sourceFile);
        return currentHash != entry.sourceHash;
    } catch (...) {
        return true; // If we can't check, assume stale
    }
}

bool CompilationCache::loadCacheEntry(const std::string& sourceFile, CacheEntry& entry) const {
    std::string metaPath = getCachePath(sourceFile) + ".meta";
    std::ifstream metaFile(metaPath);
    if (!metaFile) return false;

    std::string line;
    while (std::getline(metaFile, line)) {
        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;

        std::string key = line.substr(0, eq);
        std::string value = line.substr(eq + 1);

        if (key == "hash") entry.sourceHash = value;
        else if (key == "mtime") entry.sourceModTime = std::stoull(value);
        else if (key == "ctime") entry.cacheTime = std::stoull(value);
        else if (key == "size") entry.sourceSize = std::stoull(value);
    }

    entry.bitcodePath = getCachePath(sourceFile);
    return true;
}

bool CompilationCache::saveCacheEntry(const std::string& sourceFile, const CacheEntry& entry) {
    std::string metaPath = getCachePath(sourceFile) + ".meta";
    std::ofstream metaFile(metaPath);
    if (!metaFile) return false;

    metaFile << "hash=" << entry.sourceHash << "\n";
    metaFile << "mtime=" << entry.sourceModTime << "\n";
    metaFile << "ctime=" << entry.cacheTime << "\n";
    metaFile << "size=" << entry.sourceSize << "\n";

    return true;
}

bool CompilationCache::hasValidCache(const std::string& sourceFile) {
    if (!cacheEnabled) return false;

    CacheEntry entry;
    if (!loadCacheEntry(sourceFile, entry)) {
        missCount++;
        return false;
    }

    // Check if bitcode file exists
    if (!std::filesystem::exists(entry.bitcodePath)) {
        missCount++;
        return false;
    }

    // Check if stale
    if (isStale(sourceFile, entry)) {
        missCount++;
        return false;
    }

    hitCount++;
    return true;
}

std::unique_ptr<llvm::Module> CompilationCache::getCachedModule(
    const std::string& sourceFile,
    llvm::LLVMContext& context
) {
    if (!hasValidCache(sourceFile)) {
        return nullptr;
    }

    CacheEntry entry;
    if (!loadCacheEntry(sourceFile, entry)) {
        return nullptr;
    }

    // Load bitcode
    auto bufferOrErr = llvm::MemoryBuffer::getFile(entry.bitcodePath);
    if (!bufferOrErr) {
        return nullptr;
    }

    auto moduleOrErr = llvm::parseBitcodeFile(
        bufferOrErr.get()->getMemBufferRef(),
        context
    );

    if (!moduleOrErr) {
        return nullptr;
    }

    return std::move(moduleOrErr.get());
}

bool CompilationCache::cacheModule(
    const std::string& sourceFile,
    llvm::Module* module
) {
    if (!cacheEnabled || !module) return false;

    std::string cachePath = getCachePath(sourceFile);

    // Write bitcode
    std::error_code EC;
    llvm::raw_fd_ostream bcFile(cachePath, EC, llvm::sys::fs::OF_None);
    if (EC) {
        return false;
    }

    llvm::WriteBitcodeToFile(*module, bcFile);
    bcFile.close();

    // Create cache entry
    CacheEntry entry;
    entry.sourceHash = computeSourceHash(sourceFile);
    entry.bitcodePath = cachePath;

    auto now = std::chrono::system_clock::now();
    entry.cacheTime = std::chrono::duration_cast<std::chrono::seconds>(
        now.time_since_epoch()
    ).count();

    try {
        entry.sourceSize = std::filesystem::file_size(sourceFile);
        auto lastWrite = std::filesystem::last_write_time(sourceFile);
        entry.sourceModTime = std::chrono::duration_cast<std::chrono::seconds>(
            lastWrite.time_since_epoch()
        ).count();
    } catch (...) {
        entry.sourceSize = 0;
        entry.sourceModTime = 0;
    }

    return saveCacheEntry(sourceFile, entry);
}

void CompilationCache::clearCache() {
    try {
        std::filesystem::remove_all(cacheDir);
        std::filesystem::create_directories(cacheDir);
        hitCount = 0;
        missCount = 0;
    } catch (...) {}
}

CompilationCache::CacheStats CompilationCache::getStats() const {
    CacheStats stats;
    stats.hitCount = hitCount;
    stats.missCount = missCount;
    stats.totalEntries = 0;
    stats.totalSize = 0;

    try {
        for (const auto& entry : std::filesystem::directory_iterator(cacheDir)) {
            if (entry.path().extension() == ".bc") {
                stats.totalEntries++;
                stats.totalSize += std::filesystem::file_size(entry.path());
            }
        }
    } catch (...) {}

    return stats;
}

// Global cache instance
CompilationCache& getGlobalCache() {
    static CompilationCache cache;
    return cache;
}

} // namespace nova::codegen

#ifdef _MSC_VER
#pragma warning(pop)
#endif
