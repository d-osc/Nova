// Nova Readline Module - Node.js compatible readline API
// Provides line-by-line reading of streams

#include <algorithm>
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <cstdio>
#include <cstring>
#include <deque>

#ifdef _WIN32
#include <windows.h>
#include <conio.h>
#include <io.h>
#else
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#endif

// Direction constants for clearLine
static const int CLEAR_LEFT = -1;
static const int CLEAR_RIGHT = 1;
static const int CLEAR_BOTH = 0;

// Interface state
struct ReadlineInterface {
    FILE* input;
    FILE* output;
    std::string prompt;
    std::string line;
    int cursor;
    bool terminal;
    bool paused;
    bool closed;
    int historySize;
    std::deque<std::string> history;
    int historyIndex;
    bool removeHistoryDuplicates;
    std::string completer;  // Completer function name
    bool crlfDelay;
    bool escapeCodeTimeout;

    // Event callbacks
    std::function<void()> onClose;
    std::function<void(const char*)> onLine;
    std::function<void()> onPause;
    std::function<void()> onResume;
    std::function<void(const std::vector<std::string>&)> onHistory;
    std::function<void()> onSIGINT;
    std::function<void()> onSIGTSTP;
    std::function<void()> onSIGCONT;

    ReadlineInterface() :
        input(stdin), output(stdout), prompt("> "), line(""),
        cursor(0), terminal(true), paused(false), closed(false),
        historySize(30), historyIndex(-1), removeHistoryDuplicates(false),
        crlfDelay(true), escapeCodeTimeout(true) {}
};

// Active interfaces
static std::vector<ReadlineInterface*> interfaces;

// Question callback storage
static std::map<ReadlineInterface*, std::function<void(const char*)>> questionCallbacks;

extern "C" {

// ============================================================================
// Module-level Functions
// ============================================================================

// readline.createInterface(options)
void* nova_readline_createInterface(FILE* input, FILE* output, const char* prompt,
                                     bool terminal, int historySize) {
    auto* rl = new ReadlineInterface();
    rl->input = input ? input : stdin;
    rl->output = output ? output : stdout;
    rl->prompt = prompt ? prompt : "> ";
    rl->terminal = terminal;
    rl->historySize = historySize > 0 ? historySize : 30;

    interfaces.push_back(rl);
    return rl;
}

// readline.cursorTo(stream, x, y, callback)
void nova_readline_cursorTo(FILE* stream, int x, int y) {
    if (!stream) stream = stdout;

#ifdef _WIN32
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD pos;
    pos.X = static_cast<SHORT>(x);
    pos.Y = static_cast<SHORT>(y >= 0 ? y : 0);

    if (y < 0) {
        // Only set X, keep current Y
        CONSOLE_SCREEN_BUFFER_INFO info;
        GetConsoleScreenBufferInfo(hConsole, &info);
        pos.Y = info.dwCursorPosition.Y;
    }

    SetConsoleCursorPosition(hConsole, pos);
#else
    if (y >= 0) {
        fprintf(stream, "\033[%d;%dH", y + 1, x + 1);
    } else {
        fprintf(stream, "\033[%dG", x + 1);
    }
    fflush(stream);
#endif
}

// readline.moveCursor(stream, dx, dy, callback)
void nova_readline_moveCursor(FILE* stream, int dx, int dy) {
    if (!stream) stream = stdout;

#ifdef _WIN32
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO info;
    GetConsoleScreenBufferInfo(hConsole, &info);

    COORD pos;
    pos.X = info.dwCursorPosition.X + static_cast<SHORT>(dx);
    pos.Y = info.dwCursorPosition.Y + static_cast<SHORT>(dy);

    if (pos.X < 0) pos.X = 0;
    if (pos.Y < 0) pos.Y = 0;

    SetConsoleCursorPosition(hConsole, pos);
#else
    if (dx > 0) fprintf(stream, "\033[%dC", dx);
    else if (dx < 0) fprintf(stream, "\033[%dD", -dx);

    if (dy > 0) fprintf(stream, "\033[%dB", dy);
    else if (dy < 0) fprintf(stream, "\033[%dA", -dy);

    fflush(stream);
#endif
}

// readline.clearLine(stream, dir, callback)
void nova_readline_clearLine(FILE* stream, int dir) {
    if (!stream) stream = stdout;

#ifdef _WIN32
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO info;
    GetConsoleScreenBufferInfo(hConsole, &info);

    DWORD written;
    COORD startPos = info.dwCursorPosition;

    if (dir == CLEAR_LEFT || dir == CLEAR_BOTH) {
        // Clear from start to cursor
        COORD clearStart = {0, info.dwCursorPosition.Y};
        FillConsoleOutputCharacterA(hConsole, ' ', info.dwCursorPosition.X, clearStart, &written);
    }

    if (dir == CLEAR_RIGHT || dir == CLEAR_BOTH) {
        // Clear from cursor to end
        DWORD length = info.dwSize.X - info.dwCursorPosition.X;
        FillConsoleOutputCharacterA(hConsole, ' ', length, info.dwCursorPosition, &written);
    }

    SetConsoleCursorPosition(hConsole, startPos);
#else
    switch (dir) {
        case CLEAR_LEFT:  fprintf(stream, "\033[1K"); break;
        case CLEAR_RIGHT: fprintf(stream, "\033[0K"); break;
        case CLEAR_BOTH:  fprintf(stream, "\033[2K"); break;
    }
    fflush(stream);
#endif
}

// readline.clearScreenDown(stream, callback)
void nova_readline_clearScreenDown(FILE* stream) {
    if (!stream) stream = stdout;

#ifdef _WIN32
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO info;
    GetConsoleScreenBufferInfo(hConsole, &info);

    DWORD written;
    COORD startPos = info.dwCursorPosition;

    // Clear from cursor to end of screen
    DWORD length = (info.dwSize.Y - info.dwCursorPosition.Y) * info.dwSize.X;
    FillConsoleOutputCharacterA(hConsole, ' ', length, startPos, &written);
#else
    fprintf(stream, "\033[J");
    fflush(stream);
#endif
}

// readline.emitKeypressEvents(stream, interface)
void nova_readline_emitKeypressEvents(FILE* stream, void* interface_) {
    (void)stream;
    (void)interface_;
    // Enable keypress events - implementation depends on event loop integration
}

// ============================================================================
// Interface Methods
// ============================================================================

// interface.close()
void nova_readline_Interface_close(void* interface_) {
    auto* rl = static_cast<ReadlineInterface*>(interface_);
    if (rl->closed) return;

    rl->closed = true;

    if (rl->onClose) {
        rl->onClose();
    }
}

// interface.pause()
void nova_readline_Interface_pause(void* interface_) {
    auto* rl = static_cast<ReadlineInterface*>(interface_);
    if (rl->paused) return;

    rl->paused = true;

    if (rl->onPause) {
        rl->onPause();
    }
}

// interface.resume()
void nova_readline_Interface_resume(void* interface_) {
    auto* rl = static_cast<ReadlineInterface*>(interface_);
    if (!rl->paused) return;

    rl->paused = false;

    if (rl->onResume) {
        rl->onResume();
    }
}

// interface.setPrompt(prompt)
void nova_readline_Interface_setPrompt(void* interface_, const char* prompt) {
    auto* rl = static_cast<ReadlineInterface*>(interface_);
    rl->prompt = prompt ? prompt : "";
}

// interface.getPrompt()
const char* nova_readline_Interface_getPrompt(void* interface_) {
    auto* rl = static_cast<ReadlineInterface*>(interface_);
    return rl->prompt.c_str();
}

// interface.prompt(preserveCursor)
void nova_readline_Interface_prompt(void* interface_, bool preserveCursor) {
    auto* rl = static_cast<ReadlineInterface*>(interface_);
    if (rl->closed || rl->paused) return;

    if (!preserveCursor) {
        rl->line = "";
        rl->cursor = 0;
    }

    fprintf(rl->output, "%s%s", rl->prompt.c_str(), rl->line.c_str());
    fflush(rl->output);
}

// interface.question(query, callback)
void nova_readline_Interface_question(void* interface_, const char* query,
                                       void (*callback)(const char*)) {
    auto* rl = static_cast<ReadlineInterface*>(interface_);
    if (rl->closed) return;

    // Print the query
    fprintf(rl->output, "%s", query);
    fflush(rl->output);

    // Store callback
    if (callback) {
        questionCallbacks[rl] = callback;
    }

    // Read line synchronously for simplicity
    char buffer[4096];
    if (fgets(buffer, sizeof(buffer), rl->input)) {
        // Remove newline
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') {
            buffer[len - 1] = '\0';
            if (len > 1 && buffer[len - 2] == '\r') {
                buffer[len - 2] = '\0';
            }
        }

        if (callback) {
            callback(buffer);
        }
    }

    questionCallbacks.erase(rl);
}

// interface.write(data, key)
void nova_readline_Interface_write(void* interface_, const char* data, const char* key) {
    auto* rl = static_cast<ReadlineInterface*>(interface_);
    if (rl->closed) return;

    if (data) {
        // Insert data at cursor position
        rl->line.insert(rl->cursor, data);
        rl->cursor += strlen(data);
    }

    if (key) {
        // Handle special keys
        if (strcmp(key, "backspace") == 0 && rl->cursor > 0) {
            rl->line.erase(rl->cursor - 1, 1);
            rl->cursor--;
        } else if (strcmp(key, "delete") == 0 && rl->cursor < static_cast<int>(rl->line.size())) {
            rl->line.erase(rl->cursor, 1);
        } else if (strcmp(key, "left") == 0 && rl->cursor > 0) {
            rl->cursor--;
        } else if (strcmp(key, "right") == 0 && rl->cursor < static_cast<int>(rl->line.size())) {
            rl->cursor++;
        } else if (strcmp(key, "home") == 0) {
            rl->cursor = 0;
        } else if (strcmp(key, "end") == 0) {
            rl->cursor = static_cast<int>(rl->line.size());
        } else if (strcmp(key, "up") == 0) {
            // History navigation
            if (rl->historyIndex < static_cast<int>(rl->history.size()) - 1) {
                rl->historyIndex++;
                rl->line = rl->history[rl->historyIndex];
                rl->cursor = static_cast<int>(rl->line.size());
            }
        } else if (strcmp(key, "down") == 0) {
            if (rl->historyIndex > 0) {
                rl->historyIndex--;
                rl->line = rl->history[rl->historyIndex];
                rl->cursor = static_cast<int>(rl->line.size());
            } else if (rl->historyIndex == 0) {
                rl->historyIndex = -1;
                rl->line = "";
                rl->cursor = 0;
            }
        }
    }
}

// interface.line - Get current line
const char* nova_readline_Interface_line(void* interface_) {
    auto* rl = static_cast<ReadlineInterface*>(interface_);
    return rl->line.c_str();
}

// interface.cursor - Get cursor position
int nova_readline_Interface_cursor(void* interface_) {
    auto* rl = static_cast<ReadlineInterface*>(interface_);
    return rl->cursor;
}

// interface.terminal - Check if terminal
bool nova_readline_Interface_terminal(void* interface_) {
    auto* rl = static_cast<ReadlineInterface*>(interface_);
    return rl->terminal;
}

// interface.getCursorPos()
void nova_readline_Interface_getCursorPos(void* interface_, int* rows, int* cols) {
    auto* rl = static_cast<ReadlineInterface*>(interface_);
    (void)rl;

#ifdef _WIN32
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO info;
    if (GetConsoleScreenBufferInfo(hConsole, &info)) {
        *cols = info.dwCursorPosition.X;
        *rows = info.dwCursorPosition.Y;
    } else {
        *cols = 0;
        *rows = 0;
    }
#else
    *cols = 0;
    *rows = 0;
    // Would need to query terminal with escape sequences
#endif
}

// ============================================================================
// History Management
// ============================================================================

// Add line to history
void nova_readline_Interface_addHistory(void* interface_, const char* line) {
    auto* rl = static_cast<ReadlineInterface*>(interface_);

    if (!line || strlen(line) == 0) return;

    // Remove duplicates if enabled
    if (rl->removeHistoryDuplicates) {
        auto it = std::find(rl->history.begin(), rl->history.end(), line);
        if (it != rl->history.end()) {
            rl->history.erase(it);
        }
    }

    // Add to front
    rl->history.push_front(line);

    // Trim to size
    while (static_cast<int>(rl->history.size()) > rl->historySize) {
        rl->history.pop_back();
    }

    rl->historyIndex = -1;

    if (rl->onHistory) {
        std::vector<std::string> hist(rl->history.begin(), rl->history.end());
        rl->onHistory(hist);
    }
}

// Get history
const char** nova_readline_Interface_history(void* interface_, int* count) {
    static std::vector<const char*> histPtrs;
    auto* rl = static_cast<ReadlineInterface*>(interface_);

    histPtrs.clear();
    for (const auto& h : rl->history) {
        histPtrs.push_back(h.c_str());
    }

    *count = static_cast<int>(histPtrs.size());
    return histPtrs.data();
}

// Clear history
void nova_readline_Interface_clearHistory(void* interface_) {
    auto* rl = static_cast<ReadlineInterface*>(interface_);
    rl->history.clear();
    rl->historyIndex = -1;
}

// Set history size
void nova_readline_Interface_setHistorySize(void* interface_, int size) {
    auto* rl = static_cast<ReadlineInterface*>(interface_);
    rl->historySize = size > 0 ? size : 30;

    while (static_cast<int>(rl->history.size()) > rl->historySize) {
        rl->history.pop_back();
    }
}

// ============================================================================
// Event Handlers
// ============================================================================

// Set 'close' event handler
void nova_readline_Interface_onClose(void* interface_, void (*callback)()) {
    auto* rl = static_cast<ReadlineInterface*>(interface_);
    rl->onClose = callback;
}

// Set 'line' event handler
void nova_readline_Interface_onLine(void* interface_, void (*callback)(const char*)) {
    auto* rl = static_cast<ReadlineInterface*>(interface_);
    rl->onLine = callback;
}

// Set 'pause' event handler
void nova_readline_Interface_onPause(void* interface_, void (*callback)()) {
    auto* rl = static_cast<ReadlineInterface*>(interface_);
    rl->onPause = callback;
}

// Set 'resume' event handler
void nova_readline_Interface_onResume(void* interface_, void (*callback)()) {
    auto* rl = static_cast<ReadlineInterface*>(interface_);
    rl->onResume = callback;
}

// Set 'SIGINT' event handler
void nova_readline_Interface_onSIGINT(void* interface_, void (*callback)()) {
    auto* rl = static_cast<ReadlineInterface*>(interface_);
    rl->onSIGINT = callback;
}

// Set 'SIGTSTP' event handler
void nova_readline_Interface_onSIGTSTP(void* interface_, void (*callback)()) {
    auto* rl = static_cast<ReadlineInterface*>(interface_);
    rl->onSIGTSTP = callback;
}

// Set 'SIGCONT' event handler
void nova_readline_Interface_onSIGCONT(void* interface_, void (*callback)()) {
    auto* rl = static_cast<ReadlineInterface*>(interface_);
    rl->onSIGCONT = callback;
}

// ============================================================================
// Synchronous Line Reading
// ============================================================================

// Read a single line (blocking)
const char* nova_readline_readLine(void* interface_) {
    static std::string result;
    auto* rl = static_cast<ReadlineInterface*>(interface_);

    if (rl->closed) return nullptr;

    char buffer[4096];
    if (fgets(buffer, sizeof(buffer), rl->input)) {
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') {
            buffer[len - 1] = '\0';
            if (len > 1 && buffer[len - 2] == '\r') {
                buffer[len - 2] = '\0';
            }
        }

        result = buffer;

        // Add to history
        if (!result.empty()) {
            nova_readline_Interface_addHistory(interface_, result.c_str());
        }

        // Fire line event
        if (rl->onLine) {
            rl->onLine(result.c_str());
        }

        return result.c_str();
    }

    return nullptr;
}

// Read line with prompt
const char* nova_readline_readLineWithPrompt(void* interface_, const char* prompt) {
    auto* rl = static_cast<ReadlineInterface*>(interface_);

    if (prompt) {
        fprintf(rl->output, "%s", prompt);
        fflush(rl->output);
    }

    return nova_readline_readLine(interface_);
}

// ============================================================================
// Interface Properties
// ============================================================================

// Check if closed
bool nova_readline_Interface_closed(void* interface_) {
    auto* rl = static_cast<ReadlineInterface*>(interface_);
    return rl->closed;
}

// Check if paused
bool nova_readline_Interface_paused(void* interface_) {
    auto* rl = static_cast<ReadlineInterface*>(interface_);
    return rl->paused;
}

// Set completer function
void nova_readline_Interface_setCompleter(void* interface_, const char* completerName) {
    auto* rl = static_cast<ReadlineInterface*>(interface_);
    rl->completer = completerName ? completerName : "";
}

// Set removeHistoryDuplicates
void nova_readline_Interface_setRemoveHistoryDuplicates(void* interface_, bool value) {
    auto* rl = static_cast<ReadlineInterface*>(interface_);
    rl->removeHistoryDuplicates = value;
}

// Set crlfDelay
void nova_readline_Interface_setCrlfDelay(void* interface_, bool value) {
    auto* rl = static_cast<ReadlineInterface*>(interface_);
    rl->crlfDelay = value;
}

// Set escapeCodeTimeout
void nova_readline_Interface_setEscapeCodeTimeout(void* interface_, bool value) {
    auto* rl = static_cast<ReadlineInterface*>(interface_);
    rl->escapeCodeTimeout = value;
}

// Free interface
void nova_readline_Interface_free(void* interface_) {
    auto* rl = static_cast<ReadlineInterface*>(interface_);

    // Remove from active list
    auto it = std::find(interfaces.begin(), interfaces.end(), rl);
    if (it != interfaces.end()) {
        interfaces.erase(it);
    }

    questionCallbacks.erase(rl);
    delete rl;
}

// ============================================================================
// Promises API (readline/promises)
// ============================================================================

// Create interface (promises version returns same object)
void* nova_readline_promises_createInterface(FILE* input, FILE* output, const char* prompt,
                                              bool terminal, int historySize) {
    return nova_readline_createInterface(input, output, prompt, terminal, historySize);
}

// Question that would return a Promise (for async/await)
// In native code, we provide a callback-based version
void nova_readline_promises_question(void* interface_, const char* query,
                                      void (*resolve)(const char*), void (*reject)(const char*)) {
    auto* rl = static_cast<ReadlineInterface*>(interface_);

    if (rl->closed) {
        if (reject) reject("Interface is closed");
        return;
    }

    fprintf(rl->output, "%s", query);
    fflush(rl->output);

    char buffer[4096];
    if (fgets(buffer, sizeof(buffer), rl->input)) {
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') {
            buffer[len - 1] = '\0';
            if (len > 1 && buffer[len - 2] == '\r') {
                buffer[len - 2] = '\0';
            }
        }

        if (resolve) resolve(buffer);
    } else {
        if (reject) reject("EOF reached");
    }
}

// Question with AbortSignal support (options.signal)
void nova_readline_promises_questionWithSignal(void* interface_, const char* query,
                                                bool* aborted,
                                                void (*resolve)(const char*),
                                                void (*reject)(const char*)) {
    auto* rl = static_cast<ReadlineInterface*>(interface_);

    if (rl->closed) {
        if (reject) reject("Interface is closed");
        return;
    }

    if (aborted && *aborted) {
        if (reject) reject("AbortError: The operation was aborted");
        return;
    }

    fprintf(rl->output, "%s", query);
    fflush(rl->output);

    char buffer[4096];
    if (fgets(buffer, sizeof(buffer), rl->input)) {
        // Check abort before resolving
        if (aborted && *aborted) {
            if (reject) reject("AbortError: The operation was aborted");
            return;
        }

        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') {
            buffer[len - 1] = '\0';
            if (len > 1 && buffer[len - 2] == '\r') {
                buffer[len - 2] = '\0';
            }
        }

        if (resolve) resolve(buffer);
    } else {
        if (reject) reject("EOF reached");
    }
}

// Close interface (returns void, but async-compatible)
void nova_readline_promises_close(void* interface_, void (*resolve)(), void (*reject)(const char*)) {
    auto* rl = static_cast<ReadlineInterface*>(interface_);

    if (rl->closed) {
        if (resolve) resolve();
        return;
    }

    rl->closed = true;

    if (rl->onClose) {
        rl->onClose();
    }

    if (resolve) resolve();
    (void)reject;
}

// Commit the current line (used internally)
void nova_readline_promises_commit(void* interface_) {
    auto* rl = static_cast<ReadlineInterface*>(interface_);

    if (!rl->line.empty() && rl->onLine) {
        rl->onLine(rl->line.c_str());

        // Add to history
        nova_readline_Interface_addHistory(interface_, rl->line.c_str());
    }

    rl->line = "";
    rl->cursor = 0;
}

// Async iterator state
struct AsyncIteratorState {
    ReadlineInterface* rl;
    bool done;
    std::string currentValue;
};

// Create async iterator for interface (for-await-of support)
void* nova_readline_promises_createAsyncIterator(void* interface_) {
    auto* rl = static_cast<ReadlineInterface*>(interface_);
    auto* state = new AsyncIteratorState();
    state->rl = rl;
    state->done = false;
    return state;
}

// Get next value from async iterator
void nova_readline_promises_asyncIteratorNext(void* iterator,
                                               void (*resolve)(const char*, bool),
                                               void (*reject)(const char*)) {
    auto* state = static_cast<AsyncIteratorState*>(iterator);

    if (state->done || state->rl->closed) {
        if (resolve) resolve(nullptr, true);  // done: true
        return;
    }

    char buffer[4096];
    if (fgets(buffer, sizeof(buffer), state->rl->input)) {
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') {
            buffer[len - 1] = '\0';
            if (len > 1 && buffer[len - 2] == '\r') {
                buffer[len - 2] = '\0';
            }
        }

        state->currentValue = buffer;

        // Add to history
        if (!state->currentValue.empty()) {
            nova_readline_Interface_addHistory(state->rl, state->currentValue.c_str());
        }

        // Fire line event
        if (state->rl->onLine) {
            state->rl->onLine(state->currentValue.c_str());
        }

        if (resolve) resolve(state->currentValue.c_str(), false);  // done: false
    } else {
        state->done = true;
        if (resolve) resolve(nullptr, true);  // done: true
    }

    (void)reject;
}

// Return from async iterator (for break/return in for-await-of)
void nova_readline_promises_asyncIteratorReturn(void* iterator,
                                                 void (*resolve)(bool)) {
    auto* state = static_cast<AsyncIteratorState*>(iterator);
    state->done = true;

    // Close the interface
    nova_readline_Interface_close(state->rl);

    if (resolve) resolve(true);  // done: true
}

// Free async iterator
void nova_readline_promises_asyncIteratorFree(void* iterator) {
    delete static_cast<AsyncIteratorState*>(iterator);
}

// Readlines - async generator that yields all lines
void nova_readline_promises_readlines(void* interface_,
                                       void (*onLine)(const char*),
                                       void (*onDone)()) {
    auto* rl = static_cast<ReadlineInterface*>(interface_);

    char buffer[4096];
    while (!rl->closed && fgets(buffer, sizeof(buffer), rl->input)) {
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') {
            buffer[len - 1] = '\0';
            if (len > 1 && buffer[len - 2] == '\r') {
                buffer[len - 2] = '\0';
            }
        }

        if (onLine) onLine(buffer);
    }

    if (onDone) onDone();
}

// Interface.line property getter (same as regular)
const char* nova_readline_promises_Interface_line(void* interface_) {
    return nova_readline_Interface_line(interface_);
}

// Interface.cursor property getter
int nova_readline_promises_Interface_cursor(void* interface_) {
    return nova_readline_Interface_cursor(interface_);
}

// Set prompt (promises version)
void nova_readline_promises_setPrompt(void* interface_, const char* prompt) {
    nova_readline_Interface_setPrompt(interface_, prompt);
}

// Get prompt (promises version)
const char* nova_readline_promises_getPrompt(void* interface_) {
    return nova_readline_Interface_getPrompt(interface_);
}

// Write data (promises version)
void nova_readline_promises_write(void* interface_, const char* data, const char* key) {
    nova_readline_Interface_write(interface_, data, key);
}

// Pause (promises version)
void nova_readline_promises_pause(void* interface_) {
    nova_readline_Interface_pause(interface_);
}

// Resume (promises version)
void nova_readline_promises_resume(void* interface_) {
    nova_readline_Interface_resume(interface_);
}

// Check if closed (promises version)
bool nova_readline_promises_closed(void* interface_) {
    return nova_readline_Interface_closed(interface_);
}

// Free interface (promises version)
void nova_readline_promises_free(void* interface_) {
    nova_readline_Interface_free(interface_);
}

// Event handler setters for promises Interface
void nova_readline_promises_onClose(void* interface_, void (*callback)()) {
    nova_readline_Interface_onClose(interface_, callback);
}

void nova_readline_promises_onLine(void* interface_, void (*callback)(const char*)) {
    nova_readline_Interface_onLine(interface_, callback);
}

void nova_readline_promises_onPause(void* interface_, void (*callback)()) {
    nova_readline_Interface_onPause(interface_, callback);
}

void nova_readline_promises_onResume(void* interface_, void (*callback)()) {
    nova_readline_Interface_onResume(interface_, callback);
}

void nova_readline_promises_onSIGINT(void* interface_, void (*callback)()) {
    nova_readline_Interface_onSIGINT(interface_, callback);
}

void nova_readline_promises_onSIGTSTP(void* interface_, void (*callback)()) {
    nova_readline_Interface_onSIGTSTP(interface_, callback);
}

void nova_readline_promises_onSIGCONT(void* interface_, void (*callback)()) {
    nova_readline_Interface_onSIGCONT(interface_, callback);
}

// ============================================================================
// Utility Functions
// ============================================================================

// Check if stream is a TTY
bool nova_readline_isTerminal(FILE* stream) {
    if (!stream) return false;

#ifdef _WIN32
    return _isatty(_fileno(stream)) != 0;
#else
    return isatty(fileno(stream)) != 0;
#endif
}

// Get terminal columns
int nova_readline_getColumns(FILE* stream) {
    (void)stream;

#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO info;
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    if (GetConsoleScreenBufferInfo(hConsole, &info)) {
        return info.dwSize.X;
    }
    return 80;
#else
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0) {
        return w.ws_col;
    }
    return 80;
#endif
}

// Get terminal rows
int nova_readline_getRows(FILE* stream) {
    (void)stream;

#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO info;
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    if (GetConsoleScreenBufferInfo(hConsole, &info)) {
        return info.dwSize.Y;
    }
    return 24;
#else
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0) {
        return w.ws_row;
    }
    return 24;
#endif
}

// Direction constants
int nova_readline_CLEAR_LEFT() { return CLEAR_LEFT; }
int nova_readline_CLEAR_RIGHT() { return CLEAR_RIGHT; }
int nova_readline_CLEAR_BOTH() { return CLEAR_BOTH; }

// Cleanup
void nova_readline_cleanup() {
    for (auto* rl : interfaces) {
        delete rl;
    }
    interfaces.clear();
    questionCallbacks.clear();
}

} // extern "C"
