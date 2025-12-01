#pragma once

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <filesystem>

namespace nova {
namespace pm {

// Package dependency info
struct PackageInfo {
    std::string name;
    std::string version;         // Semver or tag
    std::string resolvedVersion; // Actual resolved version
    std::string tarballUrl;
    std::string integrity;       // SHA hash
    bool isDev = false;
    std::vector<std::string> dependencies;
};

// Install result
struct InstallResult {
    bool success;
    int totalPackages;
    int cachedPackages;      // Used from cache
    int downloadedPackages;  // Freshly downloaded
    int apiOnlyPackages;     // Cache hit, API called for stats only
    double totalTimeMs;
    size_t totalSizeBytes;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
};

// Progress callback
using ProgressCallback = std::function<void(const std::string& package, int current, int total, bool fromCache)>;

class PackageManager {
public:
    PackageManager();
    ~PackageManager();

    // Initialize a new project
    bool init(const std::string& projectPath = ".", bool withTypeScript = false);

    // Run automated tests
    int runTests(const std::string& projectPath = ".", const std::string& pattern = "");

    // Set cache directory (default: ~/.nova/cache)
    void setCacheDir(const std::string& path);
    std::string getCacheDir() const { return cacheDir_; }

    // Set registry URL (default: https://registry.npmjs.org)
    void setRegistry(const std::string& url);
    std::string getRegistry() const { return registry_; }

    // Install packages from package.json
    InstallResult install(const std::string& projectPath = ".", bool devDependencies = true);

    // Install specific package
    InstallResult installPackage(const std::string& packageName, const std::string& version = "latest");

    // Add package to package.json and install
    InstallResult add(const std::string& packageName, const std::string& version = "latest", bool isDev = false);

    // Remove package
    bool remove(const std::string& packageName);

    // Update packages
    InstallResult update(const std::string& packageName = "");

    // Clean install from lockfile (like npm ci)
    InstallResult cleanInstall(const std::string& projectPath = ".");

    // Link current package globally
    bool link(const std::string& projectPath = ".");

    // Link a globally linked package to current project
    bool linkPackage(const std::string& packageName);

    // List installed packages
    std::vector<PackageInfo> list(bool includeTransitive = false);

    // Display dependency tree (like npm ls)
    void listDependencies(const std::string& projectPath = ".");

    // Check for outdated packages (like npm outdated)
    void checkOutdated(const std::string& projectPath = ".");

    // Login to registry
    bool login(const std::string& registry = "");

    // Logout from registry
    bool logout(const std::string& registry = "");

    // Check if logged in
    bool isLoggedIn(const std::string& registry = "");

    // Get auth token for registry
    std::string getAuthToken(const std::string& registry = "");

    // Pack project into tarball for publishing
    std::string pack(const std::string& projectPath = ".");

    // Publish package to registry
    bool publish(const std::string& projectPath = ".");

    // Clean cache
    void cleanCache(int olderThanDays = 30);

    // Set progress callback
    void setProgressCallback(ProgressCallback callback) { progressCallback_ = callback; }

    // Parallel downloads (default: 16)
    void setParallelDownloads(int count) { parallelDownloads_ = count; }

private:
    std::string cacheDir_;
    std::string registry_;
    std::string projectPath_;
    int parallelDownloads_ = 16;
    ProgressCallback progressCallback_;

    // Cache path: cacheDir/package-name/tag/version.tar.gz
    std::string getCachePath(const std::string& packageName, const std::string& tag, const std::string& version);

    // Check if package exists in cache
    bool isInCache(const std::string& packageName, const std::string& tag, const std::string& version);

    // Download package tarball
    bool downloadPackage(const PackageInfo& pkg);

    // Extract tarball to node_modules
    bool extractPackage(const std::string& tarballPath, const std::string& destPath);

    // Call API to update download stats (without downloading)
    bool pingDownloadStats(const std::string& packageName, const std::string& version);

    // Resolve package version from registry
    PackageInfo resolvePackage(const std::string& packageName, const std::string& versionRange);

    // Parse package.json
    std::map<std::string, std::string> parseDependencies(const std::string& packageJsonPath, bool dev = false);

    // Parse package-lock.json (npm lockfile v2/v3)
    std::vector<PackageInfo> parseLockfile(const std::string& lockfilePath);

    // Build dependency tree (fallback when no lockfile)
    std::vector<PackageInfo> buildDependencyTree(const std::map<std::string, std::string>& deps);

    // HTTP helpers
    std::string httpGet(const std::string& url);
    bool httpDownload(const std::string& url, const std::string& destPath);

    // Tar extraction
    bool extractTarGz(const std::string& tarPath, const std::string& destDir);

    // Version helpers
    bool satisfiesVersion(const std::string& version, const std::string& range);
    std::string getLatestVersion(const std::string& packageName);
    std::string getTagFromVersion(const std::string& version); // latest, next, beta, etc.
};

// Utility functions
std::string getDefaultCacheDir();
std::string formatBytes(size_t bytes);
std::string formatDuration(double ms);

} // namespace pm
} // namespace nova
