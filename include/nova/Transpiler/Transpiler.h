#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <filesystem>
#include <functional>
#include <set>

namespace nova {
namespace transpiler {

// tsconfig.json compilerOptions - Full compatibility with tsc
struct CompilerOptions {
    // === Output Options ===
    std::string outDir = "";              // Output directory
    std::string outFile = "";             // Bundle all output into one file
    std::string rootDir = "";             // Root directory of input files
    std::string declarationDir = "";      // Output directory for .d.ts files

    // === Module Options ===
    std::string module = "commonjs";      // commonjs, es6, es2015, es2020, es2022, esnext, node16, nodenext
    std::string moduleResolution = "node"; // node, node16, nodenext, classic, bundler
    std::string baseUrl = "";             // Base directory for non-relative module names
    std::map<std::string, std::vector<std::string>> paths; // Path mapping
    std::vector<std::string> rootDirs;    // List of root directories
    std::vector<std::string> typeRoots;   // Folders to include type definitions from
    std::vector<std::string> types;       // Type declaration files to include
    bool resolveJsonModule = false;       // Allow importing .json files
    bool allowSyntheticDefaultImports = true;
    bool esModuleInterop = true;

    // === Target & Language ===
    std::string target = "es2020";        // es3, es5, es6, es2015-es2022, esnext
    std::vector<std::string> lib;         // Library files to include

    // === JSX Options ===
    std::string jsx = "";                 // preserve, react, react-jsx, react-jsxdev, react-native
    std::string jsxFactory = "React.createElement";
    std::string jsxFragmentFactory = "React.Fragment";
    std::string jsxImportSource = "react";

    // === Declaration Options ===
    bool declaration = false;             // Generate .d.ts files
    bool declarationMap = false;          // Generate sourcemaps for .d.ts files
    bool emitDeclarationOnly = false;     // Only emit .d.ts files

    // === Source Map Options ===
    bool sourceMap = false;               // Generate .map files
    bool inlineSourceMap = false;         // Include sourcemap in .js file
    bool inlineSources = false;           // Include source in sourcemap
    std::string sourceRoot = "";          // Root path for sources in sourcemap
    std::string mapRoot = "";             // Root path for sourcemap files

    // === Emit Options ===
    bool removeComments = false;          // Remove comments from output
    bool noEmit = false;                  // Don't emit output files
    bool noEmitOnError = false;           // Don't emit if errors
    bool preserveConstEnums = false;      // Keep const enum declarations
    bool importHelpers = false;           // Import helpers from tslib
    bool downlevelIteration = false;      // Emit helpers for iteration
    bool emitBOM = false;                 // Emit UTF-8 BOM
    std::string newLine = "";             // crlf or lf
    bool stripInternal = false;           // Don't emit @internal members
    bool noEmitHelpers = false;           // Don't generate helper functions

    // === JavaScript Support ===
    bool allowJs = false;                 // Allow JavaScript files
    bool checkJs = false;                 // Type check JavaScript files
    int maxNodeModuleJsDepth = 0;         // Max depth for node_modules JS

    // === Type Checking (parsed for compatibility, affects alwaysStrict output) ===
    bool strict = false;
    bool noImplicitAny = false;
    bool strictNullChecks = false;
    bool strictFunctionTypes = false;
    bool strictBindCallApply = false;
    bool strictPropertyInitialization = false;
    bool noImplicitThis = false;
    bool useUnknownInCatchVariables = false;
    bool alwaysStrict = false;            // Adds "use strict" to output
    bool noUnusedLocals = false;
    bool noUnusedParameters = false;
    bool exactOptionalPropertyTypes = false;
    bool noImplicitReturns = false;
    bool noFallthroughCasesInSwitch = false;
    bool noUncheckedIndexedAccess = false;
    bool noImplicitOverride = false;
    bool noPropertyAccessFromIndexSignature = false;
    bool allowUnusedLabels = false;
    bool allowUnreachableCode = false;

    // === Module Detection ===
    std::string moduleDetection = "auto"; // auto, legacy, force

    // === Interop Constraints ===
    bool isolatedModules = false;
    bool isolatedDeclarations = false;    // TS 5.5: Require explicit type annotations
    bool verbatimModuleSyntax = false;
    bool allowArbitraryExtensions = false;
    bool allowImportingTsExtensions = false;
    bool resolvePackageJsonExports = true;
    bool resolvePackageJsonImports = true;
    std::vector<std::string> customConditions; // Custom export conditions
    std::vector<std::string> moduleSuffixes; // Module suffixes for resolution
    bool noResolve = false;               // Don't resolve triple-slash references
    bool allowUmdGlobalAccess = false;    // Allow UMD global access
    bool rewriteRelativeImportExtensions = false; // TS 5.7: Rewrite .ts to .js in imports

    // === Decorators ===
    bool experimentalDecorators = false;  // Enable decorators
    bool emitDecoratorMetadata = false;   // Emit decorator metadata
    bool useDefineForClassFields = true;  // Use define for class fields

    // === Build Options ===
    bool composite = false;               // Enable project references
    bool incremental = false;             // Incremental compilation
    std::string tsBuildInfoFile = "";     // Build info file location
    bool disableSolutionSearching = false;
    bool disableReferencedProjectLoad = false;
    bool disableSourceOfProjectReferenceRedirect = false;
    bool disableSizeLimit = false;

    // === Watch Options ===
    bool assumeChangesOnlyAffectDirectDependencies = false;
    bool preserveWatchOutput = false;

    // === Completeness ===
    bool skipLibCheck = true;
    bool skipDefaultLibCheck = false;
    bool forceConsistentCasingInFileNames = true;

    // === Advanced / Diagnostic ===
    bool noLib = false;                   // Don't include default lib.d.ts
    bool preserveSymlinks = false;        // Don't resolve symlinks
    bool noErrorTruncation = false;       // Don't truncate error messages
    bool listFiles = false;               // Print files in compilation
    bool listEmittedFiles = false;        // Print emitted files
    bool traceResolution = false;         // Trace module resolution
    bool extendedDiagnostics = false;     // Show extended diagnostics
    bool explainFiles = false;            // Explain why files are included
    bool pretty = true;                   // Colorize errors and messages
    bool generateCpuProfile = false;      // Generate CPU profile
    std::string generateTrace = "";       // Generate trace file

    // === Deprecated (parsed for compatibility) ===
    bool keyofStringsOnly = false;        // Deprecated
    bool suppressExcessPropertyErrors = false;  // Deprecated
    bool suppressImplicitAnyIndexErrors = false; // Deprecated
    bool noStrictGenericChecks = false;   // Deprecated
    std::string charset = "";             // Deprecated
    bool importsNotUsedAsValues = false;  // Deprecated (use verbatimModuleSyntax)
    bool preserveValueImports = false;    // Deprecated (use verbatimModuleSyntax)

    // === Language Service Plugins ===
    struct Plugin {
        std::string name;
        std::map<std::string, std::string> options;
    };
    std::vector<Plugin> plugins;

    // === Nova-specific optimizations ===
    bool minify = false;                  // Minify output
    bool treeshake = false;               // Remove unused code (future)
    bool inlineSmallFunctions = false;    // Inline small functions (future)
    bool optimizeSize = false;            // Optimize for size
};

struct TSConfig {
    CompilerOptions compilerOptions;
    std::vector<std::string> include;
    std::vector<std::string> exclude;
    std::vector<std::string> files;       // Explicit file list
    std::string extends;                  // Extend from base config

    // Watch options
    struct WatchOptions {
        std::string watchFile = "useFsEvents";
        std::string watchDirectory = "useFsEvents";
        std::string fallbackPolling = "dynamicPriority";
        bool synchronousWatchDirectory = false;
        std::vector<std::string> excludeDirectories;
        std::vector<std::string> excludeFiles;
    } watchOptions;

    // Project references
    struct Reference {
        std::string path;
        bool prepend = false;
    };
    std::vector<Reference> references;
};

// Transpiler result
struct TranspileResult {
    std::string filename;
    std::string jsCode;
    std::string dtsCode;         // Declaration file content
    std::string sourceMap;       // Source map content (.js.map)
    std::string declarationMap;  // Declaration map content (.d.ts.map)
    bool success;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;

    // Stats
    size_t inputSize;
    size_t outputSize;
    double transpileTimeMs;
};

struct BuildResult {
    bool success;
    std::vector<TranspileResult> files;
    std::vector<std::string> errors;

    // Stats
    int totalFiles;
    int successCount;
    int failCount;
    double totalTimeMs;
    size_t totalInputSize;
    size_t totalOutputSize;
};

class Transpiler {
public:
    Transpiler();
    ~Transpiler();

    // Load tsconfig.json (with extends support)
    bool loadConfig(const std::string& configPath);

    // Set options programmatically
    void setOptions(const CompilerOptions& options);
    CompilerOptions& getOptions() { return config_.compilerOptions; }

    // Transpile single file
    TranspileResult transpileFile(const std::string& filePath);

    // Transpile string content
    TranspileResult transpileString(const std::string& content, const std::string& filename = "input.ts");

    // Build entire project (using tsconfig.json)
    BuildResult build(const std::string& projectPath = ".");

    // Watch mode
    void watch(const std::string& projectPath, std::function<void(const TranspileResult&)> callback);

private:
    TSConfig config_;
    std::string projectRoot_;
    std::string configDir_;  // Directory containing tsconfig.json

    // Build cache for incremental builds
    struct BuildCache {
        std::map<std::string, std::filesystem::file_time_type> fileModTimes;
        std::map<std::string, std::string> fileHashes;
        bool isValid = false;
    } buildCache_;

    // Internal methods
    std::string transformTypeScript(const std::string& source, const std::string& filename);
    std::string removeTypeAnnotations(const std::string& source);
    std::string transformImports(const std::string& source);
    std::string transformExports(const std::string& source);
    std::string transformJSX(const std::string& source);
    std::string transformPaths(const std::string& source);
    std::string downlevelToTarget(const std::string& source);
    std::string minifyCode(const std::string& source);
    std::string generateSourceMap(const std::string& source, const std::string& output, const std::string& filename);
    std::string generateDeclaration(const std::string& source);
    std::string generateDeclarationMap(const std::string& source, const std::string& dtsCode, const std::string& filename);

    std::vector<std::string> findSourceFiles(const std::string& projectPath);
    bool matchesGlob(const std::string& path, const std::string& pattern);
    std::string resolveOutputPath(const std::string& inputPath, const std::string& ext);

    // Config helpers
    bool loadConfigRecursive(const std::string& configPath, std::set<std::string>& visited);
    void mergeConfig(const TSConfig& base);
    std::string resolveConfigPath(const std::string& extendsPath);

    // Incremental build helpers
    bool needsRebuild(const std::string& filePath);
    void saveBuildInfo();
    void loadBuildInfo();
};

// Utility functions
TSConfig parseTSConfig(const std::string& jsonContent);
std::string serializeTSConfig(const TSConfig& config);

} // namespace transpiler
} // namespace nova
