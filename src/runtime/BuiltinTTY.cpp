// Nova Builtin TTY Module Implementation
// Provides Node.js-compatible tty API

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2
#else
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#endif

extern "C" {

// ============================================================================
// TTY Detection
// ============================================================================

// Check if file descriptor is a TTY
int nova_tty_isatty(int fd) {
#ifdef _WIN32
    return _isatty(fd) ? 1 : 0;
#else
    return isatty(fd) ? 1 : 0;
#endif
}

// Check if stdin is a TTY
int nova_tty_isStdinTTY() {
    return nova_tty_isatty(STDIN_FILENO);
}

// Check if stdout is a TTY
int nova_tty_isStdoutTTY() {
    return nova_tty_isatty(STDOUT_FILENO);
}

// Check if stderr is a TTY
int nova_tty_isStderrTTY() {
    return nova_tty_isatty(STDERR_FILENO);
}

// ============================================================================
// ReadStream - TTY input stream
// ============================================================================

struct NovaReadStream {
    int fd;
    int isTTY;
    int isRaw;
#ifndef _WIN32
    struct termios originalTermios;
    int hasOriginalTermios;
#else
    DWORD originalMode;
    int hasOriginalMode;
#endif
};

// Create a ReadStream for given fd
void* nova_tty_ReadStream_create(int fd) {
    NovaReadStream* stream = new NovaReadStream();
    stream->fd = fd;
    stream->isTTY = nova_tty_isatty(fd);
    stream->isRaw = 0;
#ifndef _WIN32
    stream->hasOriginalTermios = 0;
#else
    stream->hasOriginalMode = 0;
#endif
    return stream;
}

// Create ReadStream for stdin
void* nova_tty_ReadStream_createStdin() {
    return nova_tty_ReadStream_create(STDIN_FILENO);
}

// Check if ReadStream is TTY
int nova_tty_ReadStream_isTTY(void* streamPtr) {
    if (!streamPtr) return 0;
    return ((NovaReadStream*)streamPtr)->isTTY;
}

// Check if ReadStream is in raw mode
int nova_tty_ReadStream_isRaw(void* streamPtr) {
    if (!streamPtr) return 0;
    return ((NovaReadStream*)streamPtr)->isRaw;
}

// Set raw mode on ReadStream
int nova_tty_ReadStream_setRawMode(void* streamPtr, int mode) {
    if (!streamPtr) return 0;
    NovaReadStream* stream = (NovaReadStream*)streamPtr;

    if (!stream->isTTY) return 0;

#ifdef _WIN32
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    if (hStdin == INVALID_HANDLE_VALUE) return 0;

    if (mode) {
        // Enable raw mode
        if (!stream->hasOriginalMode) {
            GetConsoleMode(hStdin, &stream->originalMode);
            stream->hasOriginalMode = 1;
        }
        DWORD newMode = stream->originalMode;
        newMode &= ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_PROCESSED_INPUT);
        newMode |= ENABLE_VIRTUAL_TERMINAL_INPUT;
        SetConsoleMode(hStdin, newMode);
        stream->isRaw = 1;
    } else {
        // Disable raw mode
        if (stream->hasOriginalMode) {
            SetConsoleMode(hStdin, stream->originalMode);
        }
        stream->isRaw = 0;
    }
    return 1;
#else
    if (mode) {
        // Enable raw mode
        if (!stream->hasOriginalTermios) {
            tcgetattr(stream->fd, &stream->originalTermios);
            stream->hasOriginalTermios = 1;
        }
        struct termios raw = stream->originalTermios;
        raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
        raw.c_oflag &= ~(OPOST);
        raw.c_cflag |= (CS8);
        raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
        raw.c_cc[VMIN] = 1;
        raw.c_cc[VTIME] = 0;
        tcsetattr(stream->fd, TCSAFLUSH, &raw);
        stream->isRaw = 1;
    } else {
        // Disable raw mode
        if (stream->hasOriginalTermios) {
            tcsetattr(stream->fd, TCSAFLUSH, &stream->originalTermios);
        }
        stream->isRaw = 0;
    }
    return 1;
#endif
}

// Get file descriptor
int nova_tty_ReadStream_fd(void* streamPtr) {
    if (!streamPtr) return -1;
    return ((NovaReadStream*)streamPtr)->fd;
}

// Free ReadStream
void nova_tty_ReadStream_free(void* streamPtr) {
    if (!streamPtr) return;
    NovaReadStream* stream = (NovaReadStream*)streamPtr;
    // Restore original mode if in raw mode
    if (stream->isRaw) {
        nova_tty_ReadStream_setRawMode(streamPtr, 0);
    }
    delete stream;
}

// ============================================================================
// WriteStream - TTY output stream
// ============================================================================

struct NovaWriteStream {
    int fd;
    int isTTY;
    int columns;
    int rows;
};

// Get terminal size
static void getTerminalSize(int* cols, int* rows) {
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
    if (GetConsoleScreenBufferInfo(hStdout, &csbi)) {
        *cols = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        *rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    } else {
        *cols = 80;
        *rows = 24;
    }
#else
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0) {
        *cols = ws.ws_col;
        *rows = ws.ws_row;
    } else {
        *cols = 80;
        *rows = 24;
    }
#endif
}

// Create a WriteStream for given fd
void* nova_tty_WriteStream_create(int fd) {
    NovaWriteStream* stream = new NovaWriteStream();
    stream->fd = fd;
    stream->isTTY = nova_tty_isatty(fd);
    getTerminalSize(&stream->columns, &stream->rows);
    return stream;
}

// Create WriteStream for stdout
void* nova_tty_WriteStream_createStdout() {
    return nova_tty_WriteStream_create(STDOUT_FILENO);
}

// Create WriteStream for stderr
void* nova_tty_WriteStream_createStderr() {
    return nova_tty_WriteStream_create(STDERR_FILENO);
}

// Check if WriteStream is TTY
int nova_tty_WriteStream_isTTY(void* streamPtr) {
    if (!streamPtr) return 0;
    return ((NovaWriteStream*)streamPtr)->isTTY;
}

// Get terminal columns
int nova_tty_WriteStream_columns(void* streamPtr) {
    if (!streamPtr) return 80;
    NovaWriteStream* stream = (NovaWriteStream*)streamPtr;
    getTerminalSize(&stream->columns, &stream->rows);
    return stream->columns;
}

// Get terminal rows
int nova_tty_WriteStream_rows(void* streamPtr) {
    if (!streamPtr) return 24;
    NovaWriteStream* stream = (NovaWriteStream*)streamPtr;
    getTerminalSize(&stream->columns, &stream->rows);
    return stream->rows;
}

// Get window size as array [columns, rows]
void nova_tty_WriteStream_getWindowSize(void* streamPtr, int* out) {
    if (!streamPtr || !out) return;
    NovaWriteStream* stream = (NovaWriteStream*)streamPtr;
    getTerminalSize(&stream->columns, &stream->rows);
    out[0] = stream->columns;
    out[1] = stream->rows;
}

// Get file descriptor
int nova_tty_WriteStream_fd(void* streamPtr) {
    if (!streamPtr) return -1;
    return ((NovaWriteStream*)streamPtr)->fd;
}

// Free WriteStream
void nova_tty_WriteStream_free(void* streamPtr) {
    if (streamPtr) delete (NovaWriteStream*)streamPtr;
}

// ============================================================================
// Cursor and Screen Control (ANSI escape sequences)
// ============================================================================

// Clear entire line
void nova_tty_WriteStream_clearLine(void* streamPtr, int dir) {
    if (!streamPtr) return;
    NovaWriteStream* stream = (NovaWriteStream*)streamPtr;
    if (!stream->isTTY) return;

    // dir: -1 = left of cursor, 0 = entire line, 1 = right of cursor
    const char* seq;
    switch (dir) {
        case -1: seq = "\x1b[1K"; break;  // Clear left
        case 1:  seq = "\x1b[0K"; break;  // Clear right
        default: seq = "\x1b[2K"; break;  // Clear entire line
    }
#ifdef _WIN32
    DWORD written;
    WriteConsoleA(GetStdHandle(stream->fd == STDERR_FILENO ? STD_ERROR_HANDLE : STD_OUTPUT_HANDLE),
                  seq, (DWORD)strlen(seq), &written, NULL);
#else
    write(stream->fd, seq, strlen(seq));
#endif
}

// Clear screen from cursor down
void nova_tty_WriteStream_clearScreenDown(void* streamPtr) {
    if (!streamPtr) return;
    NovaWriteStream* stream = (NovaWriteStream*)streamPtr;
    if (!stream->isTTY) return;

    const char* seq = "\x1b[0J";
#ifdef _WIN32
    DWORD written;
    WriteConsoleA(GetStdHandle(stream->fd == STDERR_FILENO ? STD_ERROR_HANDLE : STD_OUTPUT_HANDLE),
                  seq, (DWORD)strlen(seq), &written, NULL);
#else
    write(stream->fd, seq, strlen(seq));
#endif
}

// Move cursor to absolute position
void nova_tty_WriteStream_cursorTo(void* streamPtr, int x, int y) {
    if (!streamPtr) return;
    NovaWriteStream* stream = (NovaWriteStream*)streamPtr;
    if (!stream->isTTY) return;

    char seq[32];
    if (y < 0) {
        // Only move X
        snprintf(seq, sizeof(seq), "\x1b[%dG", x + 1);
    } else {
        snprintf(seq, sizeof(seq), "\x1b[%d;%dH", y + 1, x + 1);
    }
#ifdef _WIN32
    DWORD written;
    WriteConsoleA(GetStdHandle(stream->fd == STDERR_FILENO ? STD_ERROR_HANDLE : STD_OUTPUT_HANDLE),
                  seq, (DWORD)strlen(seq), &written, NULL);
#else
    write(stream->fd, seq, strlen(seq));
#endif
}

// Move cursor relative to current position
void nova_tty_WriteStream_moveCursor(void* streamPtr, int dx, int dy) {
    if (!streamPtr) return;
    NovaWriteStream* stream = (NovaWriteStream*)streamPtr;
    if (!stream->isTTY) return;

    char seq[64] = "";
    char tmp[32];

    if (dx > 0) {
        snprintf(tmp, sizeof(tmp), "\x1b[%dC", dx);
        strcat(seq, tmp);
    } else if (dx < 0) {
        snprintf(tmp, sizeof(tmp), "\x1b[%dD", -dx);
        strcat(seq, tmp);
    }

    if (dy > 0) {
        snprintf(tmp, sizeof(tmp), "\x1b[%dB", dy);
        strcat(seq, tmp);
    } else if (dy < 0) {
        snprintf(tmp, sizeof(tmp), "\x1b[%dA", -dy);
        strcat(seq, tmp);
    }

    if (strlen(seq) > 0) {
#ifdef _WIN32
        DWORD written;
        WriteConsoleA(GetStdHandle(stream->fd == STDERR_FILENO ? STD_ERROR_HANDLE : STD_OUTPUT_HANDLE),
                      seq, (DWORD)strlen(seq), &written, NULL);
#else
        write(stream->fd, seq, strlen(seq));
#endif
    }
}

// ============================================================================
// Color Support
// ============================================================================

// Get color depth (1, 4, 8, or 24 bits)
int nova_tty_WriteStream_getColorDepth(void* streamPtr) {
    if (!streamPtr) return 1;
    NovaWriteStream* stream = (NovaWriteStream*)streamPtr;
    if (!stream->isTTY) return 1;

#ifdef _WIN32
    // Modern Windows 10+ supports 24-bit color
    DWORD mode;
    HANDLE hOut = GetStdHandle(stream->fd == STDERR_FILENO ? STD_ERROR_HANDLE : STD_OUTPUT_HANDLE);
    if (GetConsoleMode(hOut, &mode)) {
        if (mode & ENABLE_VIRTUAL_TERMINAL_PROCESSING) {
            return 24;  // True color
        }
    }
    return 4;  // Basic 16 colors
#else
    // Check COLORTERM environment variable
    const char* colorterm = getenv("COLORTERM");
    if (colorterm) {
        if (strcmp(colorterm, "truecolor") == 0 || strcmp(colorterm, "24bit") == 0) {
            return 24;
        }
    }

    // Check TERM environment variable
    const char* term = getenv("TERM");
    if (term) {
        if (strstr(term, "256color") || strstr(term, "256")) {
            return 8;
        }
        if (strstr(term, "color") || strstr(term, "xterm") || strstr(term, "screen")) {
            return 4;
        }
    }

    return 1;  // Monochrome
#endif
}

// Check if terminal has at least count colors
int nova_tty_WriteStream_hasColors(void* streamPtr, int count) {
    int depth = nova_tty_WriteStream_getColorDepth(streamPtr);
    int available;
    switch (depth) {
        case 24: available = 16777216; break;
        case 8:  available = 256; break;
        case 4:  available = 16; break;
        default: available = 2; break;
    }
    return (count <= available) ? 1 : 0;
}

// ============================================================================
// Additional TTY utilities
// ============================================================================

// Get terminal columns (global)
int nova_tty_getColumns() {
    int cols, rows;
    getTerminalSize(&cols, &rows);
    return cols;
}

// Get terminal rows (global)
int nova_tty_getRows() {
    int cols, rows;
    getTerminalSize(&cols, &rows);
    return rows;
}

// Enable virtual terminal processing (Windows)
int nova_tty_enableVirtualTerminal() {
#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    HANDLE hErr = GetStdHandle(STD_ERROR_HANDLE);
    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);

    DWORD mode;
    int success = 1;

    if (GetConsoleMode(hOut, &mode)) {
        mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        if (!SetConsoleMode(hOut, mode)) success = 0;
    }

    if (GetConsoleMode(hErr, &mode)) {
        mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(hErr, mode);
    }

    if (GetConsoleMode(hIn, &mode)) {
        mode |= ENABLE_VIRTUAL_TERMINAL_INPUT;
        SetConsoleMode(hIn, mode);
    }

    return success;
#else
    return 1;  // Always supported on Unix
#endif
}

// Hide cursor
void nova_tty_hideCursor(void* streamPtr) {
    if (!streamPtr) return;
    NovaWriteStream* stream = (NovaWriteStream*)streamPtr;
    if (!stream->isTTY) return;

    const char* seq = "\x1b[?25l";
#ifdef _WIN32
    DWORD written;
    WriteConsoleA(GetStdHandle(stream->fd == STDERR_FILENO ? STD_ERROR_HANDLE : STD_OUTPUT_HANDLE),
                  seq, (DWORD)strlen(seq), &written, NULL);
#else
    write(stream->fd, seq, strlen(seq));
#endif
}

// Show cursor
void nova_tty_showCursor(void* streamPtr) {
    if (!streamPtr) return;
    NovaWriteStream* stream = (NovaWriteStream*)streamPtr;
    if (!stream->isTTY) return;

    const char* seq = "\x1b[?25h";
#ifdef _WIN32
    DWORD written;
    WriteConsoleA(GetStdHandle(stream->fd == STDERR_FILENO ? STD_ERROR_HANDLE : STD_OUTPUT_HANDLE),
                  seq, (DWORD)strlen(seq), &written, NULL);
#else
    write(stream->fd, seq, strlen(seq));
#endif
}

// Save cursor position
void nova_tty_saveCursor(void* streamPtr) {
    if (!streamPtr) return;
    NovaWriteStream* stream = (NovaWriteStream*)streamPtr;
    if (!stream->isTTY) return;

    const char* seq = "\x1b[s";
#ifdef _WIN32
    DWORD written;
    WriteConsoleA(GetStdHandle(stream->fd == STDERR_FILENO ? STD_ERROR_HANDLE : STD_OUTPUT_HANDLE),
                  seq, (DWORD)strlen(seq), &written, NULL);
#else
    write(stream->fd, seq, strlen(seq));
#endif
}

// Restore cursor position
void nova_tty_restoreCursor(void* streamPtr) {
    if (!streamPtr) return;
    NovaWriteStream* stream = (NovaWriteStream*)streamPtr;
    if (!stream->isTTY) return;

    const char* seq = "\x1b[u";
#ifdef _WIN32
    DWORD written;
    WriteConsoleA(GetStdHandle(stream->fd == STDERR_FILENO ? STD_ERROR_HANDLE : STD_OUTPUT_HANDLE),
                  seq, (DWORD)strlen(seq), &written, NULL);
#else
    write(stream->fd, seq, strlen(seq));
#endif
}

// Get cursor position (returns 0 on success, -1 on failure)
int nova_tty_getCursorPosition(void* streamPtr, int* x, int* y) {
    if (!streamPtr || !x || !y) return -1;
    NovaWriteStream* stream = (NovaWriteStream*)streamPtr;
    if (!stream->isTTY) return -1;

#ifdef _WIN32
    HANDLE hOut = GetStdHandle(stream->fd == STDERR_FILENO ? STD_ERROR_HANDLE : STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(hOut, &csbi)) {
        *x = csbi.dwCursorPosition.X;
        *y = csbi.dwCursorPosition.Y;
        return 0;
    }
    return -1;
#else
    // This requires reading from stdin which is complex
    // Return a default for now
    *x = 0;
    *y = 0;
    return -1;
#endif
}

// Clear entire screen
void nova_tty_clearScreen(void* streamPtr) {
    if (!streamPtr) return;
    NovaWriteStream* stream = (NovaWriteStream*)streamPtr;
    if (!stream->isTTY) return;

    const char* seq = "\x1b[2J\x1b[H";
#ifdef _WIN32
    DWORD written;
    WriteConsoleA(GetStdHandle(stream->fd == STDERR_FILENO ? STD_ERROR_HANDLE : STD_OUTPUT_HANDLE),
                  seq, (DWORD)strlen(seq), &written, NULL);
#else
    write(stream->fd, seq, strlen(seq));
#endif
}

// Scroll up
void nova_tty_scrollUp(void* streamPtr, int lines) {
    if (!streamPtr || lines <= 0) return;
    NovaWriteStream* stream = (NovaWriteStream*)streamPtr;
    if (!stream->isTTY) return;

    char seq[32];
    snprintf(seq, sizeof(seq), "\x1b[%dS", lines);
#ifdef _WIN32
    DWORD written;
    WriteConsoleA(GetStdHandle(stream->fd == STDERR_FILENO ? STD_ERROR_HANDLE : STD_OUTPUT_HANDLE),
                  seq, (DWORD)strlen(seq), &written, NULL);
#else
    write(stream->fd, seq, strlen(seq));
#endif
}

// Scroll down
void nova_tty_scrollDown(void* streamPtr, int lines) {
    if (!streamPtr || lines <= 0) return;
    NovaWriteStream* stream = (NovaWriteStream*)streamPtr;
    if (!stream->isTTY) return;

    char seq[32];
    snprintf(seq, sizeof(seq), "\x1b[%dT", lines);
#ifdef _WIN32
    DWORD written;
    WriteConsoleA(GetStdHandle(stream->fd == STDERR_FILENO ? STD_ERROR_HANDLE : STD_OUTPUT_HANDLE),
                  seq, (DWORD)strlen(seq), &written, NULL);
#else
    write(stream->fd, seq, strlen(seq));
#endif
}

// Set terminal title
void nova_tty_setTitle(void* streamPtr, const char* title) {
    if (!streamPtr || !title) return;
    NovaWriteStream* stream = (NovaWriteStream*)streamPtr;
    if (!stream->isTTY) return;

#ifdef _WIN32
    SetConsoleTitleA(title);
#else
    char seq[512];
    snprintf(seq, sizeof(seq), "\x1b]0;%s\x07", title);
    write(stream->fd, seq, strlen(seq));
#endif
}

// Ring the terminal bell
void nova_tty_bell(void* streamPtr) {
    if (!streamPtr) return;
    NovaWriteStream* stream = (NovaWriteStream*)streamPtr;

    const char* seq = "\x07";
#ifdef _WIN32
    DWORD written;
    WriteConsoleA(GetStdHandle(stream->fd == STDERR_FILENO ? STD_ERROR_HANDLE : STD_OUTPUT_HANDLE),
                  seq, (DWORD)strlen(seq), &written, NULL);
#else
    write(stream->fd, seq, strlen(seq));
#endif
}

// Reset terminal attributes
void nova_tty_reset(void* streamPtr) {
    if (!streamPtr) return;
    NovaWriteStream* stream = (NovaWriteStream*)streamPtr;
    if (!stream->isTTY) return;

    const char* seq = "\x1b[0m";
#ifdef _WIN32
    DWORD written;
    WriteConsoleA(GetStdHandle(stream->fd == STDERR_FILENO ? STD_ERROR_HANDLE : STD_OUTPUT_HANDLE),
                  seq, (DWORD)strlen(seq), &written, NULL);
#else
    write(stream->fd, seq, strlen(seq));
#endif
}

// ============================================================================
// Alternative screen buffer
// ============================================================================

// Enter alternative screen buffer
void nova_tty_enterAlternateScreen(void* streamPtr) {
    if (!streamPtr) return;
    NovaWriteStream* stream = (NovaWriteStream*)streamPtr;
    if (!stream->isTTY) return;

    const char* seq = "\x1b[?1049h";
#ifdef _WIN32
    DWORD written;
    WriteConsoleA(GetStdHandle(stream->fd == STDERR_FILENO ? STD_ERROR_HANDLE : STD_OUTPUT_HANDLE),
                  seq, (DWORD)strlen(seq), &written, NULL);
#else
    write(stream->fd, seq, strlen(seq));
#endif
}

// Leave alternative screen buffer
void nova_tty_leaveAlternateScreen(void* streamPtr) {
    if (!streamPtr) return;
    NovaWriteStream* stream = (NovaWriteStream*)streamPtr;
    if (!stream->isTTY) return;

    const char* seq = "\x1b[?1049l";
#ifdef _WIN32
    DWORD written;
    WriteConsoleA(GetStdHandle(stream->fd == STDERR_FILENO ? STD_ERROR_HANDLE : STD_OUTPUT_HANDLE),
                  seq, (DWORD)strlen(seq), &written, NULL);
#else
    write(stream->fd, seq, strlen(seq));
#endif
}

} // extern "C"
