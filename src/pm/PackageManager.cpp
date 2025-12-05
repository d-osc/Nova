/**
 * Nova Package Manager Implementation
 * Fast package manager with caching support
 */

#include "nova/PackageManager/PackageManager.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <thread>
#include <cstdlib>
#include <regex>
#include <iomanip>

#ifdef _WIN32
#include <windows.h>
#include <wininet.h>
#include <direct.h>
#include <conio.h>
#pragma comment(lib, "wininet.lib")
#define mkdir(dir, mode) _mkdir(dir)
#else
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#endif

namespace nova {
namespace pm {

// Simple JSON parsing helpers
static std::string getJsonString(const std::string& json, const std::string& key) {
    std::string searchKey = "\"" + key + "\"";
    size_t keyPos = json.find(searchKey);
    if (keyPos == std::string::npos) return "";

    size_t colonPos = json.find(':', keyPos);
    if (colonPos == std::string::npos) return "";

    size_t startQuote = json.find('"', colonPos);
    if (startQuote == std::string::npos) return "";

    size_t endQuote = json.find('"', startQuote + 1);
    if (endQuote == std::string::npos) return "";

    return json.substr(startQuote + 1, endQuote - startQuote - 1);
}

// Get default cache directory
std::string getDefaultCacheDir() {
#ifdef _WIN32
    const char* localAppData = std::getenv("LOCALAPPDATA");
    if (localAppData) {
        return std::string(localAppData) + "\\nova\\cache";
    }
    return "C:\\nova\\cache";
#else
    const char* home = std::getenv("HOME");
    if (home) {
        return std::string(home) + "/.nova/cache";
    }
    return "/tmp/nova/cache";
#endif
}

// Get global packages directory
std::string PackageManager::getGlobalDir() {
#ifdef _WIN32
    const char* localAppData = std::getenv("LOCALAPPDATA");
    if (localAppData) {
        return std::string(localAppData) + "\\nova\\global";
    }
    return "C:\\nova\\global";
#else
    const char* home = std::getenv("HOME");
    if (home) {
        return std::string(home) + "/.nova/global";
    }
    return "/usr/local/lib/nova/global";
#endif
}

// Get dependency key name from type
static std::string getDependencyKeyFromType(DependencyType depType) {
    switch (depType) {
        case DependencyType::Production:
            return "dependencies";
        case DependencyType::Development:
            return "devDependencies";
        case DependencyType::Peer:
            return "peerDependencies";
        case DependencyType::Optional:
            return "optionalDependencies";
        default:
            return "dependencies";
    }
}

// Format bytes for display
std::string formatBytes(size_t bytes) {
    if (bytes < 1024) return std::to_string(bytes) + " B";
    if (bytes < 1024 * 1024) return std::to_string(bytes / 1024) + " KB";
    return std::to_string(bytes / (1024 * 1024)) + " MB";
}

// Format duration for display
std::string formatDuration(double ms) {
    if (ms < 1000) return std::to_string((int)ms) + "ms";
    return std::to_string((int)(ms / 1000)) + "." + std::to_string((int)ms % 1000 / 100) + "s";
}

// Credentials path
static std::string getCredentialsPath() {
#ifdef _WIN32
    const char* localAppData = std::getenv("LOCALAPPDATA");
    if (localAppData) {
        return std::string(localAppData) + "\\nova\\credentials.json";
    }
    return "C:\\nova\\credentials.json";
#else
    const char* home = std::getenv("HOME");
    if (home) {
        return std::string(home) + "/.nova/credentials.json";
    }
    return "/tmp/nova/credentials.json";
#endif
}

// Load credentials from file
static std::map<std::string, std::string> loadCredentials() {
    std::map<std::string, std::string> creds;
    std::ifstream file(getCredentialsPath());
    if (!file.is_open()) return creds;

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();

    // Simple JSON parsing for credentials
    size_t pos = 0;
    while ((pos = content.find("\"", pos)) != std::string::npos) {
        size_t keyEnd = content.find("\"", pos + 1);
        if (keyEnd == std::string::npos) break;

        std::string key = content.substr(pos + 1, keyEnd - pos - 1);

        size_t colonPos = content.find(":", keyEnd);
        if (colonPos == std::string::npos) break;

        size_t valueStart = content.find("\"", colonPos);
        if (valueStart == std::string::npos) break;

        size_t valueEnd = content.find("\"", valueStart + 1);
        if (valueEnd == std::string::npos) break;

        std::string value = content.substr(valueStart + 1, valueEnd - valueStart - 1);
        creds[key] = value;

        pos = valueEnd + 1;
    }

    return creds;
}

// Save credentials to file
static void saveCredentials(const std::map<std::string, std::string>& creds) {
    std::filesystem::path credPath = getCredentialsPath();
    std::filesystem::create_directories(credPath.parent_path());

    std::ofstream file(credPath);
    file << "{\n";
    bool first = true;
    for (const auto& [key, value] : creds) {
        if (!first) file << ",\n";
        file << "  \"" << key << "\": \"" << value << "\"";
        first = false;
    }
    file << "\n}\n";
}

// Base64 encoding for publish
static std::string base64Encode(const std::vector<unsigned char>& data) {
    static const char* chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string result;
    result.reserve((data.size() + 2) / 3 * 4);

    for (size_t i = 0; i < data.size(); i += 3) {
        unsigned int n = data[i] << 16;
        if (i + 1 < data.size()) n |= data[i + 1] << 8;
        if (i + 2 < data.size()) n |= data[i + 2];

        result += chars[(n >> 18) & 0x3F];
        result += chars[(n >> 12) & 0x3F];
        result += (i + 1 < data.size()) ? chars[(n >> 6) & 0x3F] : '=';
        result += (i + 2 < data.size()) ? chars[n & 0x3F] : '=';
    }

    return result;
}

// Get user's home directory
static std::string getHomeDir() {
#ifdef _WIN32
    const char* userProfile = std::getenv("USERPROFILE");
    if (userProfile) return userProfile;
    const char* homeDrive = std::getenv("HOMEDRIVE");
    const char* homePath = std::getenv("HOMEPATH");
    if (homeDrive && homePath) {
        return std::string(homeDrive) + homePath;
    }
    return "C:\\";
#else
    const char* home = std::getenv("HOME");
    if (home) return home;
    return "/tmp";
#endif
}

// Get global npmrc path
static std::string getGlobalNpmrcPath() {
#ifdef _WIN32
    const char* appData = std::getenv("APPDATA");
    if (appData) {
        return std::string(appData) + "\\npm\\etc\\npmrc";
    }
    return "C:\\Program Files\\nodejs\\etc\\npmrc";
#else
    return "/etc/npmrc";
#endif
}

// Constructor
PackageManager::PackageManager() {
    cacheDir_ = getDefaultCacheDir();
    registry_ = "https://registry.npmjs.org";
    projectPath_ = ".";
    // Load default .npmrc from home directory
    loadNpmrc(".");
}

// Destructor
PackageManager::~PackageManager() {
}

// Parse a single .npmrc file
void PackageManager::parseNpmrcFile(const std::string& filePath, NpmrcConfig& config) {
    std::ifstream file(filePath);
    if (!file.is_open()) return;

    std::string line;
    while (std::getline(file, line)) {
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#' || line[0] == ';') continue;

        // Trim whitespace
        size_t start = line.find_first_not_of(" \t");
        size_t end = line.find_last_not_of(" \t\r\n");
        if (start == std::string::npos) continue;
        line = line.substr(start, end - start + 1);

        // Handle environment variable substitution ${VAR}
        size_t varStart;
        while ((varStart = line.find("${")) != std::string::npos) {
            size_t varEnd = line.find("}", varStart);
            if (varEnd == std::string::npos) break;
            std::string varName = line.substr(varStart + 2, varEnd - varStart - 2);
            const char* varValue = std::getenv(varName.c_str());
            line.replace(varStart, varEnd - varStart + 1, varValue ? varValue : "");
        }

        // Find the = separator
        size_t eqPos = line.find('=');
        if (eqPos == std::string::npos) continue;

        std::string key = line.substr(0, eqPos);
        std::string value = line.substr(eqPos + 1);

        // Trim key and value
        size_t keyEnd = key.find_last_not_of(" \t");
        if (keyEnd != std::string::npos) key = key.substr(0, keyEnd + 1);

        size_t valueStart = value.find_first_not_of(" \t");
        if (valueStart != std::string::npos) value = value.substr(valueStart);

        // Handle scoped registry: @scope:registry=url
        if (key[0] == '@' && key.find(":registry") != std::string::npos) {
            size_t colonPos = key.find(':');
            std::string scope = key.substr(0, colonPos);
            config.scopedRegistries[scope] = value;
            continue;
        }

        // Handle auth tokens: //registry.npmjs.org/:_authToken=xxx
        if (key.substr(0, 2) == "//") {
            size_t authPos = key.find(":_authToken");
            if (authPos != std::string::npos) {
                std::string registry = key.substr(0, authPos);
                config.authTokens[registry] = value;
                continue;
            }
            size_t authBasicPos = key.find(":_auth");
            if (authBasicPos != std::string::npos) {
                std::string registry = key.substr(0, authBasicPos);
                config.authBasic[registry] = value;
                continue;
            }
        }

        // Handle standard settings
        if (key == "registry") {
            config.registry = value;
        } else if (key == "save-exact") {
            config.saveExact = (value == "true" || value == "1");
        } else if (key == "save-prefix") {
            config.savePrefix = (value != "false" && value != "0");
        } else if (key == "prefix") {
            config.prefix = value;
        } else if (key == "strict-ssl") {
            config.strictSSL = (value == "true" || value == "1");
        } else if (key == "cafile") {
            config.cafile = value;
        } else if (key == "proxy") {
            config.proxy = value;
        } else if (key == "https-proxy") {
            config.httpsProxy = value;
        } else if (key == "progress") {
            config.progress = (value == "true" || value == "1");
        } else if (key == "fetch-retries") {
            try {
                config.fetchRetries = std::stoi(value);
            } catch (...) {}
        } else if (key == "fetch-timeout") {
            try {
                config.fetchTimeout = std::stoi(value);
            } catch (...) {}
        } else {
            // Store any other settings
            config.customSettings[key] = value;
        }
    }
}

// Load .npmrc configuration from multiple sources
void PackageManager::loadNpmrc(const std::string& projectPath) {
    // Start with defaults
    npmrcConfig_ = NpmrcConfig();
    npmrcConfig_.registry = "https://registry.npmjs.org";

    // 1. Load global npmrc (/etc/npmrc or C:\Program Files\nodejs\etc\npmrc)
    parseNpmrcFile(getGlobalNpmrcPath(), npmrcConfig_);

    // 2. Load user's .npmrc (~/.npmrc)
    std::string userNpmrc = getHomeDir() +
#ifdef _WIN32
        "\\.npmrc";
#else
        "/.npmrc";
#endif
    parseNpmrcFile(userNpmrc, npmrcConfig_);

    // 3. Load project's .npmrc (projectPath/.npmrc)
    std::filesystem::path projectNpmrc = std::filesystem::absolute(projectPath) / ".npmrc";
    parseNpmrcFile(projectNpmrc.string(), npmrcConfig_);

    // Apply registry from config
    if (!npmrcConfig_.registry.empty()) {
        registry_ = npmrcConfig_.registry;
    }
}

// Get registry URL for a specific package (handles scoped packages)
std::string PackageManager::getRegistryForPackage(const std::string& packageName) const {
    // Check if it's a scoped package (@scope/name)
    if (!packageName.empty() && packageName[0] == '@') {
        size_t slashPos = packageName.find('/');
        if (slashPos != std::string::npos) {
            std::string scope = packageName.substr(0, slashPos);
            auto it = npmrcConfig_.scopedRegistries.find(scope);
            if (it != npmrcConfig_.scopedRegistries.end()) {
                return it->second;
            }
        }
    }
    return registry_;
}

// Get auth token for a specific registry
std::string PackageManager::getAuthTokenForRegistry(const std::string& registryUrl) const {
    // Extract hostname from URL for matching
    // e.g., https://registry.npmjs.org -> //registry.npmjs.org
    std::string host;
    size_t protocolEnd = registryUrl.find("://");
    if (protocolEnd != std::string::npos) {
        host = "//" + registryUrl.substr(protocolEnd + 3);
    } else {
        host = "//" + registryUrl;
    }

    // Remove trailing slash
    if (!host.empty() && host.back() == '/') {
        host.pop_back();
    }

    // Look for exact match first
    auto it = npmrcConfig_.authTokens.find(host);
    if (it != npmrcConfig_.authTokens.end()) {
        return it->second;
    }

    // Try with trailing slash
    it = npmrcConfig_.authTokens.find(host + "/");
    if (it != npmrcConfig_.authTokens.end()) {
        return it->second;
    }

    return "";
}

// Set cache directory
void PackageManager::setCacheDir(const std::string& path) {
    cacheDir_ = path;
}

// Set registry URL
void PackageManager::setRegistry(const std::string& url) {
    registry_ = url;
}

// Initialize a new project
bool PackageManager::init(const std::string& projectPath, bool withTypeScript) {
    projectPath_ = projectPath;
    std::filesystem::path basePath = std::filesystem::absolute(projectPath);

    std::cout << "[nova] Initializing new project..." << std::endl;
    std::cout << std::endl;

    // Get project name from directory
    std::string projectName = basePath.filename().string();
    if (projectName.empty() || projectName == ".") {
        projectName = "my-project";
    }

    // Interactive prompts
    std::string name, version, description, author, license;

    std::cout << "Project name: (" << projectName << ") ";
    std::getline(std::cin, name);
    if (name.empty()) name = projectName;

    std::cout << "Version: (1.0.0) ";
    std::getline(std::cin, version);
    if (version.empty()) version = "1.0.0";

    std::cout << "Description: ";
    std::getline(std::cin, description);

    std::cout << "Author: ";
    std::getline(std::cin, author);

    std::cout << "License: (MIT) ";
    std::getline(std::cin, license);
    if (license.empty()) license = "MIT";

    std::cout << std::endl;

    // Create package.json
    std::filesystem::path packageJsonPath = basePath / "package.json";
    std::ofstream pkgFile(packageJsonPath);

    pkgFile << "{\n";
    pkgFile << "  \"name\": \"" << name << "\",\n";
    pkgFile << "  \"version\": \"" << version << "\",\n";
    pkgFile << "  \"description\": \"" << description << "\",\n";

    if (withTypeScript) {
        pkgFile << "  \"main\": \"dist/index.js\",\n";
        pkgFile << "  \"types\": \"dist/index.d.ts\",\n";
    } else {
        pkgFile << "  \"main\": \"index.js\",\n";
    }

    pkgFile << "  \"scripts\": {\n";
    if (withTypeScript) {
        pkgFile << "    \"build\": \"nova build\",\n";
        pkgFile << "    \"start\": \"nova run src/index.ts\",\n";
    } else {
        pkgFile << "    \"start\": \"nova run index.js\",\n";
    }
    pkgFile << "    \"test\": \"nova test\"\n";
    pkgFile << "  },\n";

    if (!author.empty()) {
        pkgFile << "  \"author\": \"" << author << "\",\n";
    }
    pkgFile << "  \"license\": \"" << license << "\",\n";
    pkgFile << "  \"dependencies\": {},\n";
    pkgFile << "  \"devDependencies\": {}\n";
    pkgFile << "}\n";
    pkgFile.close();

    std::cout << "Created package.json" << std::endl;

    if (withTypeScript) {
        // Create tsconfig.json
        std::filesystem::path tsconfigPath = basePath / "tsconfig.json";
        std::ofstream tsFile(tsconfigPath);

        tsFile << "{\n";
        tsFile << "  \"compilerOptions\": {\n";
        tsFile << "    \"target\": \"ES2020\",\n";
        tsFile << "    \"module\": \"commonjs\",\n";
        tsFile << "    \"lib\": [\"ES2020\"],\n";
        tsFile << "    \"outDir\": \"./dist\",\n";
        tsFile << "    \"rootDir\": \"./src\",\n";
        tsFile << "    \"strict\": true,\n";
        tsFile << "    \"esModuleInterop\": true,\n";
        tsFile << "    \"skipLibCheck\": true,\n";
        tsFile << "    \"forceConsistentCasingInFileNames\": true,\n";
        tsFile << "    \"declaration\": true,\n";
        tsFile << "    \"declarationMap\": true,\n";
        tsFile << "    \"sourceMap\": true\n";
        tsFile << "  },\n";
        tsFile << "  \"include\": [\"src/**/*\"],\n";
        tsFile << "  \"exclude\": [\"node_modules\", \"dist\"]\n";
        tsFile << "}\n";
        tsFile.close();

        std::cout << "Created tsconfig.json" << std::endl;

        // Create src directory and index.ts
        std::filesystem::path srcDir = basePath / "src";
        std::filesystem::create_directories(srcDir);

        std::filesystem::path indexPath = srcDir / "index.ts";
        std::ofstream indexFile(indexPath);
        indexFile << "// " << name << "\n\n";
        indexFile << "function main(): void {\n";
        indexFile << "    console.log(\"Hello from " << name << "!\");\n";
        indexFile << "}\n\n";
        indexFile << "main();\n";
        indexFile.close();

        std::cout << "Created src/index.ts" << std::endl;
    } else {
        // Create index.js
        std::filesystem::path indexPath = basePath / "index.js";
        std::ofstream indexFile(indexPath);
        indexFile << "// " << name << "\n\n";
        indexFile << "function main() {\n";
        indexFile << "    console.log(\"Hello from " << name << "!\");\n";
        indexFile << "}\n\n";
        indexFile << "main();\n";
        indexFile.close();

        std::cout << "Created index.js" << std::endl;
    }

    std::cout << std::endl;
    std::cout << "[nova] Project initialized successfully!" << std::endl;
    std::cout << std::endl;
    std::cout << "Next steps:" << std::endl;
    if (withTypeScript) {
        std::cout << "  nova run src/index.ts   # Run the project" << std::endl;
        std::cout << "  nova build              # Build to JavaScript" << std::endl;
    } else {
        std::cout << "  nova run index.js       # Run the project" << std::endl;
    }
    std::cout << "  nova install <package>  # Install dependencies" << std::endl;

    return true;
}

// Run automated tests
int PackageManager::runTests(const std::string& projectPath, const std::string& pattern) {
    projectPath_ = projectPath;

    std::cout << "[nova] Running tests..." << std::endl;
    std::cout << std::endl;

    // Find test files
    std::vector<std::string> testFiles;
    std::filesystem::path basePath = std::filesystem::absolute(projectPath);

    // Test file patterns to look for
    std::vector<std::string> testPatterns = {
        "*.test.ts", "*.spec.ts", "*.test.js", "*.spec.js",
        "*_test.ts", "*_spec.ts", "*_test.js", "*_spec.js"
    };

    // Test directories to search
    std::vector<std::string> testDirs = {
        "tests", "test", "__tests__", "spec", "specs", "src"
    };

    // Helper to check if file matches test pattern
    auto isTestFile = [&](const std::string& filename) -> bool {
        // If specific pattern is provided, match against it
        if (!pattern.empty()) {
            if (filename.find(pattern) != std::string::npos) {
                return true;
            }
            return false;
        }

        // Check against test patterns
        for (const auto& pat : testPatterns) {
            // Simple wildcard matching for *.test.ts etc.
            if (pat[0] == '*') {
                std::string suffix = pat.substr(1);
                if (filename.size() >= suffix.size() &&
                    filename.substr(filename.size() - suffix.size()) == suffix) {
                    return true;
                }
            }
        }
        return false;
    };

    // Search for test files
    for (const auto& dir : testDirs) {
        std::filesystem::path testDir = basePath / dir;
        if (!std::filesystem::exists(testDir)) continue;

        for (const auto& entry : std::filesystem::recursive_directory_iterator(testDir)) {
            if (!entry.is_regular_file()) continue;

            std::string filename = entry.path().filename().string();
            if (isTestFile(filename)) {
                std::filesystem::path relativePath = std::filesystem::relative(entry.path(), basePath);
                testFiles.push_back(relativePath.string());
            }
        }
    }

    // Also search root directory for test files
    for (const auto& entry : std::filesystem::directory_iterator(basePath)) {
        if (!entry.is_regular_file()) continue;

        std::string filename = entry.path().filename().string();
        if (isTestFile(filename)) {
            std::filesystem::path relativePath = std::filesystem::relative(entry.path(), basePath);
            testFiles.push_back(relativePath.string());
        }
    }

    if (testFiles.empty()) {
        std::cout << "[nova] No test files found." << std::endl;
        std::cout << std::endl;
        std::cout << "Test file patterns:" << std::endl;
        std::cout << "  *.test.ts, *.spec.ts, *.test.js, *.spec.js" << std::endl;
        std::cout << "  *_test.ts, *_spec.ts, *_test.js, *_spec.js" << std::endl;
        std::cout << std::endl;
        std::cout << "Test directories:" << std::endl;
        std::cout << "  tests/, test/, __tests__/, spec/, src/" << std::endl;
        return 0;
    }

    std::cout << "[nova] Found " << testFiles.size() << " test file(s)" << std::endl;
    std::cout << std::endl;

    // Test results tracking
    int totalTests = 0;
    int passedTests = 0;
    int failedTests = 0;
    std::vector<std::string> failedTestFiles;
    auto startTime = std::chrono::high_resolution_clock::now();

    // Run each test file
    for (const auto& testFile : testFiles) {
        std::cout << "  " << testFile << " ";

        std::filesystem::path fullPath = basePath / testFile;
        std::string ext = fullPath.extension().string();

        int exitCode = 0;
        bool isTypeScript = (ext == ".ts");

        // For TypeScript files, compile and run with nova
        // For JavaScript files, run with node
        std::string cmd;

#ifdef _WIN32
        if (isTypeScript) {
            // Use nova to run TypeScript - get current executable path
            char exePath[MAX_PATH];
            GetModuleFileNameA(nullptr, exePath, MAX_PATH);
            std::string novaPath = exePath;
            std::string pathStr = fullPath.string();
            // Use cmd /c to properly handle Windows paths with spaces
            cmd = "cmd /c \"\"" + novaPath + "\" run \"" + pathStr + "\"\" 2>&1";
        } else {
            std::string pathStr = fullPath.string();
            cmd = "cmd /c \"node \"" + pathStr + "\"\" 2>&1";
        }
#else
        if (isTypeScript) {
            cmd = "nova run \"" + fullPath.string() + "\" 2>&1";
        } else {
            cmd = "node \"" + fullPath.string() + "\" 2>&1";
        }
#endif

        // Execute test and capture output
        std::string output;
        FILE* pipe = nullptr;

#ifdef _WIN32
        pipe = _popen(cmd.c_str(), "r");
#else
        pipe = popen(cmd.c_str(), "r");
#endif

        if (pipe) {
            char buffer[256];
            while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                output += buffer;
            }
#ifdef _WIN32
            exitCode = _pclose(pipe);
#else
            exitCode = pclose(pipe);
            exitCode = WEXITSTATUS(exitCode);
#endif
        } else {
            exitCode = -1;
        }

        totalTests++;

        if (exitCode == 0) {
            passedTests++;
            std::cout << "\033[32mPASS\033[0m" << std::endl;
        } else {
            failedTests++;
            failedTestFiles.push_back(testFile);
            std::cout << "\033[31mFAIL\033[0m" << std::endl;

            // Show error output (indented)
            if (!output.empty()) {
                std::istringstream iss(output);
                std::string line;
                while (std::getline(iss, line)) {
                    std::cout << "    " << line << std::endl;
                }
            }
        }
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

    // Print summary
    std::cout << std::endl;
    std::cout << "--------------------------------------------------" << std::endl;
    std::cout << "Test Results:" << std::endl;
    std::cout << std::endl;

    if (passedTests > 0) {
        std::cout << "  \033[32m" << passedTests << " passed\033[0m" << std::endl;
    }
    if (failedTests > 0) {
        std::cout << "  \033[31m" << failedTests << " failed\033[0m" << std::endl;
    }
    std::cout << "  " << totalTests << " total" << std::endl;
    std::cout << std::endl;
    std::cout << "Time: " << duration.count() << "ms" << std::endl;

    // List failed tests
    if (!failedTestFiles.empty()) {
        std::cout << std::endl;
        std::cout << "Failed tests:" << std::endl;
        for (const auto& f : failedTestFiles) {
            std::cout << "  - " << f << std::endl;
        }
    }

    std::cout << std::endl;
    if (failedTests > 0) {
        std::cout << "\033[31m[nova] Tests failed!\033[0m" << std::endl;
        return 1;
    } else {
        std::cout << "\033[32m[nova] All tests passed!\033[0m" << std::endl;
        return 0;
    }
}

// Run npm script from package.json
int PackageManager::runScript(const std::string& scriptName, const std::string& projectPath) {
    projectPath_ = projectPath;
    std::filesystem::path basePath = std::filesystem::absolute(projectPath);
    std::filesystem::path packageJsonPath = basePath / "package.json";

    // Check if package.json exists
    if (!std::filesystem::exists(packageJsonPath)) {
        std::cerr << "[nova] Error: package.json not found in " << projectPath << std::endl;
        return 1;
    }

    // Read package.json
    std::ifstream packageJsonFile(packageJsonPath);
    if (!packageJsonFile.is_open()) {
        std::cerr << "[nova] Error: Failed to open package.json" << std::endl;
        return 1;
    }

    std::string packageJson((std::istreambuf_iterator<char>(packageJsonFile)),
                           std::istreambuf_iterator<char>());
    packageJsonFile.close();

    // Find scripts section
    size_t scriptsPos = packageJson.find("\"scripts\"");
    if (scriptsPos == std::string::npos) {
        std::cerr << "[nova] Error: No scripts found in package.json" << std::endl;
        return 1;
    }

    // Find the script
    std::string searchKey = "\"" + scriptName + "\"";
    size_t scriptKeyPos = packageJson.find(searchKey, scriptsPos);
    if (scriptKeyPos == std::string::npos) {
        std::cerr << "[nova] Error: Script '" << scriptName << "' not found in package.json" << std::endl;
        std::cerr << std::endl;
        std::cerr << "Available scripts:" << std::endl;

        // Try to show available scripts
        size_t scriptStart = packageJson.find('{', scriptsPos);
        size_t scriptEnd = packageJson.find('}', scriptStart);
        if (scriptStart != std::string::npos && scriptEnd != std::string::npos) {
            std::string scriptsSection = packageJson.substr(scriptStart, scriptEnd - scriptStart);
            size_t pos = 0;
            while ((pos = scriptsSection.find('"', pos)) != std::string::npos) {
                size_t endQuote = scriptsSection.find('"', pos + 1);
                if (endQuote != std::string::npos) {
                    std::string name = scriptsSection.substr(pos + 1, endQuote - pos - 1);
                    if (!name.empty() && name != "scripts") {
                        std::cerr << "  - " << name << std::endl;
                    }
                    pos = endQuote + 1;
                    // Skip to next line/script
                    pos = scriptsSection.find('"', pos);
                    if (pos == std::string::npos) break;
                } else {
                    break;
                }
            }
        }
        return 1;
    }

    // Extract script command
    size_t colonPos = packageJson.find(':', scriptKeyPos);
    if (colonPos == std::string::npos) {
        std::cerr << "[nova] Error: Malformed package.json" << std::endl;
        return 1;
    }

    size_t startQuote = packageJson.find('"', colonPos);
    if (startQuote == std::string::npos) {
        std::cerr << "[nova] Error: Malformed script command" << std::endl;
        return 1;
    }

    size_t endQuote = packageJson.find('"', startQuote + 1);
    if (endQuote == std::string::npos) {
        std::cerr << "[nova] Error: Malformed script command" << std::endl;
        return 1;
    }

    std::string scriptCommand = packageJson.substr(startQuote + 1, endQuote - startQuote - 1);

    // Print what we're running
    std::cout << "[nova] Running script: " << scriptName << std::endl;
    std::cout << "[nova] Command: " << scriptCommand << std::endl;
    std::cout << std::endl;

    // Execute the script
#ifdef _WIN32
    std::string fullCommand = "cmd /c \"cd /d \"" + basePath.string() + "\" && " + scriptCommand + "\"";
#else
    std::string fullCommand = "cd " + basePath.string() + " && " + scriptCommand;
#endif

    int result = std::system(fullCommand.c_str());

    std::cout << std::endl;
    if (result == 0) {
        std::cout << "\033[32m[nova] Script '" << scriptName << "' completed successfully\033[0m" << std::endl;
    } else {
        std::cout << "\033[31m[nova] Script '" << scriptName << "' failed with code " << result << "\033[0m" << std::endl;
    }

    return result;
}

// HTTP GET request
std::string PackageManager::httpGet(const std::string& url) {
#ifdef _WIN32
    std::string result;

    HINTERNET hInternet = InternetOpenA("Nova Package Manager", INTERNET_OPEN_TYPE_PRECONFIG, nullptr, nullptr, 0);
    if (!hInternet) return "";

    HINTERNET hConnect = InternetOpenUrlA(hInternet, url.c_str(), nullptr, 0,
        INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE, 0);
    if (!hConnect) {
        InternetCloseHandle(hInternet);
        return "";
    }

    char buffer[4096];
    DWORD bytesRead;
    while (InternetReadFile(hConnect, buffer, sizeof(buffer) - 1, &bytesRead) && bytesRead > 0) {
        buffer[bytesRead] = '\0';
        result += buffer;
    }

    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);

    return result;
#else
    // Use curl on Unix
    std::string cmd = "curl -sL \"" + url + "\" 2>/dev/null";
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return "";

    std::string result;
    char buffer[4096];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }
    pclose(pipe);
    return result;
#endif
}

// HTTP download file
bool PackageManager::httpDownload(const std::string& url, const std::string& destPath) {
#ifdef _WIN32
    // Create directory if needed
    std::filesystem::path destDir = std::filesystem::path(destPath).parent_path();
    std::filesystem::create_directories(destDir);

    HINTERNET hInternet = InternetOpenA("Nova Package Manager", INTERNET_OPEN_TYPE_PRECONFIG, nullptr, nullptr, 0);
    if (!hInternet) return false;

    HINTERNET hConnect = InternetOpenUrlA(hInternet, url.c_str(), nullptr, 0,
        INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE, 0);
    if (!hConnect) {
        InternetCloseHandle(hInternet);
        return false;
    }

    std::ofstream outFile(destPath, std::ios::binary);
    if (!outFile.is_open()) {
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        return false;
    }

    char buffer[8192];
    DWORD bytesRead;
    while (InternetReadFile(hConnect, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
        outFile.write(buffer, bytesRead);
    }

    outFile.close();
    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);

    return true;
#else
    std::filesystem::path destDir = std::filesystem::path(destPath).parent_path();
    std::filesystem::create_directories(destDir);

    std::string cmd = "curl -sL \"" + url + "\" -o \"" + destPath + "\" 2>/dev/null";
    return system(cmd.c_str()) == 0;
#endif
}

// Extract tar.gz file
bool PackageManager::extractTarGz(const std::string& tarPath, const std::string& destDir) {
    std::filesystem::create_directories(destDir);

#ifdef _WIN32
    // Use Windows native tar from System32 to avoid MSYS path issues
    // MSYS tar interprets C: as a remote URL scheme
    std::string winTarPath = tarPath;
    std::string winDestDir = destDir;

    // Convert forward slashes to backslashes for Windows native tar
    std::replace(winTarPath.begin(), winTarPath.end(), '/', '\\');
    std::replace(winDestDir.begin(), winDestDir.end(), '/', '\\');

    std::string cmd = "C:\\Windows\\System32\\tar.exe -xzf \"" + winTarPath + "\" -C \"" + winDestDir + "\" --strip-components=1 2>nul";
    return system(cmd.c_str()) == 0;
#else
    std::string cmd = "tar -xzf \"" + tarPath + "\" -C \"" + destDir + "\" --strip-components=1 2>/dev/null";
    return system(cmd.c_str()) == 0;
#endif
}

// Get cache path for a package
std::string PackageManager::getCachePath(const std::string& packageName, const std::string& tag, const std::string& version) {
    std::filesystem::path cachePath = std::filesystem::path(cacheDir_) / packageName / tag / (version + ".tar.gz");
    return cachePath.string();
}

// Check if package is in cache
bool PackageManager::isInCache(const std::string& packageName, const std::string& tag, const std::string& version) {
    return std::filesystem::exists(getCachePath(packageName, tag, version));
}

// Get tag from version (latest, beta, next, etc.)
std::string PackageManager::getTagFromVersion(const std::string& version) {
    if (version == "latest" || version.empty()) return "latest";
    if (version == "next") return "next";
    if (version == "beta") return "beta";
    if (version == "alpha") return "alpha";
    if (version.find("beta") != std::string::npos) return "beta";
    if (version.find("alpha") != std::string::npos) return "alpha";
    if (version.find("rc") != std::string::npos) return "next";
    return "latest";
}

// Get latest version of a package
std::string PackageManager::getLatestVersion(const std::string& packageName) {
    // Get the appropriate registry for this package (handles scoped packages)
    std::string packageRegistry = getRegistryForPackage(packageName);
    std::string url = packageRegistry + "/" + packageName + "/latest";
    std::string response = httpGet(url);
    if (response.empty()) return "";
    return getJsonString(response, "version");
}

// Resolve package from registry
PackageInfo PackageManager::resolvePackage(const std::string& packageName, const std::string& versionRange) {
    PackageInfo info;
    info.name = packageName;
    info.version = versionRange;

    // Get the appropriate registry for this package (handles scoped packages)
    std::string packageRegistry = getRegistryForPackage(packageName);

    std::string tag = getTagFromVersion(versionRange);
    std::string url = packageRegistry + "/" + packageName + "/" + (versionRange.empty() ? "latest" : versionRange);

    std::string response = httpGet(url);
    if (response.empty()) {
        // Try latest
        url = packageRegistry + "/" + packageName + "/latest";
        response = httpGet(url);
    }

    if (!response.empty()) {
        info.resolvedVersion = getJsonString(response, "version");

        // Parse tarball URL from dist
        size_t distPos = response.find("\"dist\"");
        if (distPos != std::string::npos) {
            info.tarballUrl = getJsonString(response.substr(distPos), "tarball");
            info.integrity = getJsonString(response.substr(distPos), "integrity");
        }
    }

    return info;
}

// Download a package
bool PackageManager::downloadPackage(const PackageInfo& pkg) {
    if (pkg.tarballUrl.empty()) return false;

    std::string tag = getTagFromVersion(pkg.version);
    std::string cachePath = getCachePath(pkg.name, tag, pkg.resolvedVersion);

    return httpDownload(pkg.tarballUrl, cachePath);
}

// Extract package to destination
bool PackageManager::extractPackage(const std::string& tarballPath, const std::string& destPath) {
    return extractTarGz(tarballPath, destPath);
}

// Ping download stats (for cache hits)
bool PackageManager::pingDownloadStats(const std::string& packageName, const std::string& version) {
    // Just do a HEAD request to increment download count
    std::string url = registry_ + "/" + packageName + "/" + version;
    httpGet(url);  // This should increment the download counter
    return true;
}

// Check if version satisfies range
bool PackageManager::satisfiesVersion(const std::string& version, const std::string& range) {
    // Simple version matching for now
    if (range.empty() || range == "*" || range == "latest") return true;
    if (range[0] == '^' || range[0] == '~') {
        // Major version match for ^, minor for ~
        return version.find(range.substr(1, range.find('.') - 1)) == 0;
    }
    return version == range;
}

// Parse dependencies from package.json
std::map<std::string, std::string> PackageManager::parseDependencies(const std::string& packageJsonPath, bool dev) {
    std::map<std::string, std::string> deps;

    std::ifstream file(packageJsonPath);
    if (!file.is_open()) return deps;

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();

    std::string depsKey = dev ? "\"devDependencies\"" : "\"dependencies\"";
    size_t depsPos = content.find(depsKey);
    if (depsPos == std::string::npos) return deps;

    size_t braceStart = content.find('{', depsPos);
    if (braceStart == std::string::npos) return deps;

    size_t braceEnd = content.find('}', braceStart);
    if (braceEnd == std::string::npos) return deps;

    std::string depsSection = content.substr(braceStart + 1, braceEnd - braceStart - 1);

    // Parse key-value pairs
    std::regex depRegex("\"([^\"]+)\"\\s*:\\s*\"([^\"]+)\"");
    std::smatch match;
    std::string::const_iterator searchStart(depsSection.cbegin());

    while (std::regex_search(searchStart, depsSection.cend(), match, depRegex)) {
        deps[match[1].str()] = match[2].str();
        searchStart = match.suffix().first;
    }

    return deps;
}

// Parse package-lock.json
std::vector<PackageInfo> PackageManager::parseLockfile(const std::string& lockfilePath) {
    std::vector<PackageInfo> packages;

    std::ifstream file(lockfilePath);
    if (!file.is_open()) return packages;

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();

    // Find packages section
    size_t packagesPos = content.find("\"packages\"");
    if (packagesPos == std::string::npos) {
        // Try v1 format with "dependencies"
        packagesPos = content.find("\"dependencies\"");
    }

    if (packagesPos == std::string::npos) return packages;

    // Parse each package entry
    std::regex pkgRegex("\"node_modules/([^\"]+)\"\\s*:\\s*\\{[^}]*\"version\"\\s*:\\s*\"([^\"]+)\"[^}]*\"resolved\"\\s*:\\s*\"([^\"]+)\"");
    std::smatch match;
    std::string::const_iterator searchStart(content.cbegin());

    while (std::regex_search(searchStart, content.cend(), match, pkgRegex)) {
        PackageInfo pkg;
        pkg.name = match[1].str();
        pkg.resolvedVersion = match[2].str();
        pkg.tarballUrl = match[3].str();
        packages.push_back(pkg);
        searchStart = match.suffix().first;
    }

    return packages;
}

// Build dependency tree
std::vector<PackageInfo> PackageManager::buildDependencyTree(const std::map<std::string, std::string>& deps) {
    std::vector<PackageInfo> packages;

    for (const auto& [name, version] : deps) {
        PackageInfo pkg = resolvePackage(name, version);
        if (!pkg.resolvedVersion.empty()) {
            packages.push_back(pkg);
        }
    }

    return packages;
}

// Install packages
InstallResult PackageManager::install(const std::string& projectPath, bool devDependencies) {
    InstallResult result = {true, 0, 0, 0, 0, 0.0, 0, {}, {}};
    projectPath_ = projectPath;

    // Load .npmrc from project path
    loadNpmrc(projectPath);

    auto startTime = std::chrono::high_resolution_clock::now();

    std::filesystem::path basePath = std::filesystem::absolute(projectPath);
    std::filesystem::path packageJsonPath = basePath / "package.json";

    if (!std::filesystem::exists(packageJsonPath)) {
        result.success = false;
        result.errors.push_back("package.json not found");
        return result;
    }

    // Parse dependencies
    auto deps = parseDependencies(packageJsonPath.string(), false);
    auto devDeps = devDependencies ? parseDependencies(packageJsonPath.string(), true) : std::map<std::string, std::string>();

    // Merge deps
    for (const auto& [name, version] : devDeps) {
        deps[name] = version;
    }

    if (deps.empty()) {
        std::cout << "[nova] No dependencies to install" << std::endl;
        return result;
    }

    // Check for lockfile
    std::filesystem::path lockfilePath = basePath / "package-lock.json";
    std::vector<PackageInfo> packages;

    if (std::filesystem::exists(lockfilePath)) {
        packages = parseLockfile(lockfilePath.string());
    }

    if (packages.empty()) {
        packages = buildDependencyTree(deps);
    }

    result.totalPackages = packages.size();

    // Create node_modules
    std::filesystem::path nodeModulesPath = basePath / "node_modules";
    std::filesystem::create_directories(nodeModulesPath);

    int current = 0;
    for (auto& pkg : packages) {
        current++;

        std::string tag = getTagFromVersion(pkg.version);
        std::string cachePath = getCachePath(pkg.name, tag, pkg.resolvedVersion);
        bool fromCache = std::filesystem::exists(cachePath);

        if (progressCallback_) {
            progressCallback_(pkg.name, current, result.totalPackages, fromCache);
        }

        if (fromCache) {
            result.cachedPackages++;
            // Ping stats for cache hit
            pingDownloadStats(pkg.name, pkg.resolvedVersion);
            result.apiOnlyPackages++;
        } else {
            // Download
            if (downloadPackage(pkg)) {
                result.downloadedPackages++;
            } else {
                result.errors.push_back("Failed to download: " + pkg.name);
                continue;
            }
        }

        // Extract to node_modules
        std::filesystem::path destPath = nodeModulesPath / pkg.name;
        if (!extractPackage(cachePath, destPath.string())) {
            result.errors.push_back("Failed to extract: " + pkg.name);
        }

        // Update size
        if (std::filesystem::exists(cachePath)) {
            result.totalSizeBytes += std::filesystem::file_size(cachePath);
        }
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    result.totalTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

    std::cout << std::endl;

    if (!result.errors.empty()) {
        result.success = false;
    }

    return result;
}

// Install specific package
InstallResult PackageManager::installPackage(const std::string& packageName, const std::string& version) {
    InstallResult result = {true, 1, 0, 0, 0, 0.0, 0, {}, {}};

    auto startTime = std::chrono::high_resolution_clock::now();

    PackageInfo pkg = resolvePackage(packageName, version);
    if (pkg.resolvedVersion.empty()) {
        result.success = false;
        result.errors.push_back("Package not found: " + packageName);
        return result;
    }

    std::string tag = getTagFromVersion(version);
    std::string cachePath = getCachePath(packageName, tag, pkg.resolvedVersion);

    if (std::filesystem::exists(cachePath)) {
        result.cachedPackages = 1;
    } else {
        if (downloadPackage(pkg)) {
            result.downloadedPackages = 1;
        } else {
            result.success = false;
            result.errors.push_back("Failed to download: " + packageName);
            return result;
        }
    }

    // Extract
    std::filesystem::path destPath = std::filesystem::absolute(projectPath_) / "node_modules" / packageName;
    std::filesystem::create_directories(destPath.parent_path());

    if (!extractPackage(cachePath, destPath.string())) {
        result.success = false;
        result.errors.push_back("Failed to extract: " + packageName);
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    result.totalTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

    return result;
}

// Add package
InstallResult PackageManager::add(const std::string& packageName, const std::string& version, DependencyType depType) {
    // Handle global installation
    if (depType == DependencyType::Global) {
        return installGlobal(packageName, version);
    }

    // Load .npmrc from project path
    loadNpmrc(projectPath_);

    InstallResult result = installPackage(packageName, version);

    if (!result.success) return result;

    // Update package.json
    std::filesystem::path packageJsonPath = std::filesystem::absolute(projectPath_) / "package.json";

    std::ifstream inFile(packageJsonPath);
    if (!inFile.is_open()) {
        result.errors.push_back("Could not open package.json");
        return result;
    }

    std::stringstream buffer;
    buffer << inFile.rdbuf();
    std::string content = buffer.str();
    inFile.close();

    // Get resolved version
    PackageInfo pkg = resolvePackage(packageName, version);
    std::string versionToAdd = "^" + pkg.resolvedVersion;

    // Get the right dependency key
    std::string depsKeyName = getDependencyKeyFromType(depType);
    std::string depsKey = "\"" + depsKeyName + "\"";
    size_t depsPos = content.find(depsKey);

    if (depsPos != std::string::npos) {
        // Find the opening brace
        size_t bracePos = content.find('{', depsPos);
        if (bracePos != std::string::npos) {
            // Check if deps is empty
            size_t nextChar = content.find_first_not_of(" \n\r\t", bracePos + 1);
            std::string newEntry = "\"" + packageName + "\": \"" + versionToAdd + "\"";

            if (content[nextChar] == '}') {
                // Empty deps
                content.insert(bracePos + 1, "\n    " + newEntry + "\n  ");
            } else {
                // Add to existing deps
                content.insert(bracePos + 1, "\n    " + newEntry + ",");
            }
        }
    } else {
        // Dependencies section doesn't exist, create it
        // Find position before last closing brace
        size_t lastBrace = content.rfind('}');
        if (lastBrace != std::string::npos) {
            // Find the last non-whitespace character before the closing brace
            size_t prevContent = content.find_last_not_of(" \n\r\t", lastBrace - 1);

            std::string newSection;
            if (prevContent != std::string::npos && content[prevContent] != '{' && content[prevContent] != ',') {
                // Need to add comma after previous content - insert comma right after prevContent
                content.insert(prevContent + 1, ",");
                // Now find lastBrace again since we modified content
                lastBrace = content.rfind('}');
                // Remove whitespace between comma and lastBrace to avoid extra blank lines
                size_t wsStart = prevContent + 2; // After the comma we just inserted
                if (wsStart < lastBrace) {
                    content.erase(wsStart, lastBrace - wsStart);
                    lastBrace = content.rfind('}');
                }
            }
            newSection = "\n  " + depsKey + ": {\n    \"" + packageName + "\": \"" + versionToAdd + "\"\n  }\n";
            content.insert(lastBrace, newSection);
        }
    }

    std::ofstream outFile(packageJsonPath);
    outFile << content;
    outFile.close();

    return result;
}

// Install package globally
InstallResult PackageManager::installGlobal(const std::string& packageName, const std::string& version) {
    InstallResult result = {true, 1, 0, 0, 0, 0.0, 0, {}, {}};

    auto startTime = std::chrono::high_resolution_clock::now();

    PackageInfo pkg = resolvePackage(packageName, version);
    if (pkg.resolvedVersion.empty()) {
        result.success = false;
        result.errors.push_back("Package not found: " + packageName);
        return result;
    }

    std::string tag = getTagFromVersion(version);
    std::string cachePath = getCachePath(packageName, tag, pkg.resolvedVersion);

    if (std::filesystem::exists(cachePath)) {
        result.cachedPackages = 1;
    } else {
        if (downloadPackage(pkg)) {
            result.downloadedPackages = 1;
        } else {
            result.success = false;
            result.errors.push_back("Failed to download: " + packageName);
            return result;
        }
    }

    // Extract to global directory
    std::filesystem::path globalDir = getGlobalDir();
    std::filesystem::path destPath = globalDir / "node_modules" / packageName;
    std::filesystem::create_directories(destPath.parent_path());

    if (!extractPackage(cachePath, destPath.string())) {
        result.success = false;
        result.errors.push_back("Failed to extract: " + packageName);
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    result.totalTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

    return result;
}

// Remove global package
bool PackageManager::removeGlobal(const std::string& packageName) {
    std::filesystem::path globalDir = getGlobalDir();
    std::filesystem::path packagePath = globalDir / "node_modules" / packageName;

    if (std::filesystem::exists(packagePath)) {
        std::filesystem::remove_all(packagePath);
        std::cout << "[nova] Removed " << packageName << " from global packages" << std::endl;
        return true;
    } else {
        std::cerr << "[nova] Package " << packageName << " is not installed globally" << std::endl;
        return false;
    }
}

// Remove package
bool PackageManager::remove(const std::string& packageName, DependencyType depType) {
    // Handle global uninstall
    if (depType == DependencyType::Global) {
        return removeGlobal(packageName);
    }

    std::filesystem::path basePath = std::filesystem::absolute(projectPath_);
    std::filesystem::path packagePath = basePath / "node_modules" / packageName;

    // Remove from node_modules
    if (std::filesystem::exists(packagePath)) {
        std::filesystem::remove_all(packagePath);
    }

    // Update package.json
    std::filesystem::path packageJsonPath = basePath / "package.json";

    std::ifstream inFile(packageJsonPath);
    if (!inFile.is_open()) return false;

    std::stringstream buffer;
    buffer << inFile.rdbuf();
    std::string content = buffer.str();
    inFile.close();

    // Remove from dependencies and devDependencies (search all sections)
    std::regex depRegex("\\s*\"" + packageName + "\"\\s*:\\s*\"[^\"]+\"\\s*,?");
    content = std::regex_replace(content, depRegex, "");

    std::ofstream outFile(packageJsonPath);
    outFile << content;
    outFile.close();

    std::cout << "[nova] Removed " << packageName << std::endl;
    return true;
}

// Update packages
InstallResult PackageManager::update(const std::string& packageName, DependencyType depType) {
    if (packageName.empty()) {
        // Update all packages
        return install(projectPath_, true);
    }

    // Handle global update
    if (depType == DependencyType::Global) {
        return installGlobal(packageName, "latest");
    }

    // Update specific package - reinstall with same dependency type
    return add(packageName, "latest", depType);
}

// Clean install from lockfile
InstallResult PackageManager::cleanInstall(const std::string& projectPath) {
    InstallResult result = {true, 0, 0, 0, 0, 0.0, 0, {}, {}};
    projectPath_ = projectPath;

    std::filesystem::path basePath = std::filesystem::absolute(projectPath);
    std::filesystem::path lockfilePath = basePath / "package-lock.json";

    if (!std::filesystem::exists(lockfilePath)) {
        std::cerr << "[nova] Error: package-lock.json not found" << std::endl;
        std::cerr << "[nova] Run 'nova install' first to generate lockfile" << std::endl;
        result.success = false;
        result.errors.push_back("package-lock.json not found");
        return result;
    }

    // Remove node_modules
    std::filesystem::path nodeModulesPath = basePath / "node_modules";
    if (std::filesystem::exists(nodeModulesPath)) {
        std::cout << "[nova] Removing node_modules..." << std::endl;
        std::filesystem::remove_all(nodeModulesPath);
    }

    // Install from lockfile only
    std::cout << "[nova] Installing from lockfile..." << std::endl;
    return install(projectPath, true);
}

// Link current package globally
bool PackageManager::link(const std::string& projectPath) {
    projectPath_ = projectPath;
    std::filesystem::path basePath = std::filesystem::absolute(projectPath);
    std::filesystem::path packageJsonPath = basePath / "package.json";

    if (!std::filesystem::exists(packageJsonPath)) {
        std::cerr << "[nova] Error: package.json not found" << std::endl;
        return false;
    }

    // Read package name
    std::ifstream file(packageJsonPath);
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string packageName = getJsonString(buffer.str(), "name");

    if (packageName.empty()) {
        std::cerr << "[nova] Error: Could not find package name in package.json" << std::endl;
        return false;
    }

    // Global link directory
#ifdef _WIN32
    const char* localAppData = std::getenv("LOCALAPPDATA");
    std::filesystem::path globalLinkDir = std::string(localAppData ? localAppData : "C:\\") + "\\nova\\global-links";
#else
    const char* home = std::getenv("HOME");
    std::filesystem::path globalLinkDir = std::string(home ? home : "/tmp") + "/.nova/global-links";
#endif

    std::filesystem::create_directories(globalLinkDir);

    std::filesystem::path linkPath = globalLinkDir / packageName;

    // Remove existing link
    if (std::filesystem::exists(linkPath)) {
        std::filesystem::remove_all(linkPath);
    }

    // Create symlink
#ifdef _WIN32
    // Use junction on Windows
    std::string cmd = "mklink /J \"" + linkPath.string() + "\" \"" + basePath.string() + "\" >nul 2>&1";
    if (system(cmd.c_str()) != 0) {
        std::cerr << "[nova] Error: Failed to create link (try running as administrator)" << std::endl;
        return false;
    }
#else
    std::filesystem::create_directory_symlink(basePath, linkPath);
#endif

    std::cout << "[nova] Linked " << packageName << " globally" << std::endl;
    std::cout << "[nova] " << basePath.string() << " -> " << linkPath.string() << std::endl;

    return true;
}

// Link a global package to current project
bool PackageManager::linkPackage(const std::string& packageName) {
#ifdef _WIN32
    const char* localAppData = std::getenv("LOCALAPPDATA");
    std::filesystem::path globalLinkDir = std::string(localAppData ? localAppData : "C:\\") + "\\nova\\global-links";
#else
    const char* home = std::getenv("HOME");
    std::filesystem::path globalLinkDir = std::string(home ? home : "/tmp") + "/.nova/global-links";
#endif

    std::filesystem::path sourcePath = globalLinkDir / packageName;

    if (!std::filesystem::exists(sourcePath)) {
        std::cerr << "[nova] Error: Package '" << packageName << "' is not linked globally" << std::endl;
        std::cerr << "[nova] Run 'nova link' in the package directory first" << std::endl;
        return false;
    }

    std::filesystem::path basePath = std::filesystem::absolute(projectPath_);
    std::filesystem::path nodeModulesPath = basePath / "node_modules";
    std::filesystem::create_directories(nodeModulesPath);

    std::filesystem::path destPath = nodeModulesPath / packageName;

    // Remove existing
    if (std::filesystem::exists(destPath)) {
        std::filesystem::remove_all(destPath);
    }

    // Create symlink
#ifdef _WIN32
    std::string cmd = "mklink /J \"" + destPath.string() + "\" \"" + sourcePath.string() + "\" >nul 2>&1";
    if (system(cmd.c_str()) != 0) {
        std::cerr << "[nova] Error: Failed to create link" << std::endl;
        return false;
    }
#else
    std::filesystem::create_directory_symlink(sourcePath, destPath);
#endif

    std::cout << "[nova] Linked " << packageName << " to project" << std::endl;

    return true;
}

// List installed packages
std::vector<PackageInfo> PackageManager::list([[maybe_unused]] bool includeTransitive) {
    std::vector<PackageInfo> packages;

    std::filesystem::path basePath = std::filesystem::absolute(projectPath_);
    std::filesystem::path nodeModulesPath = basePath / "node_modules";

    if (!std::filesystem::exists(nodeModulesPath)) return packages;

    for (const auto& entry : std::filesystem::directory_iterator(nodeModulesPath)) {
        if (!entry.is_directory()) continue;

        std::string name = entry.path().filename().string();
        if (name[0] == '.') continue;

        PackageInfo pkg;
        pkg.name = name;

        // Read version from package.json
        std::filesystem::path pkgJsonPath = entry.path() / "package.json";
        if (std::filesystem::exists(pkgJsonPath)) {
            std::ifstream file(pkgJsonPath);
            std::stringstream buffer;
            buffer << file.rdbuf();
            pkg.version = getJsonString(buffer.str(), "version");
        }

        packages.push_back(pkg);
    }

    return packages;
}

// Display dependency tree
void PackageManager::listDependencies(const std::string& projectPath) {
    projectPath_ = projectPath;
    std::filesystem::path basePath = std::filesystem::absolute(projectPath);
    std::filesystem::path packageJsonPath = basePath / "package.json";

    if (!std::filesystem::exists(packageJsonPath)) {
        std::cerr << "[nova] Error: package.json not found" << std::endl;
        return;
    }

    // Read package.json
    std::ifstream file(packageJsonPath);
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();

    std::string projectName = getJsonString(content, "name");
    std::string projectVersion = getJsonString(content, "version");

    std::cout << projectName << "@" << projectVersion << " " << basePath.string() << std::endl;

    auto deps = parseDependencies(packageJsonPath.string(), false);
    auto devDeps = parseDependencies(packageJsonPath.string(), true);

    std::filesystem::path nodeModulesPath = basePath / "node_modules";

    auto printDep = [&](const std::string& name, const std::string& version, bool isDev, bool isLast) {
        std::string prefix = isLast ? "└── " : "├── ";
        std::string installedVersion = "";
        bool isLinked = false;
        bool isMissing = false;

        std::filesystem::path pkgPath = nodeModulesPath / name;
        if (std::filesystem::exists(pkgPath)) {
            // Check if it's a symlink
            if (std::filesystem::is_symlink(pkgPath)) {
                isLinked = true;
            }

            // Read installed version
            std::filesystem::path pkgJsonPath = pkgPath / "package.json";
            if (std::filesystem::exists(pkgJsonPath)) {
                std::ifstream pkgFile(pkgJsonPath);
                std::stringstream pkgBuffer;
                pkgBuffer << pkgFile.rdbuf();
                installedVersion = getJsonString(pkgBuffer.str(), "version");
            }
        } else {
            isMissing = true;
        }

        std::cout << prefix << name << "@" << (installedVersion.empty() ? version : installedVersion);

        if (isDev) std::cout << " (dev)";
        if (isLinked) std::cout << " -> linked";
        if (isMissing) std::cout << " \033[31m(missing)\033[0m";

        std::cout << std::endl;
    };

    // Print dependencies
    size_t totalDeps = deps.size() + devDeps.size();
    size_t current = 0;

    for (const auto& [name, version] : deps) {
        current++;
        printDep(name, version, false, current == totalDeps);
    }

    for (const auto& [name, version] : devDeps) {
        current++;
        printDep(name, version, true, current == totalDeps);
    }

    if (totalDeps == 0) {
        std::cout << "(no dependencies)" << std::endl;
    }
}

// Check for outdated packages
void PackageManager::checkOutdated(const std::string& projectPath) {
    projectPath_ = projectPath;
    std::filesystem::path basePath = std::filesystem::absolute(projectPath);
    std::filesystem::path packageJsonPath = basePath / "package.json";

    if (!std::filesystem::exists(packageJsonPath)) {
        std::cerr << "[nova] Error: package.json not found" << std::endl;
        return;
    }

    auto deps = parseDependencies(packageJsonPath.string(), false);
    auto devDeps = parseDependencies(packageJsonPath.string(), true);

    // Merge
    for (const auto& [name, version] : devDeps) {
        deps[name] = version;
    }

    if (deps.empty()) {
        std::cout << "[nova] No dependencies to check" << std::endl;
        return;
    }

    std::filesystem::path nodeModulesPath = basePath / "node_modules";

    // Table header
    std::cout << std::left;
    std::cout << std::setw(25) << "Package"
              << std::setw(15) << "Current"
              << std::setw(15) << "Wanted"
              << std::setw(15) << "Latest"
              << "Type" << std::endl;
    std::cout << std::string(80, '-') << std::endl;

    bool hasOutdated = false;

    for (const auto& [name, wantedVersion] : deps) {
        std::string currentVersion = "";

        // Get installed version
        std::filesystem::path pkgJsonPath = nodeModulesPath / name / "package.json";
        if (std::filesystem::exists(pkgJsonPath)) {
            std::ifstream file(pkgJsonPath);
            std::stringstream buffer;
            buffer << file.rdbuf();
            currentVersion = getJsonString(buffer.str(), "version");
        }

        // Get latest version from registry
        std::string latestVersion = getLatestVersion(name);

        if (latestVersion.empty()) continue;

        // Check if outdated
        bool isDev = devDeps.find(name) != devDeps.end();

        if (currentVersion != latestVersion) {
            hasOutdated = true;

            std::cout << std::setw(25) << name
                      << std::setw(15) << (currentVersion.empty() ? "N/A" : currentVersion)
                      << std::setw(15) << wantedVersion
                      << std::setw(15) << latestVersion
                      << (isDev ? "dev" : "dep") << std::endl;
        }
    }

    if (!hasOutdated) {
        std::cout << "[nova] All packages are up to date!" << std::endl;
    }
}

// Login to registry
bool PackageManager::login(const std::string& registry) {
    std::string targetRegistry = registry.empty() ? registry_ : registry;

    std::cout << "[nova] Log in to " << targetRegistry << std::endl;
    std::cout << std::endl;

    std::string username, password, email;

    std::cout << "Username: ";
    std::getline(std::cin, username);

    std::cout << "Password: ";
#ifdef _WIN32
    // Hide password input on Windows
    char ch;
    while ((ch = _getch()) != '\r') {
        if (ch == '\b') {
            if (!password.empty()) {
                password.pop_back();
                std::cout << "\b \b";
            }
        } else {
            password += ch;
            std::cout << '*';
        }
    }
    std::cout << std::endl;
#else
    std::getline(std::cin, password);
#endif

    std::cout << "Email: ";
    std::getline(std::cin, email);

    if (username.empty() || password.empty()) {
        std::cerr << "[nova] Error: Username and password are required" << std::endl;
        return false;
    }

    // Create auth token (simplified - real implementation would verify with registry)
    std::string authString = username + ":" + password;
    std::vector<unsigned char> authData(authString.begin(), authString.end());
    std::string token = base64Encode(authData);

    // Save credentials
    auto creds = loadCredentials();
    creds[targetRegistry] = token;
    creds[targetRegistry + "_user"] = username;
    creds[targetRegistry + "_email"] = email;
    saveCredentials(creds);

    std::cout << std::endl;
    std::cout << "[nova] Logged in as " << username << std::endl;

    return true;
}

// Logout from registry
bool PackageManager::logout(const std::string& registry) {
    std::string targetRegistry = registry.empty() ? registry_ : registry;

    auto creds = loadCredentials();

    if (creds.find(targetRegistry) == creds.end()) {
        std::cout << "[nova] Not logged in to " << targetRegistry << std::endl;
        return true;
    }

    std::string username = creds[targetRegistry + "_user"];

    creds.erase(targetRegistry);
    creds.erase(targetRegistry + "_user");
    creds.erase(targetRegistry + "_email");
    saveCredentials(creds);

    std::cout << "[nova] Logged out from " << targetRegistry;
    if (!username.empty()) {
        std::cout << " (was: " << username << ")";
    }
    std::cout << std::endl;

    return true;
}

// Check if logged in
bool PackageManager::isLoggedIn(const std::string& registry) {
    std::string targetRegistry = registry.empty() ? registry_ : registry;
    auto creds = loadCredentials();
    return creds.find(targetRegistry) != creds.end();
}

// Get auth token
std::string PackageManager::getAuthToken(const std::string& registry) {
    std::string targetRegistry = registry.empty() ? registry_ : registry;
    auto creds = loadCredentials();

    auto it = creds.find(targetRegistry);
    if (it != creds.end()) {
        return it->second;
    }
    return "";
}

// Pack project into tarball
std::string PackageManager::pack(const std::string& projectPath) {
    projectPath_ = projectPath;
    std::filesystem::path basePath = std::filesystem::absolute(projectPath);
    std::filesystem::path packageJsonPath = basePath / "package.json";

    if (!std::filesystem::exists(packageJsonPath)) {
        std::cerr << "[nova] Error: package.json not found" << std::endl;
        return "";
    }

    // Read package info
    std::ifstream file(packageJsonPath);
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();

    std::string packageName = getJsonString(content, "name");
    std::string version = getJsonString(content, "version");

    if (packageName.empty() || version.empty()) {
        std::cerr << "[nova] Error: Invalid package.json - missing name or version" << std::endl;
        return "";
    }

    // Create tarball name
    std::string tarballName = packageName + "-" + version + ".tgz";
    std::filesystem::path tarballPath = basePath / tarballName;

    std::cout << "[nova] Packing " << packageName << "@" << version << "..." << std::endl;

    // Create tar command
    std::string excludes = "--exclude=node_modules --exclude=.git --exclude=*.tgz --exclude=.DS_Store";

    // Check for .npmignore
    std::filesystem::path npmignorePath = basePath / ".npmignore";
    if (std::filesystem::exists(npmignorePath)) {
        std::ifstream ignoreFile(npmignorePath);
        std::string line;
        while (std::getline(ignoreFile, line)) {
            if (!line.empty() && line[0] != '#') {
                excludes += " --exclude=" + line;
            }
        }
    }

#ifdef _WIN32
    std::string cmd = "tar -czf \"" + tarballPath.string() + "\" " + excludes + " -C \"" + basePath.string() + "\" . 2>nul";
#else
    std::string cmd = "tar -czf \"" + tarballPath.string() + "\" " + excludes + " -C \"" + basePath.string() + "\" . 2>/dev/null";
#endif

    if (system(cmd.c_str()) != 0) {
        std::cerr << "[nova] Error: Failed to create tarball" << std::endl;
        return "";
    }

    // Get file size
    size_t fileSize = std::filesystem::file_size(tarballPath);

    std::cout << "[nova] Created " << tarballName << " (" << formatBytes(fileSize) << ")" << std::endl;

    return tarballPath.string();
}

// Publish package to registry
bool PackageManager::publish(const std::string& projectPath) {
    projectPath_ = projectPath;

    // Check login
    if (!isLoggedIn()) {
        std::cerr << "[nova] Error: You must be logged in to publish" << std::endl;
        std::cerr << "[nova] Run 'nova login' first" << std::endl;
        return false;
    }

    std::filesystem::path basePath = std::filesystem::absolute(projectPath);
    std::filesystem::path packageJsonPath = basePath / "package.json";

    // Read package info
    std::ifstream file(packageJsonPath);
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();

    std::string packageName = getJsonString(content, "name");
    std::string version = getJsonString(content, "version");

    std::cout << "[nova] Publishing " << packageName << "@" << version << " to " << registry_ << "..." << std::endl;

    // Pack first
    std::string tarballPath = pack(projectPath);
    if (tarballPath.empty()) {
        return false;
    }

    // Read tarball
    std::ifstream tarball(tarballPath, std::ios::binary);
    std::vector<unsigned char> tarballData((std::istreambuf_iterator<char>(tarball)),
                                            std::istreambuf_iterator<char>());
    tarball.close();

    // Base64 encode
    std::string tarballBase64 = base64Encode(tarballData);

    // Create publish body (simplified)
    std::stringstream publishBody;
    publishBody << "{";
    publishBody << "\"_id\": \"" << packageName << "\",";
    publishBody << "\"name\": \"" << packageName << "\",";
    publishBody << "\"versions\": {";
    publishBody << "\"" << version << "\": " << content;
    publishBody << "},";
    publishBody << "\"_attachments\": {";
    publishBody << "\"" << packageName << "-" << version << ".tgz\": {";
    publishBody << "\"content_type\": \"application/octet-stream\",";
    publishBody << "\"data\": \"" << tarballBase64 << "\"";
    publishBody << "}}}";

    // Send PUT request
    std::string url = registry_ + "/" + packageName;
    std::string token = getAuthToken();

    // TODO: Implement actual HTTP PUT with auth
    std::cout << "[nova] Would publish to: " << url << std::endl;
    std::cout << "[nova] Note: Actual publish requires npm registry authentication" << std::endl;

    // Clean up tarball
    std::filesystem::remove(tarballPath);

    return true;
}

// Clean old cache entries
void PackageManager::cleanCache(int olderThanDays) {
    std::cout << "[nova] Cleaning cache older than " << olderThanDays << " days..." << std::endl;

    auto now = std::chrono::system_clock::now();
    auto threshold = now - std::chrono::hours(24 * olderThanDays);

    size_t totalCleaned = 0;
    size_t bytesFreed = 0;

    for (const auto& entry : std::filesystem::recursive_directory_iterator(cacheDir_)) {
        if (!entry.is_regular_file()) continue;

        auto lastWrite = std::filesystem::last_write_time(entry);
        auto lastWriteTime = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            lastWrite - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());

        if (lastWriteTime < threshold) {
            bytesFreed += entry.file_size();
            std::filesystem::remove(entry);
            totalCleaned++;
        }
    }

    std::cout << "[nova] Cleaned " << totalCleaned << " files (" << formatBytes(bytesFreed) << ")" << std::endl;
}

} // namespace pm
} // namespace nova
