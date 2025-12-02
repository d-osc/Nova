// Nova REPL Module - Node.js compatible REPL API
// Provides Read-Eval-Print-Loop functionality

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <fstream>
#include <cstdio>
#include <cstring>
#include <deque>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#else
#include <unistd.h>
#endif

// REPL modes
static const int REPL_MODE_SLOPPY = 0;
static const int REPL_MODE_STRICT = 1;

// Default options
static const char* DEFAULT_PROMPT = "> ";
static const char* DEFAULT_INPUT_PROMPT = "... ";
static const int DEFAULT_HISTORY_SIZE = 1000;

// Command definition
struct ReplCommand {
    std::string help;
    std::function<void(const char*)> action;
};

// REPL Server state
struct REPLServer {
    FILE* input;
    FILE* output;
    std::string prompt;
    std::string inputPrompt;  // For multi-line input
    bool terminal;
    bool useColors;
    bool useGlobal;
    bool ignoreUndefined;
    int replMode;
    bool preview;
    bool breakEvalOnSigint;

    // Current state
    std::string currentLine;
    std::string bufferedCommand;
    bool running;
    bool paused;

    // History
    std::deque<std::string> history;
    int historySize;
    std::string historyPath;
    int historyIndex;

    // Context (simple key-value store for variables)
    std::map<std::string, std::string> context;

    // Custom commands
    std::map<std::string, ReplCommand> commands;

    // Event callbacks
    std::function<void()> onExit;
    std::function<void()> onReset;
    std::function<void(const char*)> onLine;
    std::function<const char*(const char*)> eval;
    std::function<const char*(const char*)> writer;
    std::function<std::vector<std::string>(const char*, int)> completer;

    REPLServer() :
        input(stdin), output(stdout),
        prompt(DEFAULT_PROMPT), inputPrompt(DEFAULT_INPUT_PROMPT),
        terminal(true), useColors(true), useGlobal(true),
        ignoreUndefined(true), replMode(REPL_MODE_SLOPPY),
        preview(true), breakEvalOnSigint(true),
        running(false), paused(false),
        historySize(DEFAULT_HISTORY_SIZE), historyIndex(-1) {

        // Register default commands
        registerDefaultCommands();
    }

    void registerDefaultCommands() {
        commands[".help"] = {
            "Print this help message",
            [this](const char*) { printHelp(); }
        };
        commands[".exit"] = {
            "Exit the REPL",
            [this](const char*) { running = false; }
        };
        commands[".clear"] = {
            "Break, and also clear the local context",
            [this](const char*) { clearContext(); }
        };
        commands[".break"] = {
            "Clear the current multi-line expression",
            [this](const char*) { clearBufferedCommand(); }
        };
        commands[".editor"] = {
            "Enter editor mode",
            [this](const char*) { enterEditorMode(); }
        };
        commands[".load"] = {
            "Load JS from a file into the REPL session",
            [this](const char* arg) { loadFile(arg); }
        };
        commands[".save"] = {
            "Save all evaluated commands to a file",
            [this](const char* arg) { saveSession(arg); }
        };
    }

    void printHelp() {
        fprintf(output, "\n");
        for (const auto& cmd : commands) {
            fprintf(output, "%-12s %s\n", cmd.first.c_str(), cmd.second.help.c_str());
        }
        fprintf(output, "\nPress Ctrl+C to abort current expression, Ctrl+D to exit the REPL\n\n");
    }

    void clearContext() {
        context.clear();
        bufferedCommand = "";
        if (onReset) onReset();
        fprintf(output, "Clearing context...\n");
    }

    void clearBufferedCommand() {
        bufferedCommand = "";
    }

    void enterEditorMode() {
        fprintf(output, "// Entering editor mode (Ctrl+D to finish, Ctrl+C to cancel)\n");
        std::string code;
        char buffer[4096];

        while (fgets(buffer, sizeof(buffer), input)) {
            code += buffer;
        }

        if (!code.empty()) {
            bufferedCommand = code;
            // Would evaluate the code here
        }
    }

    void loadFile(const char* filename) {
        if (!filename || strlen(filename) == 0) {
            fprintf(output, "Error: .load requires a filename\n");
            return;
        }

        std::ifstream file(filename);
        if (!file) {
            fprintf(output, "Error: Cannot open file '%s'\n", filename);
            return;
        }

        std::string line;
        while (std::getline(file, line)) {
            fprintf(output, "%s\n", line.c_str());
            // Would evaluate each line here
        }
    }

    void saveSession(const char* filename) {
        if (!filename || strlen(filename) == 0) {
            fprintf(output, "Error: .save requires a filename\n");
            return;
        }

        std::ofstream file(filename);
        if (!file) {
            fprintf(output, "Error: Cannot open file '%s' for writing\n", filename);
            return;
        }

        for (const auto& cmd : history) {
            file << cmd << "\n";
        }

        fprintf(output, "Session saved to '%s'\n", filename);
    }
};

// Active REPL servers
static std::vector<REPLServer*> replServers;

// Built-in modules list
static std::vector<std::string> builtinModules = {
    "assert", "async_hooks", "buffer", "child_process", "cluster",
    "console", "constants", "crypto", "dgram", "diagnostics_channel",
    "dns", "domain", "events", "fs", "http", "http2", "https",
    "inspector", "module", "net", "os", "path", "perf_hooks",
    "process", "punycode", "querystring", "readline", "repl",
    "stream", "string_decoder", "sys", "timers", "tls", "trace_events",
    "tty", "url", "util", "v8", "vm", "wasi", "worker_threads", "zlib"
};

extern "C" {

// ============================================================================
// Module Functions
// ============================================================================

// repl.start(options) - Start a REPL
void* nova_repl_start(const char* prompt, FILE* input, FILE* output,
                       bool terminal, bool useColors, bool useGlobal,
                       bool ignoreUndefined, int replMode, bool preview) {
    auto* server = new REPLServer();

    if (prompt) server->prompt = prompt;
    if (input) server->input = input;
    if (output) server->output = output;
    server->terminal = terminal;
    server->useColors = useColors;
    server->useGlobal = useGlobal;
    server->ignoreUndefined = ignoreUndefined;
    server->replMode = replMode;
    server->preview = preview;
    server->running = true;

    replServers.push_back(server);
    return server;
}

// Start with default options
void* nova_repl_startDefault() {
    return nova_repl_start(nullptr, nullptr, nullptr, true, true, true, true, REPL_MODE_SLOPPY, true);
}

// ============================================================================
// REPLServer Methods
// ============================================================================

// replServer.defineCommand(keyword, cmd)
void nova_repl_defineCommand(void* server, const char* keyword,
                              const char* help, void (*action)(const char*)) {
    auto* repl = static_cast<REPLServer*>(server);

    std::string key = ".";
    key += keyword;

    repl->commands[key] = {
        help ? help : "",
        action
    };
}

// replServer.displayPrompt(preserveCursor)
void nova_repl_displayPrompt(void* server, bool preserveCursor) {
    auto* repl = static_cast<REPLServer*>(server);
    (void)preserveCursor;

    const char* p = repl->bufferedCommand.empty() ?
                    repl->prompt.c_str() : repl->inputPrompt.c_str();
    fprintf(repl->output, "%s", p);
    fflush(repl->output);
}

// replServer.clearBufferedCommand()
void nova_repl_clearBufferedCommand(void* server) {
    auto* repl = static_cast<REPLServer*>(server);
    repl->clearBufferedCommand();
}

// replServer.setupHistory(historyPath, callback)
bool nova_repl_setupHistory(void* server, const char* historyPath) {
    auto* repl = static_cast<REPLServer*>(server);

    if (!historyPath) return false;

    repl->historyPath = historyPath;

    // Load existing history
    std::ifstream file(historyPath);
    if (file) {
        std::string line;
        while (std::getline(file, line)) {
            if (!line.empty()) {
                repl->history.push_back(line);
            }
        }

        // Trim to history size
        while (static_cast<int>(repl->history.size()) > repl->historySize) {
            repl->history.pop_front();
        }
    }

    return true;
}

// Save history to file
void nova_repl_saveHistory(void* server) {
    auto* repl = static_cast<REPLServer*>(server);

    if (repl->historyPath.empty()) return;

    std::ofstream file(repl->historyPath);
    if (file) {
        for (const auto& line : repl->history) {
            file << line << "\n";
        }
    }
}

// replServer.close()
void nova_repl_close(void* server) {
    auto* repl = static_cast<REPLServer*>(server);
    repl->running = false;

    // Save history
    nova_repl_saveHistory(server);

    if (repl->onExit) {
        repl->onExit();
    }
}

// replServer.pause()
void nova_repl_pause(void* server) {
    auto* repl = static_cast<REPLServer*>(server);
    repl->paused = true;
}

// replServer.resume()
void nova_repl_resume(void* server) {
    auto* repl = static_cast<REPLServer*>(server);
    repl->paused = false;
}

// ============================================================================
// Context Management
// ============================================================================

// Set context variable
void nova_repl_setContext(void* server, const char* key, const char* value) {
    auto* repl = static_cast<REPLServer*>(server);
    if (key) {
        repl->context[key] = value ? value : "";
    }
}

// Get context variable
const char* nova_repl_getContext(void* server, const char* key) {
    static std::string result;
    auto* repl = static_cast<REPLServer*>(server);

    auto it = repl->context.find(key);
    if (it != repl->context.end()) {
        result = it->second;
        return result.c_str();
    }
    return nullptr;
}

// Get all context keys
const char** nova_repl_getContextKeys(void* server, int* count) {
    static std::vector<const char*> keys;
    static std::vector<std::string> keyStorage;

    auto* repl = static_cast<REPLServer*>(server);
    keys.clear();
    keyStorage.clear();

    for (const auto& pair : repl->context) {
        keyStorage.push_back(pair.first);
    }

    for (const auto& key : keyStorage) {
        keys.push_back(key.c_str());
    }

    *count = static_cast<int>(keys.size());
    return keys.data();
}

// Reset context
void nova_repl_resetContext(void* server) {
    auto* repl = static_cast<REPLServer*>(server);
    repl->clearContext();
}

// ============================================================================
// Event Handlers
// ============================================================================

// Set 'exit' event handler
void nova_repl_onExit(void* server, void (*callback)()) {
    auto* repl = static_cast<REPLServer*>(server);
    repl->onExit = callback;
}

// Set 'reset' event handler
void nova_repl_onReset(void* server, void (*callback)()) {
    auto* repl = static_cast<REPLServer*>(server);
    repl->onReset = callback;
}

// Set line callback
void nova_repl_onLine(void* server, void (*callback)(const char*)) {
    auto* repl = static_cast<REPLServer*>(server);
    repl->onLine = callback;
}

// Set custom eval function
void nova_repl_setEval(void* server, const char* (*evalFunc)(const char*)) {
    auto* repl = static_cast<REPLServer*>(server);
    repl->eval = evalFunc;
}

// Set custom writer function
void nova_repl_setWriter(void* server, const char* (*writerFunc)(const char*)) {
    auto* repl = static_cast<REPLServer*>(server);
    repl->writer = writerFunc;
}

// ============================================================================
// Input Processing
// ============================================================================

// Process a line of input
const char* nova_repl_processLine(void* server, const char* line) {
    static std::string result;
    auto* repl = static_cast<REPLServer*>(server);

    if (!line) return nullptr;

    std::string input(line);

    // Trim whitespace
    size_t start = input.find_first_not_of(" \t\r\n");
    size_t end = input.find_last_not_of(" \t\r\n");
    if (start != std::string::npos) {
        input = input.substr(start, end - start + 1);
    } else {
        input = "";
    }

    if (input.empty()) {
        return "";
    }

    // Check for commands
    if (input[0] == '.') {
        // Find command and argument
        size_t spacePos = input.find(' ');
        std::string cmd = (spacePos != std::string::npos) ?
                          input.substr(0, spacePos) : input;
        std::string arg = (spacePos != std::string::npos) ?
                          input.substr(spacePos + 1) : "";

        auto it = repl->commands.find(cmd);
        if (it != repl->commands.end()) {
            it->second.action(arg.c_str());
            return "";
        } else {
            result = "Invalid REPL keyword";
            return result.c_str();
        }
    }

    // Add to history
    if (!input.empty()) {
        repl->history.push_back(input);
        while (static_cast<int>(repl->history.size()) > repl->historySize) {
            repl->history.pop_front();
        }
        repl->historyIndex = -1;
    }

    // Evaluate
    if (repl->eval) {
        const char* evalResult = repl->eval(input.c_str());

        // Format output
        if (evalResult) {
            if (repl->writer) {
                result = repl->writer(evalResult);
            } else {
                result = evalResult;
            }
            return result.c_str();
        } else if (!repl->ignoreUndefined) {
            result = "undefined";
            return result.c_str();
        }
    }

    // Fire line event
    if (repl->onLine) {
        repl->onLine(input.c_str());
    }

    return "";
}

// Read and process one line (blocking)
const char* nova_repl_readLine(void* server) {
    auto* repl = static_cast<REPLServer*>(server);

    nova_repl_displayPrompt(server, false);

    char buffer[4096];
    if (fgets(buffer, sizeof(buffer), repl->input)) {
        return nova_repl_processLine(server, buffer);
    }

    return nullptr;
}

// Run REPL loop
void nova_repl_run(void* server) {
    auto* repl = static_cast<REPLServer*>(server);

    while (repl->running) {
        if (repl->paused) continue;

        const char* result = nova_repl_readLine(server);

        if (result == nullptr) {
            // EOF
            break;
        }

        if (strlen(result) > 0) {
            fprintf(repl->output, "%s\n", result);
        }
    }

    nova_repl_close(server);
}

// ============================================================================
// Properties
// ============================================================================

// Get/Set prompt
const char* nova_repl_getPrompt(void* server) {
    auto* repl = static_cast<REPLServer*>(server);
    return repl->prompt.c_str();
}

void nova_repl_setPrompt(void* server, const char* prompt) {
    auto* repl = static_cast<REPLServer*>(server);
    repl->prompt = prompt ? prompt : DEFAULT_PROMPT;
}

// Get/Set input prompt (for multi-line)
const char* nova_repl_getInputPrompt(void* server) {
    auto* repl = static_cast<REPLServer*>(server);
    return repl->inputPrompt.c_str();
}

void nova_repl_setInputPrompt(void* server, const char* prompt) {
    auto* repl = static_cast<REPLServer*>(server);
    repl->inputPrompt = prompt ? prompt : DEFAULT_INPUT_PROMPT;
}

// Get buffered command
const char* nova_repl_getBufferedCommand(void* server) {
    auto* repl = static_cast<REPLServer*>(server);
    return repl->bufferedCommand.c_str();
}

// Get current line
const char* nova_repl_getCurrentLine(void* server) {
    auto* repl = static_cast<REPLServer*>(server);
    return repl->currentLine.c_str();
}

// Check if running
bool nova_repl_isRunning(void* server) {
    auto* repl = static_cast<REPLServer*>(server);
    return repl->running;
}

// Check if terminal
bool nova_repl_isTerminal(void* server) {
    auto* repl = static_cast<REPLServer*>(server);
    return repl->terminal;
}

// Get/Set useColors
bool nova_repl_getUseColors(void* server) {
    auto* repl = static_cast<REPLServer*>(server);
    return repl->useColors;
}

void nova_repl_setUseColors(void* server, bool value) {
    auto* repl = static_cast<REPLServer*>(server);
    repl->useColors = value;
}

// Get/Set useGlobal
bool nova_repl_getUseGlobal(void* server) {
    auto* repl = static_cast<REPLServer*>(server);
    return repl->useGlobal;
}

void nova_repl_setUseGlobal(void* server, bool value) {
    auto* repl = static_cast<REPLServer*>(server);
    repl->useGlobal = value;
}

// Get/Set ignoreUndefined
bool nova_repl_getIgnoreUndefined(void* server) {
    auto* repl = static_cast<REPLServer*>(server);
    return repl->ignoreUndefined;
}

void nova_repl_setIgnoreUndefined(void* server, bool value) {
    auto* repl = static_cast<REPLServer*>(server);
    repl->ignoreUndefined = value;
}

// Get/Set replMode
int nova_repl_getReplMode(void* server) {
    auto* repl = static_cast<REPLServer*>(server);
    return repl->replMode;
}

void nova_repl_setReplMode(void* server, int mode) {
    auto* repl = static_cast<REPLServer*>(server);
    repl->replMode = mode;
}

// Get/Set preview
bool nova_repl_getPreview(void* server) {
    auto* repl = static_cast<REPLServer*>(server);
    return repl->preview;
}

void nova_repl_setPreview(void* server, bool value) {
    auto* repl = static_cast<REPLServer*>(server);
    repl->preview = value;
}

// Get/Set breakEvalOnSigint
bool nova_repl_getBreakEvalOnSigint(void* server) {
    auto* repl = static_cast<REPLServer*>(server);
    return repl->breakEvalOnSigint;
}

void nova_repl_setBreakEvalOnSigint(void* server, bool value) {
    auto* repl = static_cast<REPLServer*>(server);
    repl->breakEvalOnSigint = value;
}

// ============================================================================
// History Management
// ============================================================================

// Get history
const char** nova_repl_getHistory(void* server, int* count) {
    static std::vector<const char*> histPtrs;
    auto* repl = static_cast<REPLServer*>(server);

    histPtrs.clear();
    for (const auto& h : repl->history) {
        histPtrs.push_back(h.c_str());
    }

    *count = static_cast<int>(histPtrs.size());
    return histPtrs.data();
}

// Clear history
void nova_repl_clearHistory(void* server) {
    auto* repl = static_cast<REPLServer*>(server);
    repl->history.clear();
    repl->historyIndex = -1;
}

// Get/Set history size
int nova_repl_getHistorySize(void* server) {
    auto* repl = static_cast<REPLServer*>(server);
    return repl->historySize;
}

void nova_repl_setHistorySize(void* server, int size) {
    auto* repl = static_cast<REPLServer*>(server);
    repl->historySize = size > 0 ? size : DEFAULT_HISTORY_SIZE;

    while (static_cast<int>(repl->history.size()) > repl->historySize) {
        repl->history.pop_front();
    }
}

// Navigate history
const char* nova_repl_historyUp(void* server) {
    auto* repl = static_cast<REPLServer*>(server);

    if (repl->historyIndex < static_cast<int>(repl->history.size()) - 1) {
        repl->historyIndex++;
        size_t idx = repl->history.size() - 1 - repl->historyIndex;
        return repl->history[idx].c_str();
    }
    return nullptr;
}

const char* nova_repl_historyDown(void* server) {
    auto* repl = static_cast<REPLServer*>(server);

    if (repl->historyIndex > 0) {
        repl->historyIndex--;
        size_t idx = repl->history.size() - 1 - repl->historyIndex;
        return repl->history[idx].c_str();
    } else if (repl->historyIndex == 0) {
        repl->historyIndex = -1;
        return "";
    }
    return nullptr;
}

// ============================================================================
// Built-in Commands Management
// ============================================================================

// Get available commands
const char** nova_repl_getCommands(void* server, int* count) {
    static std::vector<const char*> cmdPtrs;
    static std::vector<std::string> cmdStorage;

    auto* repl = static_cast<REPLServer*>(server);
    cmdPtrs.clear();
    cmdStorage.clear();

    for (const auto& cmd : repl->commands) {
        cmdStorage.push_back(cmd.first);
    }

    for (const auto& cmd : cmdStorage) {
        cmdPtrs.push_back(cmd.c_str());
    }

    *count = static_cast<int>(cmdPtrs.size());
    return cmdPtrs.data();
}

// Get command help
const char* nova_repl_getCommandHelp(void* server, const char* keyword) {
    static std::string result;
    auto* repl = static_cast<REPLServer*>(server);

    std::string key = keyword;
    if (key[0] != '.') key = "." + key;

    auto it = repl->commands.find(key);
    if (it != repl->commands.end()) {
        result = it->second.help;
        return result.c_str();
    }
    return nullptr;
}

// Remove command
void nova_repl_removeCommand(void* server, const char* keyword) {
    auto* repl = static_cast<REPLServer*>(server);

    std::string key = keyword;
    if (key[0] != '.') key = "." + key;

    repl->commands.erase(key);
}

// ============================================================================
// Module Constants
// ============================================================================

// repl.REPL_MODE_SLOPPY
int nova_repl_REPL_MODE_SLOPPY() {
    return REPL_MODE_SLOPPY;
}

// repl.REPL_MODE_STRICT
int nova_repl_REPL_MODE_STRICT() {
    return REPL_MODE_STRICT;
}

// repl.builtinModules
const char** nova_repl_builtinModules(int* count) {
    static std::vector<const char*> modPtrs;
    modPtrs.clear();

    for (const auto& mod : builtinModules) {
        modPtrs.push_back(mod.c_str());
    }

    *count = static_cast<int>(modPtrs.size());
    return modPtrs.data();
}

// ============================================================================
// Completer Support
// ============================================================================

// Set completer function
void nova_repl_setCompleter(void* server,
                             void (*completer)(const char*, const char***, int*)) {
    auto* repl = static_cast<REPLServer*>(server);

    if (completer) {
        repl->completer = [completer](const char* line, int cursor) -> std::vector<std::string> {
            (void)cursor;
            const char** completions;
            int count;
            completer(line, &completions, &count);

            std::vector<std::string> result;
            for (int i = 0; i < count; i++) {
                result.push_back(completions[i]);
            }
            return result;
        };
    } else {
        repl->completer = nullptr;
    }
}

// Get completions for input
const char** nova_repl_getCompletions(void* server, const char* line, int* count) {
    static std::vector<const char*> compPtrs;
    static std::vector<std::string> compStorage;

    auto* repl = static_cast<REPLServer*>(server);
    compPtrs.clear();
    compStorage.clear();

    if (repl->completer) {
        compStorage = repl->completer(line, 0);
        for (const auto& c : compStorage) {
            compPtrs.push_back(c.c_str());
        }
    }

    *count = static_cast<int>(compPtrs.size());
    return compPtrs.data();
}

// ============================================================================
// Cleanup
// ============================================================================

// Free REPL server
void nova_repl_free(void* server) {
    auto* repl = static_cast<REPLServer*>(server);

    auto it = std::find(replServers.begin(), replServers.end(), repl);
    if (it != replServers.end()) {
        replServers.erase(it);
    }

    delete repl;
}

// Cleanup all
void nova_repl_cleanup() {
    for (auto* server : replServers) {
        delete server;
    }
    replServers.clear();
}

} // extern "C"
