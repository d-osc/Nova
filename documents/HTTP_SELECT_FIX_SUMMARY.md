# Windows Winsock2 select() Fix for Nova HTTP Server

## Problem Description

The Nova HTTP server on Windows was experiencing an issue where `select()` continuously returned 0 (timeout) and never detected incoming connections, even when curl was actively trying to connect. The debug output showed:

```
DEBUG acceptOne: select returned 0, readfds.fd_count=0
```

This prevented the HTTP server from accepting any connections and blocked HTTP throughput benchmarks.

## Root Cause

The issue was **NOT** with `select()` itself, but rather with the codebase containing overly complex diagnostics code that was:

1. **Toggling socket blocking mode** - The code attempted a non-blocking `accept()` before `select()`, then switched the socket back to blocking mode. This mode switching on Windows with `ioctlsocket(FIONBIO)` was unnecessary and potentially interfering with socket event detection.

2. **Overcomplicating the event loop** - The acceptOne() function had over 150 lines of Windows-specific diagnostic code that made debugging harder and obscured the actual issue.

3. **Cache confusion** - Multiple old versions of the compiled code were running simultaneously on port 3000 (7 processes!), making it appear that connections were succeeding when they were actually connecting to old servers.

## The Fix

The fix simplified the `nova_http_Server_acceptOne()` function significantly:

### Key Changes:

1. **Removed non-blocking accept() attempt** - Eliminated the diagnostic code that tried a non-blocking accept before select()

2. **Simplified select() call** - Used a straightforward blocking select() pattern:
   ```cpp
   fd_set readfds;
   FD_ZERO(&readfds);
   FD_SET(server->socket, &readfds);

   struct timeval tv;
   tv.tv_sec = timeoutMs / 1000;
   tv.tv_usec = (timeoutMs % 1000) * 1000;

   #ifdef _WIN32
   int selectResult = select(0, &readfds, NULL, NULL, &tv);
   #else
   int selectResult = select((int)server->socket + 1, &readfds, NULL, NULL, &tv);
   #endif
   ```

3. **Added explicit blocking mode setting** in `nova_http_Server_listen()`:
   ```cpp
   #ifdef _WIN32
   // Explicitly ensure socket is in blocking mode for select() to work properly
   u_long blockingMode = 0;  // 0 = blocking, 1 = non-blocking
   ioctlsocket(server->socket, FIONBIO, &blockingMode);
   #endif
   ```

4. **Changed nullptr to NULL** - Used C-style NULL pointers for better compatibility with Windows Winsock2 API

### Files Modified:

- `C:\Users\ondev\Projects\Nova\src\runtime\BuiltinHTTP.cpp`
  - Lines 493-504: Added explicit blocking mode setting after listen()
  - Lines 1185-1250: Simplified acceptOne() function

## Verification

After the fix, the server successfully handles multiple requests:

```
DEBUG acceptOne: select() returned 1
DEBUG acceptOne: Connection detected, calling accept()
DEBUG acceptOne: accept() successful, clientSocket=264
DEBUG acceptOne: Request handler returned
DEBUG nova_http_Server_run: Handled request #1
...
DEBUG nova_http_Server_run: Handled request #7
```

Tested with curl:
```bash
curl http://127.0.0.1:3000/
# Output: Hello World
```

Multiple sequential requests work perfectly.

## Technical Insights

### Why This Works on Windows

1. **Winsock2 select() behavior** - On Windows, select() requires the socket to be in a consistent state. The constant mode toggling was potentially confusing the Windows socket layer.

2. **Explicit blocking mode** - While sockets are created in blocking mode by default, explicitly setting the mode ensures there's no ambiguity or residual state from previous operations.

3. **Simplicity** - Removing the diagnostic code eliminated potential race conditions and state inconsistencies.

### Performance Considerations

The simplified code is actually MORE efficient:
- No unnecessary non-blocking accept() call before select()
- No mode switching overhead
- Cleaner code path with fewer branches

## Build Instructions

To rebuild with the fix:

```bash
cd C:\Users\ondev\Projects\Nova
rm -rf .nova-cache
rm -f build/novacore.dir/Release/BuiltinHTTP.obj build/Release/novacore.lib build/Release/nova.exe
cmake --build build --config Release
```

Note: Always clear the `.nova-cache` directory after modifying runtime C++ code to ensure the Nova compiler relinks against the updated library.

## Next Steps

Now that the socket polling issue is fixed, you can:

1. **Run HTTP benchmarks** - Compare Nova vs Node.js vs Bun vs Deno
2. **Test concurrent connections** - Verify behavior under load
3. **Remove debug output** - Clean up fprintf() statements for production
4. **Optimize performance** - Consider non-blocking I/O or async patterns for higher throughput

## Date

Fixed on: December 3, 2025
