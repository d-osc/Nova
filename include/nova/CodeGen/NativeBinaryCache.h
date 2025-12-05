#pragma once

#include <string>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <cstdlib>

#ifdef _WIN32
#include <windows.h>
#include <process.h>
#else
#include <unistd.h>
#include <sys/wait.h>
#endif

namespace nova::codegen {

// Native Binary Cache - caches compiled executables for instant execution
class NativeBinaryCache {
public:
    NativeBinaryCache() : cacheDir(".nova-cache/bin") {
        std::filesystem::create_directories(cacheDir);
    }

    // Get cache directory
    void setCacheDir(const std::string& dir) {
        cacheDir = dir + "/bin";
        std::filesystem::create_directories(cacheDir);
    }

    // Compute hash of source file content
    static std::string hashSource(const std::string& content) {
        // FNV-1a hash
        uint64_t hash = 14695981039346656037ULL;
        for (char c : content) {
            hash ^= static_cast<uint64_t>(c);
            hash *= 1099511628211ULL;
        }

        std::stringstream ss;
        ss << std::hex << hash;
        return ss.str();
    }

    // Get cached executable path for a source file
    std::string getCachedExePath([[maybe_unused]] const std::string& sourceFile, const std::string& sourceContent) {
        std::string hash = hashSource(sourceContent);

#ifdef _WIN32
        return cacheDir + "/" + hash + ".exe";
#else
        return cacheDir + "/" + hash;
#endif
    }

    // Check if valid cached executable exists
    bool hasCachedExecutable(const std::string& exePath) {
        return std::filesystem::exists(exePath);
    }

    // Execute cached binary and return exit code
    int executeCached(const std::string& exePath) {
#ifdef _WIN32
        // Use CreateProcess for Windows with absolute path
        STARTUPINFOA si;
        PROCESS_INFORMATION pi;
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        si.dwFlags = STARTF_USESTDHANDLES;
        si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
        si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
        si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
        ZeroMemory(&pi, sizeof(pi));

        // Convert to absolute path
        std::string absPath = std::filesystem::absolute(exePath).string();
        std::string cmdLine = "\"" + absPath + "\"";

        if (!CreateProcessA(
            absPath.c_str(),
            NULL,
            NULL, NULL, TRUE,  // Inherit handles
            0, NULL, NULL,
            &si, &pi
        )) {
            return -1;
        }

        WaitForSingleObject(pi.hProcess, INFINITE);

        DWORD exitCode;
        GetExitCodeProcess(pi.hProcess, &exitCode);

        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);

        return static_cast<int>(exitCode);
#else
        // Use fork/exec for Unix
        pid_t pid = fork();
        if (pid == 0) {
            execl(exePath.c_str(), exePath.c_str(), nullptr);
            _exit(127);
        } else if (pid > 0) {
            int status;
            waitpid(pid, &status, 0);
            if (WIFEXITED(status)) {
                return WEXITSTATUS(status);
            }
            return -1;
        }
        return -1;
#endif
    }

    // Save compiled executable to cache
    bool saveToCache(const std::string& tempExePath, const std::string& cachePath) {
        try {
            std::filesystem::copy_file(
                tempExePath,
                cachePath,
                std::filesystem::copy_options::overwrite_existing
            );
            return true;
        } catch (...) {
            return false;
        }
    }

    // Clear binary cache
    void clearCache() {
        try {
            std::filesystem::remove_all(cacheDir);
            std::filesystem::create_directories(cacheDir);
        } catch (...) {}
    }

    // Get cache stats
    struct Stats {
        size_t numBinaries;
        size_t totalSize;
    };

    Stats getStats() const {
        Stats stats{0, 0};
        try {
            for (const auto& entry : std::filesystem::directory_iterator(cacheDir)) {
                if (entry.is_regular_file()) {
                    stats.numBinaries++;
                    stats.totalSize += std::filesystem::file_size(entry.path());
                }
            }
        } catch (...) {}
        return stats;
    }

private:
    std::string cacheDir;
};

// Global instance
inline NativeBinaryCache& getNativeBinaryCache() {
    static NativeBinaryCache cache;
    return cache;
}

} // namespace nova::codegen
