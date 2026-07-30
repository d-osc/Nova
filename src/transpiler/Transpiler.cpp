#include "nova/Transpiler/Transpiler.h"
#include <fstream>
#include <sstream>
#include <chrono>
#include <regex>
#include <algorithm>
#include <cctype>
#include <iostream>
#include <thread>
#include <future>
#include <iomanip>

namespace nova {
namespace transpiler {

// ============================================================================
// Base64 Encoding for inline source maps
// ============================================================================

namespace {
    static const char base64Chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    std::string base64Encode(const std::string& input) {
        std::string output;
        int i = 0;
        unsigned char charArray3[3];
        unsigned char charArray4[4];
        size_t inputLen = input.size();
        const char* bytesToEncode = input.c_str();

        while (inputLen--) {
            charArray3[i++] = *(bytesToEncode++);
            if (i == 3) {
                charArray4[0] = (charArray3[0] & 0xfc) >> 2;
                charArray4[1] = ((charArray3[0] & 0x03) << 4) + ((charArray3[1] & 0xf0) >> 4);
                charArray4[2] = ((charArray3[1] & 0x0f) << 2) + ((charArray3[2] & 0xc0) >> 6);
                charArray4[3] = charArray3[2] & 0x3f;

                for (i = 0; i < 4; i++)
                    output += base64Chars[charArray4[i]];
                i = 0;
            }
        }

        if (i) {
            for (int j = i; j < 3; j++)
                charArray3[j] = '\0';

            charArray4[0] = (charArray3[0] & 0xfc) >> 2;
            charArray4[1] = ((charArray3[0] & 0x03) << 4) + ((charArray3[1] & 0xf0) >> 4);
            charArray4[2] = ((charArray3[1] & 0x0f) << 2) + ((charArray3[2] & 0xc0) >> 6);

            for (int j = 0; j < i + 1; j++)
                output += base64Chars[charArray4[j]];

            while (i++ < 3)
                output += '=';
        }

        return output;
    }

    std::string jsonEscape(const std::string& input) {
        std::string output;
        for (unsigned char character : input) {
            switch (character) {
                case '\\': output += "\\\\"; break;
                case '"': output += "\\\""; break;
                case '\b': output += "\\b"; break;
                case '\f': output += "\\f"; break;
                case '\n': output += "\\n"; break;
                case '\r': output += "\\r"; break;
                case '\t': output += "\\t"; break;
                default:
                    if (character < 0x20) {
                        const char* digits = "0123456789abcdef";
                        output += "\\u00";
                        output += digits[(character >> 4) & 0xf];
                        output += digits[character & 0xf];
                    } else {
                        output += static_cast<char>(character);
                    }
            }
        }
        return output;
    }

    class JSXTransformer {
    public:
        JSXTransformer(const std::string& source, std::string factory,
                       std::string fragment, bool automatic)
            : source_(source), factory_(std::move(factory)),
              fragment_(std::move(fragment)), automatic_(automatic) {}

        std::string transform(bool& changed) {
            std::string output;
            for (size_t position = 0; position < source_.size();) {
                std::string element;
                size_t end = position;
                if (source_[position] == '<' &&
                    parseElement(position, end, element)) {
                    output += element;
                    position = end;
                    changed = true;
                } else {
                    output += source_[position++];
                }
            }
            return output;
        }

    private:
        const std::string& source_;
        std::string factory_;
        std::string fragment_;
        bool automatic_;

        static void skipSpace(const std::string& source, size_t& position) {
            while (position < source.size() &&
                   std::isspace(static_cast<unsigned char>(
                       source[position])) != 0) {
                ++position;
            }
        }

        static std::string quote(const std::string& text) {
            std::string result = "\"";
            for (char character : text) {
                if (character == '\\' || character == '"') result += '\\';
                if (character == '\n' || character == '\r') {
                    result += ' ';
                } else {
                    result += character;
                }
            }
            result += '"';
            return result;
        }

        bool parseBraced(size_t start, size_t& end,
                         std::string& expression) const {
            if (start >= source_.size() || source_[start] != '{') return false;
            int depth = 1;
            char stringQuote = '\0';
            for (size_t position = start + 1; position < source_.size();
                 ++position) {
                const char character = source_[position];
                if (stringQuote != '\0') {
                    if (character == '\\') {
                        ++position;
                    } else if (character == stringQuote) {
                        stringQuote = '\0';
                    }
                    continue;
                }
                if (character == '\'' || character == '"' ||
                    character == '`') {
                    stringQuote = character;
                } else if (character == '{') {
                    ++depth;
                } else if (character == '}' && --depth == 0) {
                    expression =
                        source_.substr(start + 1, position - start - 1);
                    end = position + 1;
                    return true;
                }
            }
            return false;
        }

        bool parseElement(size_t start, size_t& end,
                          std::string& output) const {
            if (start >= source_.size() || source_[start] != '<' ||
                start + 1 >= source_.size() || source_[start + 1] == '/') {
                return false;
            }
            size_t position = start + 1;
            bool fragment = source_[position] == '>';
            std::string tag;
            if (fragment) {
                ++position;
            } else {
                if (!(std::isalpha(static_cast<unsigned char>(
                          source_[position])) != 0 ||
                      source_[position] == '_' ||
                      source_[position] == '$')) {
                    return false;
                }
                const size_t tagStart = position;
                while (position < source_.size() &&
                       (std::isalnum(static_cast<unsigned char>(
                            source_[position])) != 0 ||
                        source_[position] == '_' ||
                        source_[position] == '$' ||
                        source_[position] == '.' ||
                        source_[position] == '-' ||
                        source_[position] == ':')) {
                    ++position;
                }
                tag = source_.substr(tagStart, position - tagStart);
            }

            std::vector<std::string> properties;
            bool selfClosing = false;
            if (!fragment) {
                while (position < source_.size()) {
                    skipSpace(source_, position);
                    if (source_.compare(position, 2, "/>") == 0) {
                        position += 2;
                        selfClosing = true;
                        break;
                    }
                    if (position < source_.size() &&
                        source_[position] == '>') {
                        ++position;
                        break;
                    }
                    if (position < source_.size() &&
                        source_[position] == '{') {
                        size_t braceEnd = position;
                        std::string spread;
                        if (!parseBraced(position, braceEnd, spread) ||
                            spread.rfind("...", 0) != 0) {
                            return false;
                        }
                        properties.push_back(spread);
                        position = braceEnd;
                        continue;
                    }
                    const size_t nameStart = position;
                    while (position < source_.size() &&
                           (std::isalnum(static_cast<unsigned char>(
                                source_[position])) != 0 ||
                            source_[position] == '_' ||
                            source_[position] == '$' ||
                            source_[position] == '-')) {
                        ++position;
                    }
                    if (position == nameStart) return false;
                    std::string name =
                        source_.substr(nameStart, position - nameStart);
                    skipSpace(source_, position);
                    std::string value = "true";
                    if (position < source_.size() &&
                        source_[position] == '=') {
                        ++position;
                        skipSpace(source_, position);
                        if (position >= source_.size()) return false;
                        if (source_[position] == '"' ||
                            source_[position] == '\'') {
                            const char delimiter = source_[position++];
                            const size_t valueStart = position;
                            while (position < source_.size() &&
                                   source_[position] != delimiter) {
                                if (source_[position] == '\\') ++position;
                                ++position;
                            }
                            if (position >= source_.size()) return false;
                            value = quote(source_.substr(
                                valueStart, position - valueStart));
                            ++position;
                        } else if (source_[position] == '{') {
                            size_t braceEnd = position;
                            if (!parseBraced(position, braceEnd, value)) {
                                return false;
                            }
                            position = braceEnd;
                        } else {
                            const size_t valueStart = position;
                            while (position < source_.size() &&
                                   std::isspace(static_cast<unsigned char>(
                                       source_[position])) == 0 &&
                                   source_[position] != '>' &&
                                   source_[position] != '/') {
                                ++position;
                            }
                            value = source_.substr(
                                valueStart, position - valueStart);
                        }
                    }
                    properties.push_back(name + ": " + value);
                }
            }

            std::vector<std::string> children;
            if (!selfClosing) {
                while (position < source_.size()) {
                    if ((fragment &&
                         source_.compare(position, 3, "</>") == 0) ||
                        (!fragment &&
                         source_.compare(position, tag.size() + 3,
                             "</" + tag + ">") == 0)) {
                        position += fragment ? 3 : tag.size() + 3;
                        break;
                    }
                    if (source_[position] == '<') {
                        size_t childEnd = position;
                        std::string child;
                        if (!parseElement(position, childEnd, child)) {
                            return false;
                        }
                        children.push_back(child);
                        position = childEnd;
                    } else if (source_[position] == '{') {
                        size_t braceEnd = position;
                        std::string expression;
                        if (!parseBraced(position, braceEnd, expression)) {
                            return false;
                        }
                        if (expression.find_first_not_of(" \t\r\n") !=
                            std::string::npos) {
                            children.push_back(expression);
                        }
                        position = braceEnd;
                    } else {
                        const size_t textStart = position;
                        while (position < source_.size() &&
                               source_[position] != '<' &&
                               source_[position] != '{') {
                            ++position;
                        }
                        std::string text = source_.substr(
                            textStart, position - textStart);
                        const size_t first = text.find_first_not_of(" \t\r\n");
                        const size_t last = text.find_last_not_of(" \t\r\n");
                        if (first != std::string::npos) {
                            children.push_back(
                                quote(text.substr(first, last - first + 1)));
                        }
                    }
                }
            }

            const std::string elementType = fragment
                ? fragment_
                : (std::islower(static_cast<unsigned char>(tag[0])) != 0
                    ? quote(tag) : tag);
            if (automatic_) {
                if (!children.empty()) {
                    std::string childValue;
                    if (children.size() == 1) {
                        childValue = children.front();
                    } else {
                        childValue = "[";
                        for (size_t index = 0; index < children.size();
                             ++index) {
                            if (index) childValue += ", ";
                            childValue += children[index];
                        }
                        childValue += "]";
                    }
                    properties.push_back("children: " + childValue);
                }
                std::string props = "{";
                for (size_t index = 0; index < properties.size(); ++index) {
                    if (index) props += ", ";
                    props += properties[index];
                }
                props += "}";
                output = (children.size() > 1 ? "_jsxs" : factory_) +
                    "(" + elementType + ", " + props + ")";
            } else {
                std::string props = properties.empty() ? "null" : "{";
                if (!properties.empty()) {
                    for (size_t index = 0; index < properties.size(); ++index) {
                        if (index) props += ", ";
                        props += properties[index];
                    }
                    props += "}";
                }
                output = factory_ + "(" + elementType + ", " + props;
                for (const std::string& child : children) {
                    output += ", " + child;
                }
                output += ")";
            }
            end = position;
            return true;
        }
    };
}

Transpiler::Transpiler() {
    // Default configuration
    config_.compilerOptions.target = "es2020";
    config_.compilerOptions.module = "commonjs";
    config_.compilerOptions.outDir = "./dist";
    config_.include.push_back("**/*.ts");
    config_.include.push_back("**/*.tsx");
    config_.exclude.push_back("node_modules");
    config_.exclude.push_back("**/*.d.ts");
}

Transpiler::~Transpiler() = default;

// ============================================================================
// Config Loading with extends support
// ============================================================================

bool Transpiler::loadConfig(const std::string& configPath) {
    std::set<std::string> visited;
    return loadConfigRecursive(configPath, visited);
}

bool Transpiler::loadConfigRecursive(const std::string& configPath, std::set<std::string>& visited) {
    // Prevent circular extends
    std::filesystem::path absPath = std::filesystem::absolute(configPath);
    std::string absPathStr = absPath.string();

    if (visited.find(absPathStr) != visited.end()) {
        return false; // Circular reference
    }
    visited.insert(absPathStr);

    std::ifstream file(configPath);
    if (!file.is_open()) {
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    file.close();

    const std::string currentConfigDir = absPath.parent_path().string();
    configDir_ = currentConfigDir;
    if (configDir_.empty()) configDir_ = ".";
    projectRoot_ = configDir_;

    // Parse the config
    TSConfig newConfig = parseTSConfig(content);

    // Handle extends
    if (!newConfig.extends.empty()) {
        std::string basePath = resolveConfigPath(newConfig.extends);
        if (!loadConfigRecursive(basePath, visited)) return false;
        configDir_ = currentConfigDir.empty() ? "." : currentConfigDir;
        projectRoot_ = configDir_;
    }

    // Apply current config on top
    mergeConfig(newConfig);

    return true;
}

std::string Transpiler::resolveConfigPath(const std::string& extendsPath) {
    // Handle npm package extends (e.g., "@tsconfig/node16/tsconfig.json")
    if (extendsPath[0] != '.' && extendsPath[0] != '/') {
        // Try node_modules
        std::string npmPath = configDir_ + "/node_modules/" + extendsPath;
        if (std::filesystem::exists(npmPath)) {
            return npmPath;
        }
        // Try with /tsconfig.json appended
        if (extendsPath.find(".json") == std::string::npos) {
            npmPath = configDir_ + "/node_modules/" + extendsPath + "/tsconfig.json";
            if (std::filesystem::exists(npmPath)) {
                return npmPath;
            }
        }
    }

    // Relative path
    std::filesystem::path resolved = std::filesystem::path(configDir_) / extendsPath;
    if (std::filesystem::exists(resolved)) {
        return resolved.string();
    }

    // Try with .json extension
    if (extendsPath.find(".json") == std::string::npos) {
        resolved = std::filesystem::path(configDir_) / (extendsPath + ".json");
        if (std::filesystem::exists(resolved)) {
            return resolved.string();
        }
    }

    return extendsPath;
}

void Transpiler::mergeConfig(const TSConfig& other) {
    auto& opts = config_.compilerOptions;
    auto& otherOpts = other.compilerOptions;
    const auto specified = [&](const std::string& option) {
        return other.specifiedCompilerOptions.count(option) != 0;
    };

    if (specified("outDir")) opts.outDir = otherOpts.outDir;
    if (specified("outFile")) opts.outFile = otherOpts.outFile;
    if (specified("rootDir")) opts.rootDir = otherOpts.rootDir;
    if (specified("declarationDir")) {
        opts.declarationDir = otherOpts.declarationDir;
    }
    if (specified("module")) opts.module = otherOpts.module;
    if (specified("moduleResolution")) {
        opts.moduleResolution = otherOpts.moduleResolution;
    }
    if (specified("moduleDetection")) {
        opts.moduleDetection = otherOpts.moduleDetection;
    }
    if (specified("baseUrl")) opts.baseUrl = otherOpts.baseUrl;
    if (specified("target")) opts.target = otherOpts.target;
    if (specified("jsx")) opts.jsx = otherOpts.jsx;
    if (specified("jsxFactory")) opts.jsxFactory = otherOpts.jsxFactory;
    if (specified("jsxFragmentFactory")) {
        opts.jsxFragmentFactory = otherOpts.jsxFragmentFactory;
    }
    if (specified("jsxImportSource")) {
        opts.jsxImportSource = otherOpts.jsxImportSource;
    }
    if (specified("sourceRoot")) opts.sourceRoot = otherOpts.sourceRoot;
    if (specified("mapRoot")) opts.mapRoot = otherOpts.mapRoot;
    if (specified("newLine")) opts.newLine = otherOpts.newLine;
    if (specified("tsBuildInfoFile")) {
        opts.tsBuildInfoFile = otherOpts.tsBuildInfoFile;
    }

#define NOVA_MERGE_BOOL(name) \
    if (specified(#name)) opts.name = otherOpts.name
    NOVA_MERGE_BOOL(declaration);
    NOVA_MERGE_BOOL(declarationMap);
    NOVA_MERGE_BOOL(emitDeclarationOnly);
    NOVA_MERGE_BOOL(sourceMap);
    NOVA_MERGE_BOOL(inlineSourceMap);
    NOVA_MERGE_BOOL(inlineSources);
    NOVA_MERGE_BOOL(removeComments);
    NOVA_MERGE_BOOL(noEmit);
    NOVA_MERGE_BOOL(noEmitOnError);
    NOVA_MERGE_BOOL(preserveConstEnums);
    NOVA_MERGE_BOOL(importHelpers);
    NOVA_MERGE_BOOL(downlevelIteration);
    NOVA_MERGE_BOOL(allowJs);
    NOVA_MERGE_BOOL(checkJs);
    NOVA_MERGE_BOOL(resolveJsonModule);
    NOVA_MERGE_BOOL(esModuleInterop);
    NOVA_MERGE_BOOL(allowSyntheticDefaultImports);
    NOVA_MERGE_BOOL(strict);
    NOVA_MERGE_BOOL(composite);
    NOVA_MERGE_BOOL(incremental);
    NOVA_MERGE_BOOL(isolatedModules);
    NOVA_MERGE_BOOL(isolatedDeclarations);
    NOVA_MERGE_BOOL(verbatimModuleSyntax);
    NOVA_MERGE_BOOL(allowArbitraryExtensions);
    NOVA_MERGE_BOOL(allowImportingTsExtensions);
    NOVA_MERGE_BOOL(resolvePackageJsonExports);
    NOVA_MERGE_BOOL(resolvePackageJsonImports);
    NOVA_MERGE_BOOL(noResolve);
    NOVA_MERGE_BOOL(rewriteRelativeImportExtensions);
    NOVA_MERGE_BOOL(skipLibCheck);
    NOVA_MERGE_BOOL(minify);
#undef NOVA_MERGE_BOOL

    // Merge paths
    if (specified("paths")) {
        for (const auto& [key, value] : otherOpts.paths) {
            opts.paths[key] = value;
        }
    }

    if (other.specifiedTopLevel.count("include")) {
        config_.include = other.include;
    }
    if (other.specifiedTopLevel.count("exclude")) {
        config_.exclude = other.exclude;
    }
    if (other.specifiedTopLevel.count("files")) {
        config_.files = other.files;
    }
    if (other.specifiedTopLevel.count("references")) {
        config_.references = other.references;
    }
    if (specified("lib")) opts.lib = otherOpts.lib;
    if (specified("types")) opts.types = otherOpts.types;
    if (specified("typeRoots")) opts.typeRoots = otherOpts.typeRoots;
    if (specified("rootDirs")) opts.rootDirs = otherOpts.rootDirs;
    if (specified("moduleSuffixes")) {
        opts.moduleSuffixes = otherOpts.moduleSuffixes;
    }
    if (specified("customConditions")) {
        opts.customConditions = otherOpts.customConditions;
    }
}

void Transpiler::setOptions(const CompilerOptions& options) {
    config_.compilerOptions = options;
}

// ============================================================================
// Transpilation
// ============================================================================

TranspileResult Transpiler::transpileFile(const std::string& filePath) {
    auto startTime = std::chrono::high_resolution_clock::now();

    TranspileResult result;
    result.filename = filePath;
    result.success = false;

    // Read file
    std::ifstream file(filePath);
    if (!file.is_open()) {
        result.errors.push_back("Cannot open file: " + filePath);
        return result;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();
    file.close();

    result.inputSize = source.size();

    // Transpile
    result = transpileString(source, filePath);

    auto endTime = std::chrono::high_resolution_clock::now();
    result.transpileTimeMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();

    return result;
}

TranspileResult Transpiler::transpileString(const std::string& content, const std::string& filename) {
    auto startTime = std::chrono::high_resolution_clock::now();

    TranspileResult result;
    result.filename = filename;
    result.inputSize = content.size();
    result.success = true;

    try {
        // Check if noEmit
        if (config_.compilerOptions.noEmit) {
            result.jsCode = "";
            result.outputSize = 0;
            return result;
        }

        // Transform TypeScript to JavaScript
        std::string jsCode = transformTypeScript(content, filename);

        // Minify if requested
        if (config_.compilerOptions.minify) {
            jsCode = minifyCode(jsCode);
        }

        // Check emitDeclarationOnly
        if (!config_.compilerOptions.emitDeclarationOnly) {
            result.jsCode = jsCode;
        }
        result.outputSize = jsCode.size();

        // Generate declaration file if requested
        if (config_.compilerOptions.declaration) {
            result.dtsCode = generateDeclaration(content);

            // Generate declaration map if requested
            if (config_.compilerOptions.declarationMap && !result.dtsCode.empty()) {
                result.declarationMap = generateDeclarationMap(content, result.dtsCode, filename);
                result.dtsCode += "\n//# sourceMappingURL=" +
                    std::filesystem::path(filename).stem().string() +
                    ".d.ts.map\n";
            }
        }

        // Generate source map if requested
        if (config_.compilerOptions.sourceMap && !config_.compilerOptions.inlineSourceMap) {
            result.sourceMap = generateSourceMap(content, jsCode, filename);
            if (!result.jsCode.empty()) {
                result.jsCode += "\n//# sourceMappingURL=" +
                    std::filesystem::path(filename).stem().string() +
                    ".js.map\n";
            }
        } else if (config_.compilerOptions.inlineSourceMap) {
            std::string sourceMapData = generateSourceMap(content, jsCode, filename);
            // Base64 encode and append to jsCode
            result.jsCode += "\n//# sourceMappingURL=data:application/json;base64," + base64Encode(sourceMapData);
        }

    } catch (const std::exception& e) {
        result.success = false;
        result.errors.push_back(e.what());
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    result.transpileTimeMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();

    return result;
}

std::string Transpiler::transformTypeScript(const std::string& source, const std::string& filename) {
    std::string result = source;

    // Step 1: Remove TypeScript-only syntax before injecting JSX runtime
    // imports so import aliases are not mistaken for `as` assertions.
    result = removeTypeAnnotations(result);

    // Step 2: Transform JSX if needed
    bool isTsx = filename.size() > 4 && filename.substr(filename.size() - 4) == ".tsx";
    bool isJsx = filename.size() > 4 && filename.substr(filename.size() - 4) == ".jsx";
    if ((isTsx || isJsx) && !config_.compilerOptions.jsx.empty()) {
        result = transformJSX(result);
    }

    // Step 3: Transform path aliases
    if (!config_.compilerOptions.paths.empty() ||
        !config_.compilerOptions.baseUrl.empty() ||
        config_.compilerOptions.rewriteRelativeImportExtensions) {
        result = transformPaths(result, filename);
    }

    // Step 4: Transform imports based on module type
    result = transformImports(result);

    // Step 5: Transform exports based on module type
    result = transformExports(result);

    // Step 6: Downlevel to target if needed
    if (config_.compilerOptions.target == "es5" || config_.compilerOptions.target == "es3") {
        result = downlevelToTarget(result);
    }

    // Step 7: Remove comments if configured
    if (config_.compilerOptions.removeComments) {
        // Protect URLs by temporarily replacing them
        std::string urlPlaceholder = "__URL_PLACEHOLDER_";
        std::vector<std::string> urls;
        std::regex urlRegex("(https?://[^\\s\"'<>]+)");
        std::smatch urlMatch;
        std::string tempResult = result;
        while (std::regex_search(tempResult, urlMatch, urlRegex)) {
            urls.push_back(urlMatch[1].str());
            tempResult = urlMatch.suffix().str();
        }
        for (size_t i = 0; i < urls.size(); i++) {
            size_t pos = result.find(urls[i]);
            if (pos != std::string::npos) {
                result.replace(pos, urls[i].size(), urlPlaceholder + std::to_string(i) + "__");
            }
        }

        // Remove single-line comments
        result = std::regex_replace(result, std::regex("//[^\n]*"), "");
        // Remove multi-line comments
        result = std::regex_replace(result, std::regex("/\\*[\\s\\S]*?\\*/"), "");

        // Restore URLs
        for (size_t i = 0; i < urls.size(); i++) {
            std::string placeholder = urlPlaceholder + std::to_string(i) + "__";
            size_t pos = result.find(placeholder);
            if (pos != std::string::npos) {
                result.replace(pos, placeholder.size(), urls[i]);
            }
        }
    }

    // Step 8: Clean up extra whitespace
    result = std::regex_replace(result, std::regex("\n\\s*\n\\s*\n"), "\n\n");

    // Step 9: Add "use strict" if alwaysStrict is enabled
    if (config_.compilerOptions.alwaysStrict || config_.compilerOptions.strict) {
        // Check if "use strict" is not already present
        if (result.find("\"use strict\"") == std::string::npos &&
            result.find("'use strict'") == std::string::npos) {
            result = "\"use strict\";\n" + result;
        }
    }

    // Handle newLine option
    if (config_.compilerOptions.newLine == "crlf") {
        result = std::regex_replace(result, std::regex("\n"), "\r\n");
    } else if (config_.compilerOptions.newLine == "lf") {
        result = std::regex_replace(result, std::regex("\r\n"), "\n");
    }

    return result;
}

std::string Transpiler::removeTypeAnnotations(const std::string& source) {
    std::string result = source;

    // Remove interface declarations (with optional generics)
    result = std::regex_replace(result, std::regex("interface\\s+\\w+\\s*(?:<[^>]*>)?\\s*(?:extends\\s+[^{]+)?\\s*\\{[^}]*\\}"), "");

    // Remove type alias declarations
    result = std::regex_replace(result, std::regex("type\\s+\\w+\\s*(?:<[^>]*>)?\\s*=\\s*[^;]+;"), "");

    // Remove optional parameter type annotations: (param?: Type) -> (param)
    result = std::regex_replace(result, std::regex("(\\w+)\\?\\s*:\\s*[\\w<>\\[\\]|&\\s]+(?=[,)])"), "$1");

    // Remove parameter type annotations: (param: Type) -> (param)
    result = std::regex_replace(result, std::regex("(\\w+)\\s*:\\s*[\\w<>\\[\\]|&\\s]+(?=[,)])"), "$1");

    // Remove return type annotations: ): Type { -> ) {
    result = std::regex_replace(result, std::regex("\\)\\s*:\\s*[\\w<>\\[\\]|&\\s]+\\s*(?=\\{)"), ") ");
    result = std::regex_replace(result, std::regex("\\)\\s*:\\s*[\\w<>\\[\\]|&\\s]+\\s*(?==>)"), ") ");

    // Remove variable type annotations: let x: Type = -> let x =
    result = std::regex_replace(result, std::regex("(let|const|var)\\s+(\\w+)\\s*:\\s*[\\w<>\\[\\]|&\\s]+\\s*="), "$1 $2 =");

    // Remove optional property type annotations: name?: Type; -> name;
    result = std::regex_replace(result, std::regex("(\\w+)\\?\\s*:\\s*[\\w<>\\[\\]|&\\s]+\\s*;"), "$1;");
    result = std::regex_replace(result, std::regex("(\\w+)\\?\\s*:\\s*[\\w<>\\[\\]|&\\s]+\\s*="), "$1 =");

    // Remove property type annotations in classes: name: string; -> name;
    result = std::regex_replace(result, std::regex("(\\w+)\\s*:\\s*[\\w<>\\[\\]|&\\s]+\\s*;"), "$1;");
    result = std::regex_replace(result, std::regex("(\\w+)\\s*:\\s*[\\w<>\\[\\]|&\\s]+\\s*="), "$1 =");

    // Remove type assertions: <Type>value and value as Type
    result = std::regex_replace(result, std::regex("<[\\w<>\\[\\]|&\\s]+>(?=\\w)"), "");
    result = std::regex_replace(result, std::regex("\\s+as\\s+[\\w<>\\[\\]|&\\s]+"), "");

    // Remove generic type parameters from functions: function<T>( -> function(
    result = std::regex_replace(result, std::regex("(function\\s*\\w*)\\s*<[^>]+>\\s*\\("), "$1(");

    // Remove generic type parameters from arrow functions
    result = std::regex_replace(result, std::regex("<[^>]+>\\s*(?=\\([^)]*\\)\\s*=>)"), "");

    // Remove class generic parameters: class Foo<T> -> class Foo
    result = std::regex_replace(result, std::regex("(class\\s+\\w+)\\s*<[^>]+>"), "$1");

    // Remove generic type in new expression: new Foo<T>(...) -> new Foo(...)
    result = std::regex_replace(result, std::regex("new\\s+(\\w+)\\s*<[^>]+>\\s*\\("), "new $1(");

    // Remove implements/extends type constraints
    result = std::regex_replace(result, std::regex("\\s+implements\\s+[\\w<>,\\s]+(?=\\s*\\{)"), "");

    // Remove readonly modifier
    result = std::regex_replace(result, std::regex("\\breadonly\\s+"), "");

    // Remove public/private/protected modifiers
    result = std::regex_replace(result, std::regex("\\b(public|private|protected)\\s+"), "");

    // Remove abstract modifier
    result = std::regex_replace(result, std::regex("\\babstract\\s+"), "");

    // Remove declare statements
    result = std::regex_replace(result, std::regex("declare\\s+[^;]+;"), "");
    result = std::regex_replace(result, std::regex("declare\\s+(function|class|const|let|var|enum|interface|type|namespace|module)[^{;]+[{;]"), "");

    // Remove namespace/module declarations (keep content)
    result = std::regex_replace(result, std::regex("namespace\\s+\\w+\\s*\\{"), "{");
    result = std::regex_replace(result, std::regex("module\\s+\\w+\\s*\\{"), "{");

    // Remove non-null assertions: value! -> value
    result = std::regex_replace(result, std::regex("(\\w+)!(?=[^=])"), "$1");

    // Remove definite assignment assertions: name!: -> name
    result = std::regex_replace(result, std::regex("(\\w+)!\\s*:"), "$1:");

    // Remove satisfies expression
    result = std::regex_replace(result, std::regex("\\s+satisfies\\s+[\\w<>\\[\\]|&\\s]+"), "");

    return result;
}

std::string Transpiler::transformJSX(const std::string& source) {
    const auto& opts = config_.compilerOptions;

    if (opts.jsx == "preserve" || opts.jsx == "react-native") {
        // Keep JSX as-is
        return source;
    }

    std::string createElement = opts.jsxFactory;
    std::string fragmentType = opts.jsxFragmentFactory;
    const bool automatic =
        opts.jsx == "react-jsx" || opts.jsx == "react-jsxdev";
    if (automatic) {
        createElement = "_jsx";
        fragmentType = "_Fragment";
    }
    bool changed = false;
    JSXTransformer transformer(
        source, createElement, fragmentType, automatic);
    std::string result = transformer.transform(changed);
    if (automatic && changed) {
        const bool commonJS =
            opts.module == "commonjs" || opts.module == "CommonJS";
        if (commonJS) {
            result =
                "const { jsx: _jsx, jsxs: _jsxs, Fragment: _Fragment } = "
                "require(\"" + opts.jsxImportSource +
                "/jsx-runtime\");\n" + result;
        } else {
            result =
                "import { jsx as _jsx, jsxs as _jsxs, "
                "Fragment as _Fragment } from \"" +
                opts.jsxImportSource + "/jsx-runtime\";\n" + result;
        }
    }
    return result;
}

std::string Transpiler::transformPaths(
    const std::string& source, const std::string& filename) {
    const auto& opts = config_.compilerOptions;

    if (opts.paths.empty() && !opts.rewriteRelativeImportExtensions) {
        return source;
    }
    const auto rewrite = [&](const std::string& specifier) {
        for (const auto& [pattern, replacements] : opts.paths) {
            if (replacements.empty()) continue;
            const size_t star = pattern.find('*');
            std::string wildcard;
            if (star == std::string::npos) {
                if (specifier != pattern) continue;
            } else {
                const std::string prefix = pattern.substr(0, star);
                const std::string suffix = pattern.substr(star + 1);
                if (specifier.size() < prefix.size() + suffix.size() ||
                    specifier.compare(0, prefix.size(), prefix) != 0 ||
                    specifier.compare(
                        specifier.size() - suffix.size(),
                        suffix.size(), suffix) != 0) {
                    continue;
                }
                wildcard = specifier.substr(
                    prefix.size(),
                    specifier.size() - prefix.size() - suffix.size());
            }
            std::string replacement = replacements.front();
            const size_t replacementStar = replacement.find('*');
            if (replacementStar != std::string::npos) {
                replacement.replace(replacementStar, 1, wildcard);
            }
            std::filesystem::path base = opts.baseUrl.empty()
                ? std::filesystem::path(configDir_)
                : std::filesystem::path(
                    resolveConfigRelativePath(opts.baseUrl));
            std::filesystem::path targetSource = base / replacement;
            if (!targetSource.has_extension()) targetSource += ".ts";
            std::filesystem::path targetOutput =
                resolveOutputPath(targetSource.string(), ".js");
            std::filesystem::path importerOutput =
                resolveOutputPath(filename, ".js");
            std::filesystem::path relative = std::filesystem::relative(
                targetOutput, importerOutput.parent_path());
            if (!opts.rewriteRelativeImportExtensions) {
                relative.replace_extension();
            }
            std::string resolved = relative.generic_string();
            if (!resolved.empty() && resolved[0] != '.') resolved = "./" + resolved;
            return resolved;
        }
        if (opts.rewriteRelativeImportExtensions &&
            (specifier.rfind("./", 0) == 0 ||
             specifier.rfind("../", 0) == 0)) {
            std::filesystem::path rewritten(specifier);
            const std::string extension = rewritten.extension().string();
            if (extension == ".ts" || extension == ".tsx" ||
                extension == ".mts" || extension == ".cts") {
                rewritten.replace_extension(".js");
                return rewritten.generic_string();
            }
        }
        return specifier;
    };

    std::string result;
    for (size_t position = 0; position < source.size();) {
        if (source[position] != '"' && source[position] != '\'') {
            result += source[position++];
            continue;
        }
        const char delimiter = source[position];
        const size_t valueStart = ++position;
        while (position < source.size() &&
               source[position] != delimiter) {
            if (source[position] == '\\' && position + 1 < source.size()) {
                position += 2;
            } else {
                ++position;
            }
        }
        const std::string value =
            source.substr(valueStart, position - valueStart);
        result += delimiter;
        result += rewrite(value);
        if (position < source.size()) result += source[position++];
    }
    return result;
}

std::string Transpiler::transformImports(const std::string& source) {
    std::string result = source;
    const auto& opts = config_.compilerOptions;

    // Remove import type statements (they have no runtime effect)
    result = std::regex_replace(result, std::regex("import\\s+type\\s+[^;]+;"), "");
    result = std::regex_replace(result, std::regex("import\\s*\\{\\s*type\\s+[^}]+\\}\\s*from\\s*[^;]+;"), "");

    // Check module format
    bool isCommonJS = (opts.module == "commonjs" || opts.module == "CommonJS");
    bool isES = (opts.module == "es6" || opts.module == "es2015" || opts.module == "es2020" ||
                 opts.module == "es2022" || opts.module == "esnext" || opts.module == "ESNext");

    if (isES) {
        // Keep ES modules as-is, just clean up type imports
        return result;
    }

    if (isCommonJS) {
        // Transform ES6 imports to CommonJS require

        // import { a, b } from 'module' -> const { a, b } = require('module')
        result = std::regex_replace(result,
            std::regex("import\\s*\\{([^}]+)\\}\\s*from\\s*['\"]([^'\"]+)['\"]"),
            "const {$1} = require(\"$2\")");

        // import * as name from 'module' -> const name = require('module')
        result = std::regex_replace(result,
            std::regex("import\\s*\\*\\s*as\\s+(\\w+)\\s*from\\s*['\"]([^'\"]+)['\"]"),
            "const $1 = require(\"$2\")");

        // import name from 'module' -> const name = require('module').default || require('module')
        if (opts.esModuleInterop) {
            result = std::regex_replace(result,
                std::regex("import\\s+(\\w+)\\s*from\\s*['\"]([^'\"]+)['\"]"),
                "const $1 = require(\"$2\").default || require(\"$2\")");
        } else {
            result = std::regex_replace(result,
                std::regex("import\\s+(\\w+)\\s*from\\s*['\"]([^'\"]+)['\"]"),
                "const $1 = require(\"$2\")");
        }

        // import 'module' -> require('module')
        result = std::regex_replace(result,
            std::regex("import\\s*['\"]([^'\"]+)['\"]"),
            "require(\"$1\")");

        // Handle JSON imports if enabled
        if (opts.resolveJsonModule) {
            // Already handled by require()
        }
    }

    return result;
}

std::string Transpiler::transformExports(const std::string& source) {
    std::string result = source;
    std::vector<std::string> exportedNames;
    const auto& opts = config_.compilerOptions;

    bool isCommonJS = (opts.module == "commonjs" || opts.module == "CommonJS");
    bool isES = (opts.module == "es6" || opts.module == "es2015" || opts.module == "es2020" ||
                 opts.module == "es2022" || opts.module == "esnext" || opts.module == "ESNext");

    if (isES) {
        // Keep ES exports, just handle enum transformation
        // Transform enum member syntax: Key = "value" -> Key: "value"
        result = std::regex_replace(result,
            std::regex("(\\n\\s*)(\\w+)\\s*=\\s*(\"[^\"]*\")"),
            "$1$2: $3");
        result = std::regex_replace(result,
            std::regex("(\\n\\s*)(\\w+)\\s*=\\s*('[^']*')"),
            "$1$2: $3");
        result = std::regex_replace(result,
            std::regex("(\\n\\s*)(\\w+)\\s*=\\s*(\\d+)\\s*([,}])"),
            "$1$2: $3$4");

        // export enum Name { ... } -> export const Name = { ... };
        result = std::regex_replace(result,
            std::regex("export\\s+enum\\s+(\\w+)\\s*\\{([^}]*)\\}"),
            "export const $1 = {$2};");

        // enum Name { ... } -> const Name = { ... };
        result = std::regex_replace(result,
            std::regex("\\benum\\s+(\\w+)\\s*\\{([^}]*)\\}"),
            "const $1 = {$2};");

        return result;
    }

    if (isCommonJS) {
        // Collect exported names first

        // Find export const/let/var names
        std::regex varExportRegex("export\\s+(?:const|let|var)\\s+(\\w+)");
        std::sregex_iterator varIt(source.begin(), source.end(), varExportRegex);
        std::sregex_iterator endIt;
        while (varIt != endIt) {
            exportedNames.push_back((*varIt)[1].str());
            ++varIt;
        }

        // Find export function names
        std::regex funcExportRegex("export\\s+(?:async\\s+)?function\\s+(\\w+)");
        std::sregex_iterator funcIt(source.begin(), source.end(), funcExportRegex);
        while (funcIt != endIt) {
            exportedNames.push_back((*funcIt)[1].str());
            ++funcIt;
        }

        // Find export class names
        std::regex classExportRegex("export\\s+class\\s+(\\w+)");
        std::sregex_iterator classIt(source.begin(), source.end(), classExportRegex);
        while (classIt != endIt) {
            exportedNames.push_back((*classIt)[1].str());
            ++classIt;
        }

        // Find export enum names
        std::regex enumExportRegex("export\\s+enum\\s+(\\w+)");
        std::sregex_iterator enumIt(source.begin(), source.end(), enumExportRegex);
        while (enumIt != endIt) {
            exportedNames.push_back((*enumIt)[1].str());
            ++enumIt;
        }

        // Transform enum member syntax FIRST: Key = "value" -> Key: "value"
        result = std::regex_replace(result,
            std::regex("(\\n\\s*)(\\w+)\\s*=\\s*(\"[^\"]*\")"),
            "$1$2: $3");
        result = std::regex_replace(result,
            std::regex("(\\n\\s*)(\\w+)\\s*=\\s*('[^']*')"),
            "$1$2: $3");
        result = std::regex_replace(result,
            std::regex("(\\n\\s*)(\\w+)\\s*=\\s*(\\d+)\\s*([,}])"),
            "$1$2: $3$4");

        // Transform ES6 exports to CommonJS

        // export default value -> module.exports = value
        result = std::regex_replace(result,
            std::regex("export\\s+default\\s+"),
            "module.exports = ");

        // export { a, b } -> module.exports = { a, b }
        result = std::regex_replace(result,
            std::regex("export\\s*\\{([^}]+)\\}\\s*;?"),
            "module.exports = {$1};");

        // export const/let/var name = value -> const/let/var name = value
        result = std::regex_replace(result,
            std::regex("export\\s+(const|let|var)\\s+(\\w+)"),
            "$1 $2");

        // export async function name -> async function name
        result = std::regex_replace(result,
            std::regex("export\\s+async\\s+function\\s+(\\w+)"),
            "async function $1");

        // export function name -> function name
        result = std::regex_replace(result,
            std::regex("export\\s+function\\s+(\\w+)"),
            "function $1");

        // export class name -> class name
        result = std::regex_replace(result,
            std::regex("export\\s+class\\s+(\\w+)"),
            "class $1");

        // export enum Name { ... } -> const Name = { ... };
        result = std::regex_replace(result,
            std::regex("export\\s+enum\\s+(\\w+)\\s*\\{([^}]*)\\}"),
            "const $1 = {$2};");

        // Non-exported enum: enum Name { ... } -> const Name = { ... };
        result = std::regex_replace(result,
            std::regex("\\benum\\s+(\\w+)\\s*\\{([^}]*)\\}"),
            "const $1 = {$2};");

        // Add module.exports at the end for collected exports
        if (!exportedNames.empty()) {
            result += "\n\nmodule.exports = { ";
            for (size_t i = 0; i < exportedNames.size(); ++i) {
                if (i > 0) result += ", ";
                result += exportedNames[i];
            }
            result += " };\n";
        }
    }

    return result;
}

std::string Transpiler::downlevelToTarget(const std::string& source) {
    std::string result = source;
    const auto& opts = config_.compilerOptions;

    // ES5/ES3 transformations
    if (opts.target == "es5" || opts.target == "es3") {
        // Transform arrow functions: (x) => x -> function(x) { return x; }
        result = std::regex_replace(result,
            std::regex("\\(([^)]*)\\)\\s*=>\\s*([^{][^;\\n]*)"),
            "function($1) { return $2; }");

        // Transform arrow functions with block: (x) => { ... } -> function(x) { ... }
        result = std::regex_replace(result,
            std::regex("\\(([^)]*)\\)\\s*=>\\s*\\{"),
            "function($1) {");

        // Transform template literals: `${x}` -> "" + x + ""
        // This is a simplified transformation
        result = std::regex_replace(result,
            std::regex("`([^`]*)`"),
            "\"$1\"");

        // Transform let/const to var
        result = std::regex_replace(result, std::regex("\\blet\\s+"), "var ");
        result = std::regex_replace(result, std::regex("\\bconst\\s+"), "var ");

        // Transform class to function (simplified)
        // Full class transformation would be more complex
    }

    return result;
}

std::string Transpiler::minifyCode(const std::string& source) {
    std::string result = source;

    // Remove multi-line whitespace (replace newlines with single space)
    result = std::regex_replace(result, std::regex("\\n+"), " ");

    // Collapse multiple spaces to one
    result = std::regex_replace(result, std::regex("  +"), " ");

    // Remove spaces around brackets and punctuation
    result = std::regex_replace(result, std::regex("\\s*\\(\\s*"), "(");
    result = std::regex_replace(result, std::regex("\\s*\\)\\s*"), ")");
    result = std::regex_replace(result, std::regex("\\s*\\[\\s*"), "[");
    result = std::regex_replace(result, std::regex("\\s*\\]\\s*"), "]");
    result = std::regex_replace(result, std::regex("\\s*;\\s*"), ";");
    result = std::regex_replace(result, std::regex("\\s*,\\s*"), ",");

    // Handle braces
    result = std::regex_replace(result, std::regex("\\s*\\{\\s*"), "{");
    result = std::regex_replace(result, std::regex("\\s*\\}\\s*"), "}");

    // Remove spaces around operators
    result = std::regex_replace(result, std::regex("\\s*=\\s*"), "=");
    result = std::regex_replace(result, std::regex("\\s*\\+\\s*"), "+");
    result = std::regex_replace(result, std::regex("\\s*-\\s*"), "-");
    result = std::regex_replace(result, std::regex("\\s*\\*\\s*"), "*");
    result = std::regex_replace(result, std::regex("\\s*/\\s*"), "/");
    result = std::regex_replace(result, std::regex("\\s*<\\s*"), "<");
    result = std::regex_replace(result, std::regex("\\s*>\\s*"), ">");

    // Remove trailing semicolons before closing brace
    result = std::regex_replace(result, std::regex(";\\}"), "}");

    // Trim leading/trailing whitespace
    size_t start = result.find_first_not_of(" \t\n\r");
    size_t end = result.find_last_not_of(" \t\n\r");
    if (start != std::string::npos && end != std::string::npos) {
        result = result.substr(start, end - start + 1);
    }

    return result;
}

std::string Transpiler::generateSourceMap(const std::string& source, const std::string& output, const std::string& filename) {
    const auto& opts = config_.compilerOptions;

    // Generate a proper source map
    std::stringstream ss;
    ss << "{\n";
    ss << "  \"version\": 3,\n";
    ss << "  \"file\": \"" << std::filesystem::path(filename).stem().string() << ".js\",\n";

    // Source root
    if (!opts.sourceRoot.empty()) {
        ss << "  \"sourceRoot\": \"" << jsonEscape(opts.sourceRoot)
           << "\",\n";
    }

    ss << "  \"sources\": [\"" << jsonEscape(filename) << "\"],\n";

    // Include sources if requested
    if (opts.inlineSources) {
        ss << "  \"sourcesContent\": [\"" << jsonEscape(source)
           << "\"],\n";
    }

    ss << "  \"names\": [],\n";

    // Generate basic mappings (line-by-line)
    // This is a simplified implementation - real source maps need VLQ encoding
    ss << "  \"mappings\": \"";

    // Count lines in output
    int lineCount = 1;
    for (char c : output) {
        if (c == '\n') lineCount++;
    }

    // Generate simple 1:1 line mapping
    for (int i = 0; i < lineCount; i++) {
        if (i > 0) ss << ";";
        ss << "AAAA"; // Simple identity mapping
    }

    ss << "\"\n";
    ss << "}\n";

    return ss.str();
}

std::string Transpiler::generateDeclaration(const std::string& source) {
    std::string result;
    std::set<std::string> emittedFunctions;
    std::set<std::string> emittedVariables;

    // Extract interfaces (keep them as-is for .d.ts)
    std::regex interfaceRegex("((?:export\\s+)?interface\\s+\\w+\\s*(?:<[^>]*>)?\\s*(?:extends\\s+[^{]+)?\\s*\\{[^}]*\\})");
    std::sregex_iterator it(source.begin(), source.end(), interfaceRegex);
    std::sregex_iterator end;
    while (it != end) {
        result += it->str() + "\n\n";
        ++it;
    }

    // Extract type aliases
    std::regex typeRegex("((?:export\\s+)?type\\s+\\w+\\s*(?:<[^>]*>)?\\s*=\\s*[^;]+;)");
    it = std::sregex_iterator(source.begin(), source.end(), typeRegex);
    while (it != end) {
        result += it->str() + "\n";
        ++it;
    }

    // Extract exported function signatures
    std::regex funcRegex(
        "export\\s+((?:async\\s+)?function\\s+(\\w+)\\s*"
        "(?:<[^>]*>)?\\s*\\([^)]*\\)\\s*:\\s*"
        "[\\w<>\\[\\]|&.,\\s]+)");
    it = std::sregex_iterator(source.begin(), source.end(), funcRegex);
    while (it != end) {
        result += "export declare " + it->str(1) + ";\n";
        emittedFunctions.insert(it->str(2));
        ++it;
    }
    std::regex inferredFuncRegex(
        "export\\s+((?:async\\s+)?function\\s+(\\w+)\\s*"
        "((?:<[^>]*>)?)\\s*\\(([^)]*)\\))\\s*\\{");
    it = std::sregex_iterator(
        source.begin(), source.end(), inferredFuncRegex);
    while (it != end) {
        if (emittedFunctions.insert(it->str(2)).second) {
            result += "export declare " + it->str(1) + ": unknown;\n";
        }
        ++it;
    }

    // Extract exported class declarations
    std::regex classRegex("export\\s+(class\\s+\\w+\\s*(?:<[^>]*>)?\\s*(?:extends\\s+[^{]+)?\\s*(?:implements\\s+[^{]+)?\\s*)\\{");
    it = std::sregex_iterator(source.begin(), source.end(), classRegex);
    while (it != end) {
        result += "export declare " + it->str(1) + "{ }\n";
        ++it;
    }

    // Extract exported const/let declarations
    std::regex varRegex("export\\s+(const|let)\\s+(\\w+)\\s*:\\s*([^=;]+)");
    it = std::sregex_iterator(source.begin(), source.end(), varRegex);
    while (it != end) {
        result += "export declare " + it->str(1) + " " + it->str(2) + ": " + it->str(3) + ";\n";
        emittedVariables.insert(it->str(2));
        ++it;
    }
    std::regex inferredVarRegex(
        "export\\s+(const|let)\\s+(\\w+)\\s*=\\s*([^;]+);");
    it = std::sregex_iterator(source.begin(), source.end(), inferredVarRegex);
    while (it != end) {
        const std::string name = it->str(2);
        if (emittedVariables.insert(name).second) {
            std::string initializer = it->str(3);
            initializer.erase(0, initializer.find_first_not_of(" \t\r\n"));
            std::string inferred = "unknown";
            if (!initializer.empty() &&
                (initializer.front() == '"' || initializer.front() == '\'' ||
                 initializer.front() == '`')) {
                inferred = "string";
            } else if (initializer == "true" || initializer == "false") {
                inferred = "boolean";
            } else if (std::regex_match(
                           initializer,
                           std::regex("[+-]?[0-9]+(?:\\.[0-9]+)?"))) {
                inferred = "number";
            }
            result += "export declare " + it->str(1) + " " + name +
                ": " + inferred + ";\n";
        }
        ++it;
    }

    // Extract enums
    std::regex enumRegex("((?:export\\s+)?enum\\s+\\w+\\s*\\{[^}]*\\})");
    it = std::sregex_iterator(source.begin(), source.end(), enumRegex);
    while (it != end) {
        std::string enumStr = it->str();
        if (enumStr.find("export") == std::string::npos) {
            result += "declare " + enumStr + "\n";
        } else {
            result += enumStr.substr(0, 6) + " declare" + enumStr.substr(6) + "\n";
        }
        ++it;
    }

    return result;
}

std::string Transpiler::generateDeclarationMap([[maybe_unused]] const std::string& source, const std::string& dtsCode, const std::string& filename) {
    std::stringstream ss;
    std::filesystem::path srcPath(filename);

    // Extract just the filename for the source map
    std::string srcFile = srcPath.filename().string();
    std::string dtsFile = srcPath.stem().string() + ".d.ts";

    ss << "{\n";
    ss << "  \"version\": 3,\n";
    ss << "  \"file\": \"" << dtsFile << "\",\n";
    ss << "  \"sources\": [\"" << jsonEscape(srcFile) << "\"],\n";
    ss << "  \"names\": [],\n";

    // Generate simple line mappings from .d.ts to source
    // Each line in .d.ts maps to corresponding source location
    std::string mappings;
    int lineCount = std::count(dtsCode.begin(), dtsCode.end(), '\n') + 1;
    for (int i = 0; i < lineCount; i++) {
        if (i > 0) mappings += ";";
        mappings += "AAAA";  // Simple 1:1 mapping
    }

    ss << "  \"mappings\": \"" << mappings << "\"\n";
    ss << "}";

    return ss.str();
}

// ============================================================================
// File Discovery
// ============================================================================

std::vector<std::string> Transpiler::findSourceFiles(const std::string& projectPath) {
    std::vector<std::string> files;
    const auto& opts = config_.compilerOptions;

    std::filesystem::path root(projectPath);
    if (!std::filesystem::exists(root)) {
        return files;
    }

    // If explicit files are specified, use those
    if (!config_.files.empty()) {
        for (const auto& file : config_.files) {
            std::filesystem::path filePath = root / file;
            if (std::filesystem::exists(filePath)) {
                files.push_back(filePath.string());
            }
        }
        return files;
    }

    for (const auto& entry : std::filesystem::recursive_directory_iterator(root)) {
        if (!entry.is_regular_file()) continue;

        std::string path = entry.path().string();
        std::string ext = entry.path().extension().string();

        // Check file extension
        bool isTS = (ext == ".ts" || ext == ".tsx" ||
                     ext == ".mts" || ext == ".cts");
        bool isJS = (ext == ".js" || ext == ".jsx" ||
                     ext == ".mjs" || ext == ".cjs");

        if (!isTS && !(opts.allowJs && isJS)) continue;

        // Skip .d.ts files
        if (path.size() > 5 && path.substr(path.size() - 5) == ".d.ts") continue;

        // Check if matches include patterns
        bool included = config_.include.empty();
        for (const auto& pattern : config_.include) {
            if (matchesGlob(path, pattern)) {
                included = true;
                break;
            }
        }

        if (!included) continue;

        // Check if matches exclude patterns
        bool excluded = false;
        for (const auto& pattern : config_.exclude) {
            if (matchesGlob(path, pattern)) {
                excluded = true;
                break;
            }
        }

        if (excluded) continue;

        files.push_back(path);
    }

    return files;
}

bool Transpiler::matchesGlob(const std::string& path, const std::string& pattern) {
    // Normalize path separators to forward slash for matching
    std::string normalizedPath = path;
    std::replace(normalizedPath.begin(), normalizedPath.end(), '\\', '/');

    // Common patterns
    if (pattern == "**/*.ts" || pattern == "./**/*.ts") {
        return normalizedPath.size() > 3 && normalizedPath.substr(normalizedPath.size() - 3) == ".ts" &&
               normalizedPath.substr(normalizedPath.size() - 5) != ".d.ts";
    }
    if (pattern == "**/*.tsx" || pattern == "./**/*.tsx") {
        return normalizedPath.size() > 4 && normalizedPath.substr(normalizedPath.size() - 4) == ".tsx";
    }
    if (pattern == "**/*.js" || pattern == "./**/*.js") {
        return normalizedPath.size() > 3 && normalizedPath.substr(normalizedPath.size() - 3) == ".js";
    }
    if (pattern == "**/*.jsx" || pattern == "./**/*.jsx") {
        return normalizedPath.size() > 4 && normalizedPath.substr(normalizedPath.size() - 4) == ".jsx";
    }
    if (pattern == "node_modules" || pattern == "**/node_modules" || pattern == "**/node_modules/**") {
        return normalizedPath.find("node_modules") != std::string::npos;
    }
    if (pattern == "**/*.d.ts") {
        return normalizedPath.size() > 5 && normalizedPath.substr(normalizedPath.size() - 5) == ".d.ts";
    }
    if (pattern == "dist" || pattern == "**/dist" || pattern == "**/dist/**" || pattern == "./dist") {
        return normalizedPath.find("/dist/") != std::string::npos ||
               normalizedPath.find("/dist") == normalizedPath.size() - 5;
    }

    // Handle patterns like "src/**/*.ts" - directory + glob + extension
    size_t starPos = pattern.find("**");
    if (starPos != std::string::npos) {
        std::string prefix = pattern.substr(0, starPos);  // "src/"
        std::string suffix = pattern.substr(starPos + 2); // "/*.ts"

        // Check prefix (directory part)
        bool hasPrefix = prefix.empty() || normalizedPath.find(prefix) != std::string::npos;

        // Check suffix (extension part like "/*.ts")
        bool hasSuffix = true;
        if (!suffix.empty()) {
            if (suffix[0] == '/') suffix = suffix.substr(1); // Remove leading /
            if (suffix[0] == '*') suffix = suffix.substr(1); // Remove *
            // suffix is now ".ts" or ".tsx" etc
            hasSuffix = normalizedPath.size() >= suffix.size() &&
                       normalizedPath.substr(normalizedPath.size() - suffix.size()) == suffix;
            // Skip .d.ts if pattern is for .ts
            if (suffix == ".ts" && normalizedPath.size() > 5 &&
                normalizedPath.substr(normalizedPath.size() - 5) == ".d.ts") {
                hasSuffix = false;
            }
        }

        return hasPrefix && hasSuffix;
    }

    // Check for simple directory pattern like "src/**"
    if (pattern.size() > 3 && pattern.substr(pattern.size() - 3) == "/**") {
        std::string prefix = pattern.substr(0, pattern.size() - 3);
        return normalizedPath.find(prefix) != std::string::npos;
    }

    // Direct match or contains
    return normalizedPath.find(pattern) != std::string::npos;
}

std::string Transpiler::resolveOutputPath(const std::string& inputPath, const std::string& ext) {
    const auto& opts = config_.compilerOptions;
    std::filesystem::path input(inputPath);
    std::filesystem::path output;

    std::string outDir = resolveConfigRelativePath(
        opts.outDir.empty() ? "." : opts.outDir);

    if (!opts.rootDir.empty()) {
        // Preserve directory structure relative to rootDir
        std::filesystem::path rootDir(
            resolveConfigRelativePath(opts.rootDir));
        std::filesystem::path relativePath = std::filesystem::relative(input.parent_path(), rootDir);
        output = std::filesystem::path(outDir) / relativePath / (input.stem().string() + ext);
    } else {
        // Flat output
        output = std::filesystem::path(outDir) / (input.stem().string() + ext);
    }

    return output.string();
}

std::string Transpiler::resolveConfigRelativePath(
    const std::string& path) const {
    std::filesystem::path candidate(path.empty() ? "." : path);
    if (candidate.is_absolute() || configDir_.empty()) {
        return candidate.lexically_normal().string();
    }
    return (std::filesystem::path(configDir_) / candidate)
        .lexically_normal().string();
}

// ============================================================================
// Build
// ============================================================================

BuildResult Transpiler::build(const std::string& projectPath) {
    auto startTime = std::chrono::high_resolution_clock::now();

    BuildResult result;
    result.success = true;
    result.totalFiles = 0;
    result.successCount = 0;
    result.failCount = 0;
    result.totalInputSize = 0;
    result.totalOutputSize = 0;

    const auto& opts = config_.compilerOptions;

    // Build project references first (for composite projects)
    if (!config_.references.empty()) {
        std::cout << "[Build] Building " << config_.references.size() << " referenced project(s)..." << std::endl;

        for (const auto& ref : config_.references) {
            // Resolve reference path relative to current project
            std::filesystem::path refPath = std::filesystem::path(configDir_) / ref.path;
            std::string refConfigPath = (refPath / "tsconfig.json").string();

            if (std::filesystem::exists(refConfigPath)) {
                std::cout << "[Build] Building reference: " << ref.path << std::endl;

                // Create a new transpiler for the referenced project
                Transpiler refTranspiler;
                if (refTranspiler.loadConfig(refConfigPath)) {
                    auto refResult = refTranspiler.build(refPath.string());
                    if (!refResult.success) {
                        result.success = false;
                        result.errors.push_back("Referenced project failed: " + ref.path);
                        for (const auto& err : refResult.errors) {
                            result.errors.push_back("  " + err);
                        }
                        return result;
                    }
                    std::cout << "[Build] Reference built: " << ref.path
                              << " (" << refResult.successCount << " files)" << std::endl;
                } else {
                    std::cerr << "[WARN] Could not load referenced config: " << refConfigPath << std::endl;
                }
            } else {
                std::cerr << "[WARN] Referenced project not found: " << ref.path << std::endl;
            }
        }
    }

    // Load build cache if incremental
    if (opts.incremental) {
        loadBuildInfo();
    }

    // Find all source files
    std::vector<std::string> files = findSourceFiles(projectPath);
    result.totalFiles = static_cast<int>(files.size());

    if (files.empty()) {
        result.errors.push_back("No TypeScript files found");
        result.success = false;
        return result;
    }

    // Create output directory
    std::string outDir = resolveConfigRelativePath(
        opts.outDir.empty() ? "." : opts.outDir);
    std::filesystem::create_directories(outDir);

    // Create declaration directory if different
    if (!opts.declarationDir.empty() && opts.declarationDir != outDir) {
        std::filesystem::create_directories(
            resolveConfigRelativePath(opts.declarationDir));
    }

    // Filter files for incremental build
    std::vector<std::string> filesToBuild;
    if (opts.incremental) {
        for (const auto& file : files) {
            if (needsRebuild(file)) {
                filesToBuild.push_back(file);
            }
        }
    } else {
        filesToBuild = files;
    }

    // Transpile files in parallel
    std::vector<std::future<TranspileResult>> futures;
    for (const auto& file : filesToBuild) {
        futures.push_back(std::async(std::launch::async, [this, file]() {
            return this->transpileFile(file);
        }));
    }

    // Collect results
    bool hasErrors = false;
    for (auto& future : futures) {
        TranspileResult fileResult = future.get();
        result.files.push_back(fileResult);

        if (fileResult.success) {
            result.successCount++;
            result.totalInputSize += fileResult.inputSize;
            result.totalOutputSize += fileResult.outputSize;

            // Don't write if noEmit
            if (opts.noEmit) continue;

            // Write output file
            std::string jsPath = resolveOutputPath(fileResult.filename, ".js");

            // Create subdirectories if needed
            std::filesystem::create_directories(std::filesystem::path(jsPath).parent_path());

            if (!opts.emitDeclarationOnly && !fileResult.jsCode.empty()) {
                std::ofstream outFile(jsPath);
                if (outFile.is_open()) {
                    outFile << fileResult.jsCode;
                    outFile.close();
                }
            }

            // Write declaration file if generated
            if (!fileResult.dtsCode.empty()) {
                std::string dtsPath = resolveOutputPath(fileResult.filename, ".d.ts");
                if (!opts.declarationDir.empty()) {
                    std::filesystem::path input(fileResult.filename);
                    std::filesystem::path root = opts.rootDir.empty()
                        ? std::filesystem::path(configDir_)
                        : std::filesystem::path(
                            resolveConfigRelativePath(opts.rootDir));
                    std::filesystem::path relative =
                        std::filesystem::relative(input.parent_path(), root);
                    dtsPath =
                        (std::filesystem::path(resolveConfigRelativePath(
                             opts.declarationDir)) /
                         relative /
                         (input.stem().string() + ".d.ts")).string();
                }

                std::filesystem::create_directories(std::filesystem::path(dtsPath).parent_path());
                std::ofstream dtsFile(dtsPath);
                if (dtsFile.is_open()) {
                    dtsFile << fileResult.dtsCode;
                    dtsFile.close();
                }

                // Write declaration map if generated
                if (!fileResult.declarationMap.empty()) {
                    std::string dtsMapPath = dtsPath + ".map";
                    std::ofstream dtsMapFile(dtsMapPath);
                    if (dtsMapFile.is_open()) {
                        dtsMapFile << fileResult.declarationMap;
                        dtsMapFile.close();
                    }
                }
            }

            // Write source map if generated
            if (!fileResult.sourceMap.empty()) {
                std::string mapPath = jsPath + ".map";
                std::ofstream mapFile(mapPath);
                if (mapFile.is_open()) {
                    mapFile << fileResult.sourceMap;
                    mapFile.close();
                }
            }

            // Update build cache
            if (opts.incremental) {
                buildCache_.fileModTimes[fileResult.filename] =
                    std::filesystem::last_write_time(fileResult.filename);
            }
        } else {
            result.failCount++;
            hasErrors = true;
            for (const auto& error : fileResult.errors) {
                result.errors.push_back(fileResult.filename + ": " + error);
            }
        }
    }

    // Handle noEmitOnError
    if (opts.noEmitOnError && hasErrors) {
        // Would need to delete emitted files, but let's just mark as failed
        result.success = false;
    } else {
        result.success = (result.failCount == 0);
    }

    // Save build info for incremental builds
    if (opts.incremental) {
        saveBuildInfo();
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    result.totalTimeMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();

    return result;
}

// ============================================================================
// Incremental Build Helpers
// ============================================================================

bool Transpiler::needsRebuild(const std::string& filePath) {
    if (!buildCache_.isValid) return true;

    auto it = buildCache_.fileModTimes.find(filePath);
    if (it == buildCache_.fileModTimes.end()) return true;

    auto currentModTime = std::filesystem::last_write_time(filePath);
    return currentModTime > it->second;
}

void Transpiler::saveBuildInfo() {
    const auto& opts = config_.compilerOptions;
    std::string infoPath = opts.tsBuildInfoFile.empty()
        ? (std::filesystem::path(resolveConfigRelativePath(
               opts.outDir.empty() ? "." : opts.outDir)) /
           ".tsbuildinfo").string()
        : resolveConfigRelativePath(opts.tsBuildInfoFile);

    std::ofstream file(infoPath);
    if (file.is_open()) {
        file << "{\n  \"version\": \"nova-1.0\",\n  \"files\": {\n";
        bool first = true;
        for (const auto& [path, time] : buildCache_.fileModTimes) {
            if (!first) file << ",\n";
            first = false;
            auto timeT = std::chrono::system_clock::to_time_t(
                std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                    time - std::filesystem::file_time_type::clock::now() +
                    std::chrono::system_clock::now()));
            file << "    \"" << path << "\": " << timeT;
        }
        file << "\n  }\n}\n";
        file.close();
    }
}

void Transpiler::loadBuildInfo() {
    const auto& opts = config_.compilerOptions;
    std::string infoPath = opts.tsBuildInfoFile.empty()
        ? (std::filesystem::path(resolveConfigRelativePath(
               opts.outDir.empty() ? "." : opts.outDir)) /
           ".tsbuildinfo").string()
        : resolveConfigRelativePath(opts.tsBuildInfoFile);

    std::ifstream file(infoPath);
    if (!file.is_open()) {
        buildCache_.isValid = false;
        return;
    }

    // Simple parsing - just look for file paths and timestamps
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    file.close();

    buildCache_.fileModTimes.clear();
    buildCache_.isValid = true;

    // Parse file timestamps (simplified)
    std::regex fileRegex("\"([^\"]+)\"\\s*:\\s*(\\d+)");
    std::sregex_iterator it(content.begin(), content.end(), fileRegex);
    std::sregex_iterator end;
    while (it != end) {
        std::string path = (*it)[1].str();
        // Skip non-path entries
        if (path != "version" && path != "files") {
            // We store as time_t but the actual conversion is complex
            // For now, just invalidate cache to force rebuild
        }
        ++it;
    }
}

void Transpiler::watch(const std::string& projectPath, std::function<void(const TranspileResult&)> callback) {
    const auto& opts = config_.compilerOptions;
    const auto& watchOpts = config_.watchOptions;

    // Determine polling interval from watchOptions
    int pollInterval = 1000; // Default 1 second
    if (watchOpts.fallbackPolling == "fixedInterval") {
        pollInterval = 500;
    } else if (watchOpts.fallbackPolling == "dynamicPriority") {
        pollInterval = 250;
    }

    std::map<std::string, std::filesystem::file_time_type> lastModTimes;
    std::string outDir = opts.outDir.empty() ? "./dist" : opts.outDir;

    // Initial scan
    std::cout << "[Watch] Starting watch mode..." << std::endl;
    std::cout << "[Watch] Watching: " << projectPath << std::endl;

    auto files = findSourceFiles(projectPath);
    for (const auto& file : files) {
        try {
            lastModTimes[file] = std::filesystem::last_write_time(file);
        } catch (...) {
            // File might not exist yet
        }
    }

    std::cout << "[Watch] Found " << files.size() << " files to watch" << std::endl;
    std::cout << "[Watch] Press Ctrl+C to stop" << std::endl;

    // Main watch loop
    while (true) {
        try {
            files = findSourceFiles(projectPath);

            for (const auto& file : files) {
                try {
                    auto modTime = std::filesystem::last_write_time(file);

                    auto it = lastModTimes.find(file);
                    if (it == lastModTimes.end()) {
                        // New file
                        lastModTimes[file] = modTime;
                        std::cout << "[Watch] New file: " << file << std::endl;

                        auto result = transpileFile(file);
                        if (result.success) {
                            // Write output
                            std::string jsPath = resolveOutputPath(file, ".js");
                            std::filesystem::create_directories(std::filesystem::path(jsPath).parent_path());
                            std::ofstream out(jsPath);
                            out << result.jsCode;
                            out.close();
                            std::cout << "[Watch] Compiled: " << file << " -> " << jsPath << std::endl;
                        }
                        callback(result);
                    } else if (modTime > it->second) {
                        // File changed
                        lastModTimes[file] = modTime;
                        std::cout << "[Watch] Changed: " << file << std::endl;

                        auto result = transpileFile(file);
                        if (result.success) {
                            // Write output
                            std::string jsPath = resolveOutputPath(file, ".js");
                            std::filesystem::create_directories(std::filesystem::path(jsPath).parent_path());
                            std::ofstream out(jsPath);
                            out << result.jsCode;
                            out.close();

                            // Write declaration file if enabled
                            if (!result.dtsCode.empty()) {
                                std::string dtsPath = resolveOutputPath(file, ".d.ts");
                                std::ofstream dtsOut(dtsPath);
                                dtsOut << result.dtsCode;
                                dtsOut.close();
                            }

                            // Write source map if enabled
                            if (!result.sourceMap.empty()) {
                                std::ofstream mapOut(jsPath + ".map");
                                mapOut << result.sourceMap;
                                mapOut.close();
                            }

                            std::cout << "[Watch] Compiled: " << file << " -> " << jsPath << std::endl;
                        } else {
                            std::cout << "[Watch] Error in: " << file << std::endl;
                            for (const auto& err : result.errors) {
                                std::cout << "  " << err << std::endl;
                            }
                        }
                        callback(result);
                    }
                } catch (const std::exception&) {
                    // Skip files that can't be accessed
                }
            }

            // Check for deleted files
            for (auto it = lastModTimes.begin(); it != lastModTimes.end(); ) {
                if (!std::filesystem::exists(it->first)) {
                    std::cout << "[Watch] Deleted: " << it->first << std::endl;
                    it = lastModTimes.erase(it);
                } else {
                    ++it;
                }
            }

        } catch (const std::exception& e) {
            std::cerr << "[Watch] Error: " << e.what() << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(pollInterval));
    }
}

// ============================================================================
// tsconfig.json Parser
// ============================================================================

TSConfig parseTSConfig(const std::string& jsonContent) {
    TSConfig config;
    const auto hasKey = [&jsonContent](const std::string& key) {
        return std::regex_search(
            jsonContent, std::regex("\"" + key + "\"\\s*:"));
    };
    const std::vector<std::string> compilerOptionNames = {
        "outDir", "outFile", "rootDir", "declarationDir", "module",
        "moduleResolution", "moduleDetection", "baseUrl", "target", "jsx",
        "jsxFactory", "jsxFragmentFactory", "jsxImportSource", "sourceRoot",
        "mapRoot", "newLine", "tsBuildInfoFile", "declaration",
        "declarationMap", "emitDeclarationOnly", "sourceMap",
        "inlineSourceMap", "inlineSources", "removeComments", "noEmit",
        "noEmitOnError", "preserveConstEnums", "importHelpers",
        "downlevelIteration", "allowJs", "checkJs", "resolveJsonModule",
        "esModuleInterop", "allowSyntheticDefaultImports", "strict",
        "composite", "incremental", "isolatedModules", "skipLibCheck",
        "isolatedDeclarations", "verbatimModuleSyntax",
        "allowArbitraryExtensions", "allowImportingTsExtensions",
        "resolvePackageJsonExports", "resolvePackageJsonImports",
        "noResolve", "rewriteRelativeImportExtensions", "minify", "paths",
        "lib", "types", "typeRoots", "rootDirs", "moduleSuffixes",
        "customConditions"
    };
    for (const std::string& option : compilerOptionNames) {
        if (hasKey(option)) config.specifiedCompilerOptions.insert(option);
    }
    for (const std::string& option :
         {"extends", "include", "exclude", "files", "references"}) {
        if (hasKey(option)) config.specifiedTopLevel.insert(option);
    }

    // Helper to get string value
    auto getValue = [&jsonContent](const std::string& key) -> std::string {
        std::regex regex("\"" + key + "\"\\s*:\\s*\"([^\"]+)\"");
        std::smatch match;
        if (std::regex_search(jsonContent, match, regex)) {
            return match[1].str();
        }
        return "";
    };

    // Helper to get boolean value
    auto getBoolValue = [&jsonContent](const std::string& key, bool defaultVal) -> bool {
        std::regex regex("\"" + key + "\"\\s*:\\s*(true|false)");
        std::smatch match;
        if (std::regex_search(jsonContent, match, regex)) {
            return match[1].str() == "true";
        }
        return defaultVal;
    };

    // Helper to get integer value
    auto getIntValue = [&jsonContent](const std::string& key, int defaultVal) -> int {
        std::regex regex("\"" + key + "\"\\s*:\\s*(\\d+)");
        std::smatch match;
        if (std::regex_search(jsonContent, match, regex)) {
            return std::stoi(match[1].str());
        }
        return defaultVal;
    };

    // Helper to get array values
    auto getArrayValue = [&jsonContent](const std::string& key) -> std::vector<std::string> {
        std::vector<std::string> result;
        std::regex arrayRegex("\"" + key + "\"\\s*:\\s*\\[([^\\]]+)\\]");
        std::smatch match;
        if (std::regex_search(jsonContent, match, arrayRegex)) {
            std::string items = match[1].str();
            std::regex itemRegex("\"([^\"]+)\"");
            std::sregex_iterator it(items.begin(), items.end(), itemRegex);
            std::sregex_iterator end;
            while (it != end) {
                result.push_back(it->str(1));
                ++it;
            }
        }
        return result;
    };

    // Parse extends
    config.extends = getValue("extends");

    // Parse compilerOptions
    std::string val;
    auto& opts = config.compilerOptions;

    // Output options
    if (!(val = getValue("outDir")).empty()) opts.outDir = val;
    if (!(val = getValue("outFile")).empty()) opts.outFile = val;
    if (!(val = getValue("rootDir")).empty()) opts.rootDir = val;
    if (!(val = getValue("declarationDir")).empty()) opts.declarationDir = val;

    // Module options
    if (!(val = getValue("module")).empty()) opts.module = val;
    if (!(val = getValue("moduleResolution")).empty()) opts.moduleResolution = val;
    if (!(val = getValue("moduleDetection")).empty()) opts.moduleDetection = val;
    if (!(val = getValue("baseUrl")).empty()) opts.baseUrl = val;
    if (!(val = getValue("target")).empty()) opts.target = val;

    // JSX options
    if (!(val = getValue("jsx")).empty()) opts.jsx = val;
    if (!(val = getValue("jsxFactory")).empty()) opts.jsxFactory = val;
    if (!(val = getValue("jsxFragmentFactory")).empty()) opts.jsxFragmentFactory = val;
    if (!(val = getValue("jsxImportSource")).empty()) opts.jsxImportSource = val;

    // Source map options
    if (!(val = getValue("sourceRoot")).empty()) opts.sourceRoot = val;
    if (!(val = getValue("mapRoot")).empty()) opts.mapRoot = val;
    if (!(val = getValue("newLine")).empty()) opts.newLine = val;

    // Build options
    if (!(val = getValue("tsBuildInfoFile")).empty()) opts.tsBuildInfoFile = val;

    // Boolean options
    opts.declaration = getBoolValue("declaration", false);
    opts.declarationMap = getBoolValue("declarationMap", false);
    opts.emitDeclarationOnly = getBoolValue("emitDeclarationOnly", false);
    opts.sourceMap = getBoolValue("sourceMap", false);
    opts.inlineSourceMap = getBoolValue("inlineSourceMap", false);
    opts.inlineSources = getBoolValue("inlineSources", false);
    opts.removeComments = getBoolValue("removeComments", false);
    opts.noEmit = getBoolValue("noEmit", false);
    opts.noEmitOnError = getBoolValue("noEmitOnError", false);
    opts.preserveConstEnums = getBoolValue("preserveConstEnums", false);
    opts.importHelpers = getBoolValue("importHelpers", false);
    opts.downlevelIteration = getBoolValue("downlevelIteration", false);
    opts.allowJs = getBoolValue("allowJs", false);
    opts.checkJs = getBoolValue("checkJs", false);
    opts.maxNodeModuleJsDepth = getIntValue("maxNodeModuleJsDepth", 0);
    opts.resolveJsonModule = getBoolValue("resolveJsonModule", false);
    opts.esModuleInterop = getBoolValue("esModuleInterop", true);
    opts.allowSyntheticDefaultImports = getBoolValue("allowSyntheticDefaultImports", true);

    // Type checking options (parsed for strict mode / alwaysStrict)
    opts.strict = getBoolValue("strict", false);
    opts.noImplicitAny = getBoolValue("noImplicitAny", false);
    opts.strictNullChecks = getBoolValue("strictNullChecks", false);
    opts.strictFunctionTypes = getBoolValue("strictFunctionTypes", false);
    opts.strictBindCallApply = getBoolValue("strictBindCallApply", false);
    opts.strictPropertyInitialization = getBoolValue("strictPropertyInitialization", false);
    opts.noImplicitThis = getBoolValue("noImplicitThis", false);
    opts.useUnknownInCatchVariables = getBoolValue("useUnknownInCatchVariables", false);
    opts.alwaysStrict = getBoolValue("alwaysStrict", false);
    opts.noUnusedLocals = getBoolValue("noUnusedLocals", false);
    opts.noUnusedParameters = getBoolValue("noUnusedParameters", false);
    opts.exactOptionalPropertyTypes = getBoolValue("exactOptionalPropertyTypes", false);
    opts.noImplicitReturns = getBoolValue("noImplicitReturns", false);
    opts.noFallthroughCasesInSwitch = getBoolValue("noFallthroughCasesInSwitch", false);
    opts.noUncheckedIndexedAccess = getBoolValue("noUncheckedIndexedAccess", false);
    opts.noImplicitOverride = getBoolValue("noImplicitOverride", false);
    opts.noPropertyAccessFromIndexSignature = getBoolValue("noPropertyAccessFromIndexSignature", false);
    opts.allowUnusedLabels = getBoolValue("allowUnusedLabels", false);
    opts.allowUnreachableCode = getBoolValue("allowUnreachableCode", false);

    // Interop constraints
    opts.isolatedModules = getBoolValue("isolatedModules", false);
    opts.isolatedDeclarations = getBoolValue("isolatedDeclarations", false);
    opts.verbatimModuleSyntax = getBoolValue("verbatimModuleSyntax", false);
    opts.allowArbitraryExtensions = getBoolValue("allowArbitraryExtensions", false);
    opts.allowImportingTsExtensions = getBoolValue("allowImportingTsExtensions", false);
    opts.resolvePackageJsonExports = getBoolValue("resolvePackageJsonExports", true);
    opts.resolvePackageJsonImports = getBoolValue("resolvePackageJsonImports", true);
    opts.noResolve = getBoolValue("noResolve", false);
    opts.allowUmdGlobalAccess = getBoolValue("allowUmdGlobalAccess", false);
    opts.rewriteRelativeImportExtensions = getBoolValue("rewriteRelativeImportExtensions", false);

    // Decorators
    opts.experimentalDecorators = getBoolValue("experimentalDecorators", false);
    opts.emitDecoratorMetadata = getBoolValue("emitDecoratorMetadata", false);
    opts.useDefineForClassFields = getBoolValue("useDefineForClassFields", true);

    // Build options
    opts.composite = getBoolValue("composite", false);
    opts.incremental = getBoolValue("incremental", false);
    opts.disableSolutionSearching = getBoolValue("disableSolutionSearching", false);
    opts.disableReferencedProjectLoad = getBoolValue("disableReferencedProjectLoad", false);
    opts.disableSourceOfProjectReferenceRedirect = getBoolValue("disableSourceOfProjectReferenceRedirect", false);
    opts.disableSizeLimit = getBoolValue("disableSizeLimit", false);

    // Watch options
    opts.assumeChangesOnlyAffectDirectDependencies = getBoolValue("assumeChangesOnlyAffectDirectDependencies", false);
    opts.preserveWatchOutput = getBoolValue("preserveWatchOutput", false);

    // Completeness
    opts.skipLibCheck = getBoolValue("skipLibCheck", true);
    opts.skipDefaultLibCheck = getBoolValue("skipDefaultLibCheck", false);
    opts.forceConsistentCasingInFileNames = getBoolValue("forceConsistentCasingInFileNames", true);

    // Advanced / Diagnostic options
    opts.noLib = getBoolValue("noLib", false);
    opts.preserveSymlinks = getBoolValue("preserveSymlinks", false);
    opts.noErrorTruncation = getBoolValue("noErrorTruncation", false);
    opts.listFiles = getBoolValue("listFiles", false);
    opts.listEmittedFiles = getBoolValue("listEmittedFiles", false);
    opts.traceResolution = getBoolValue("traceResolution", false);
    opts.extendedDiagnostics = getBoolValue("extendedDiagnostics", false);
    opts.explainFiles = getBoolValue("explainFiles", false);
    opts.pretty = getBoolValue("pretty", true);
    opts.generateCpuProfile = getBoolValue("generateCpuProfile", false);
    if (!(val = getValue("generateTrace")).empty()) opts.generateTrace = val;

    // Deprecated options (parsed for compatibility)
    opts.keyofStringsOnly = getBoolValue("keyofStringsOnly", false);
    opts.suppressExcessPropertyErrors = getBoolValue("suppressExcessPropertyErrors", false);
    opts.suppressImplicitAnyIndexErrors = getBoolValue("suppressImplicitAnyIndexErrors", false);
    opts.noStrictGenericChecks = getBoolValue("noStrictGenericChecks", false);
    if (!(val = getValue("charset")).empty()) opts.charset = val;
    opts.importsNotUsedAsValues = getBoolValue("importsNotUsedAsValues", false);
    opts.preserveValueImports = getBoolValue("preserveValueImports", false);

    // Emit options
    opts.emitBOM = getBoolValue("emitBOM", false);
    opts.stripInternal = getBoolValue("stripInternal", false);
    opts.noEmitHelpers = getBoolValue("noEmitHelpers", false);
    opts.importHelpers = getBoolValue("importHelpers", false);

    // Nova-specific
    opts.minify = getBoolValue("minify", false);

    // If strict is enabled, enable all strict sub-options
    if (opts.strict) {
        opts.noImplicitAny = true;
        opts.strictNullChecks = true;
        opts.strictFunctionTypes = true;
        opts.strictBindCallApply = true;
        opts.strictPropertyInitialization = true;
        opts.noImplicitThis = true;
        opts.useUnknownInCatchVariables = true;
        opts.alwaysStrict = true;
    }

    // Parse arrays
    config.include = getArrayValue("include");
    config.exclude = getArrayValue("exclude");
    config.files = getArrayValue("files");
    opts.lib = getArrayValue("lib");
    opts.types = getArrayValue("types");
    opts.typeRoots = getArrayValue("typeRoots");
    opts.rootDirs = getArrayValue("rootDirs");
    opts.moduleSuffixes = getArrayValue("moduleSuffixes");
    opts.customConditions = getArrayValue("customConditions");

    // Parse plugins array (more complex structure with objects)
    std::regex pluginsRegex("\"plugins\"\\s*:\\s*\\[([^\\]]+)\\]");
    std::smatch pluginsMatch;
    if (std::regex_search(jsonContent, pluginsMatch, pluginsRegex)) {
        std::string pluginsContent = pluginsMatch[1].str();
        std::regex pluginRegex("\\{([^}]*)\\}");
        std::sregex_iterator it(pluginsContent.begin(), pluginsContent.end(), pluginRegex);
        std::sregex_iterator end;
        while (it != end) {
            std::string pluginObj = (*it)[1].str();
            CompilerOptions::Plugin plugin;

            // Extract name
            std::regex nameRegex("\"name\"\\s*:\\s*\"([^\"]+)\"");
            std::smatch nameMatch;
            if (std::regex_search(pluginObj, nameMatch, nameRegex)) {
                plugin.name = nameMatch[1].str();
            }

            // Extract other string options
            std::regex optRegex("\"(\\w+)\"\\s*:\\s*\"([^\"]+)\"");
            std::sregex_iterator optIt(pluginObj.begin(), pluginObj.end(), optRegex);
            while (optIt != end) {
                std::string key = (*optIt)[1].str();
                std::string value = (*optIt)[2].str();
                if (key != "name") {
                    plugin.options[key] = value;
                }
                ++optIt;
            }

            if (!plugin.name.empty()) {
                opts.plugins.push_back(plugin);
            }
            ++it;
        }
    }

    // Parse paths (more complex structure)
    std::regex pathsRegex("\"paths\"\\s*:\\s*\\{([^}]+)\\}");
    std::smatch pathsMatch;
    if (std::regex_search(jsonContent, pathsMatch, pathsRegex)) {
        std::string pathsContent = pathsMatch[1].str();
        std::regex pathRegex("\"([^\"]+)\"\\s*:\\s*\\[([^\\]]+)\\]");
        std::sregex_iterator it(pathsContent.begin(), pathsContent.end(), pathRegex);
        std::sregex_iterator end;
        while (it != end) {
            std::string key = (*it)[1].str();
            std::string valuesStr = (*it)[2].str();

            std::vector<std::string> values;
            std::regex valueRegex("\"([^\"]+)\"");
            std::sregex_iterator valIt(valuesStr.begin(), valuesStr.end(), valueRegex);
            while (valIt != end) {
                values.push_back((*valIt)[1].str());
                ++valIt;
            }

            opts.paths[key] = values;
            ++it;
        }
    }

    // Parse project references
    std::regex referencesRegex("\"references\"\\s*:\\s*\\[([^\\]]+)\\]");
    std::smatch refsMatch;
    if (std::regex_search(jsonContent, refsMatch, referencesRegex)) {
        std::string refsContent = refsMatch[1].str();
        std::regex refRegex("\\{[^}]*\"path\"\\s*:\\s*\"([^\"]+)\"[^}]*\\}");
        std::sregex_iterator it(refsContent.begin(), refsContent.end(), refRegex);
        std::sregex_iterator end;
        while (it != end) {
            TSConfig::Reference ref;
            ref.path = (*it)[1].str();

            // Check for prepend option
            std::string refStr = (*it)[0].str();
            if (refStr.find("\"prepend\"") != std::string::npos &&
                refStr.find("true") != std::string::npos) {
                ref.prepend = true;
            }

            config.references.push_back(ref);
            ++it;
        }
    }

    // Parse watchOptions
    std::regex watchRegex("\"watchOptions\"\\s*:\\s*\\{([^}]+)\\}");
    std::smatch watchMatch;
    if (std::regex_search(jsonContent, watchMatch, watchRegex)) {
        std::string watchContent = watchMatch[1].str();
        auto& wo = config.watchOptions;

        std::regex strOptRegex("\"(\\w+)\"\\s*:\\s*\"([^\"]+)\"");
        std::sregex_iterator it(watchContent.begin(), watchContent.end(), strOptRegex);
        std::sregex_iterator end;
        while (it != end) {
            std::string key = (*it)[1].str();
            std::string value = (*it)[2].str();
            if (key == "watchFile") wo.watchFile = value;
            else if (key == "watchDirectory") wo.watchDirectory = value;
            else if (key == "fallbackPolling") wo.fallbackPolling = value;
            ++it;
        }

        // Parse boolean options
        if (watchContent.find("\"synchronousWatchDirectory\"") != std::string::npos &&
            watchContent.find("true") != std::string::npos) {
            wo.synchronousWatchDirectory = true;
        }
    }

    return config;
}

std::string serializeTSConfig([[maybe_unused]] const TSConfig& config) {
    // TODO: Implement serialization if needed
    return "{}";
}

} // namespace transpiler
} // namespace nova
