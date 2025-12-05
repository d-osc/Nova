// Nova Stream Module - Node.js compatible stream API
// Provides streaming data handling

#include <string>
#include <vector>
#include <deque>
#include <map>
#include <functional>
#include <cstring>
#include <cstdint>
#include <algorithm>

// Stream states
static const int STREAM_STATE_INITIAL = 0;
static const int STREAM_STATE_READABLE = 1;
static const int STREAM_STATE_WRITABLE = 2;
static const int STREAM_STATE_FLOWING = 4;
static const int STREAM_STATE_PAUSED = 8;
static const int STREAM_STATE_ENDED = 16;
static const int STREAM_STATE_FINISHED = 32;
static const int STREAM_STATE_DESTROYED = 64;
static const int STREAM_STATE_ERROR = 128;

// Default high water mark
static size_t defaultHighWaterMark = 16384;  // 16KB
static size_t defaultObjectHighWaterMark = 16;

// Buffer chunk
struct StreamChunk {
    std::vector<uint8_t> data;
    std::string encoding;
    bool isObject;
};

// Base stream structure
struct StreamBase {
    int state;
    size_t highWaterMark;
    bool objectMode;
    std::string defaultEncoding;
    std::deque<StreamChunk> buffer;
    size_t bufferSize;
    std::string lastError;

    // Event callbacks
    std::function<void()> onClose;
    std::function<void(const char*)> onError;
    std::function<void()> onDrain;
    std::function<void()> onFinish;
    std::function<void()> onEnd;
    std::function<void(const uint8_t*, size_t)> onData;
    std::function<void()> onReadable;
    std::function<void(StreamBase*)> onPipe;
    std::function<void(StreamBase*)> onUnpipe;

    StreamBase() : state(STREAM_STATE_INITIAL), highWaterMark(defaultHighWaterMark),
                   objectMode(false), defaultEncoding("utf8"), bufferSize(0) {}
};

// Readable stream
struct ReadableStream : public StreamBase {
    std::function<void(size_t)> readImpl;
    std::function<void()> destroyImpl;
    std::vector<StreamBase*> pipes;
    bool readableEnded;
    size_t readableLength;
    bool readableFlowing;

    ReadableStream() : readableEnded(false), readableLength(0), readableFlowing(false) {
        state |= STREAM_STATE_READABLE;
    }
};

// Writable stream
struct WritableStream : public StreamBase {
    std::function<void(const uint8_t*, size_t, const char*, std::function<void()>)> writeImpl;
    std::function<void(std::function<void()>)> finalImpl;
    std::function<void()> destroyImpl;
    bool writableEnded;
    bool writableFinished;
    size_t writableLength;
    bool writableNeedDrain;
    int writableCorked;

    WritableStream() : writableEnded(false), writableFinished(false),
                       writableLength(0), writableNeedDrain(false), writableCorked(0) {
        state |= STREAM_STATE_WRITABLE;
    }
};

// Duplex stream (both readable and writable)
struct DuplexStream : public StreamBase {
    ReadableStream readable;
    WritableStream writable;
    bool allowHalfOpen;

    DuplexStream() : allowHalfOpen(true) {
        state |= (STREAM_STATE_READABLE | STREAM_STATE_WRITABLE);
    }
};

// Transform stream
struct TransformStream : public DuplexStream {
    std::function<void(const uint8_t*, size_t, const char*, std::function<void(const uint8_t*, size_t)>)> transformImpl;
    std::function<void(std::function<void()>)> flushImpl;

    TransformStream() {}
};

// PassThrough stream
struct PassThroughStream : public TransformStream {
    PassThroughStream() {
        // PassThrough just passes data through unchanged
        transformImpl = [](const uint8_t* data, size_t len, const char*, std::function<void(const uint8_t*, size_t)> push) {
            push(data, len);
        };
    }
};

// Active streams
static std::vector<StreamBase*> streams;

extern "C" {

// ============================================================================
// Module-level Functions
// ============================================================================

// stream.getDefaultHighWaterMark(objectMode)
size_t nova_stream_getDefaultHighWaterMark(bool objectMode) {
    return objectMode ? defaultObjectHighWaterMark : defaultHighWaterMark;
}

// stream.setDefaultHighWaterMark(objectMode, value)
void nova_stream_setDefaultHighWaterMark(bool objectMode, size_t value) {
    if (objectMode) {
        defaultObjectHighWaterMark = value;
    } else {
        defaultHighWaterMark = value;
    }
}

// ============================================================================
// Readable Stream
// ============================================================================

// Create readable stream
void* nova_stream_Readable_new(size_t highWaterMark, bool objectMode, const char* encoding) {
    auto* stream = new ReadableStream();
    stream->highWaterMark = highWaterMark > 0 ? highWaterMark :
                           (objectMode ? defaultObjectHighWaterMark : defaultHighWaterMark);
    stream->objectMode = objectMode;
    stream->defaultEncoding = encoding ? encoding : "utf8";

    streams.push_back(stream);
    return stream;
}

// readable.read(size)
const uint8_t* nova_stream_Readable_read(void* stream, size_t size, size_t* outSize) {
    static std::vector<uint8_t> result;
    auto* s = static_cast<ReadableStream*>(stream);

    result.clear();

    if (s->buffer.empty()) {
        *outSize = 0;
        return nullptr;
    }

    size_t toRead = (size == 0) ? s->bufferSize : std::min(size, s->bufferSize);

    while (!s->buffer.empty() && result.size() < toRead) {
        auto& chunk = s->buffer.front();
        size_t needed = toRead - result.size();

        if (chunk.data.size() <= needed) {
            result.insert(result.end(), chunk.data.begin(), chunk.data.end());
            s->bufferSize -= chunk.data.size();
            s->buffer.pop_front();
        } else {
            result.insert(result.end(), chunk.data.begin(), chunk.data.begin() + needed);
            chunk.data.erase(chunk.data.begin(), chunk.data.begin() + needed);
            s->bufferSize -= needed;
        }
    }

    s->readableLength = s->bufferSize;
    *outSize = result.size();
    return result.data();
}

// readable.push(chunk)
bool nova_stream_Readable_push(void* stream, const uint8_t* data, size_t len) {
    auto* s = static_cast<ReadableStream*>(stream);

    if (s->readableEnded) return false;

    if (data == nullptr) {
        // null signals end of stream
        s->readableEnded = true;
        s->state |= STREAM_STATE_ENDED;
        if (s->onEnd) s->onEnd();
        return false;
    }

    StreamChunk chunk;
    chunk.data.assign(data, data + len);
    chunk.encoding = s->defaultEncoding;
    chunk.isObject = s->objectMode;

    s->buffer.push_back(chunk);
    s->bufferSize += len;
    s->readableLength = s->bufferSize;

    if (s->onReadable) s->onReadable();

    if (s->readableFlowing && s->onData) {
        s->onData(data, len);
    }

    return s->bufferSize < s->highWaterMark;
}

// readable.unshift(chunk)
void nova_stream_Readable_unshift(void* stream, const uint8_t* data, size_t len) {
    auto* s = static_cast<ReadableStream*>(stream);

    StreamChunk chunk;
    chunk.data.assign(data, data + len);
    chunk.encoding = s->defaultEncoding;

    s->buffer.push_front(chunk);
    s->bufferSize += len;
    s->readableLength = s->bufferSize;
}

// readable.pause()
void nova_stream_Readable_pause(void* stream) {
    auto* s = static_cast<ReadableStream*>(stream);
    s->readableFlowing = false;
    s->state |= STREAM_STATE_PAUSED;
    s->state &= ~STREAM_STATE_FLOWING;
}

// readable.resume()
void nova_stream_Readable_resume(void* stream) {
    auto* s = static_cast<ReadableStream*>(stream);
    s->readableFlowing = true;
    s->state |= STREAM_STATE_FLOWING;
    s->state &= ~STREAM_STATE_PAUSED;

    // Emit buffered data
    while (s->readableFlowing && !s->buffer.empty()) {
        auto& chunk = s->buffer.front();
        if (s->onData) {
            s->onData(chunk.data.data(), chunk.data.size());
        }
        s->bufferSize -= chunk.data.size();
        s->buffer.pop_front();
    }
    s->readableLength = s->bufferSize;
}

// readable.isPaused()
bool nova_stream_Readable_isPaused(void* stream) {
    auto* s = static_cast<ReadableStream*>(stream);
    return !s->readableFlowing;
}

// readable.pipe(destination)
void* nova_stream_Readable_pipe(void* stream, void* destination) {
    auto* src = static_cast<ReadableStream*>(stream);
    auto* dst = static_cast<WritableStream*>(destination);

    src->pipes.push_back(dst);

    if (src->onPipe) src->onPipe(dst);

    // Set up data flow
    src->onData = [dst](const uint8_t* data, size_t len) {
        // Write to destination (simplified)
        if (dst->writeImpl) {
            dst->writeImpl(data, len, "utf8", []() {});
        }
    };

    src->readableFlowing = true;
    return destination;
}

// readable.unpipe(destination)
void nova_stream_Readable_unpipe(void* stream, void* destination) {
    auto* src = static_cast<ReadableStream*>(stream);

    if (destination) {
        auto* dst = static_cast<StreamBase*>(destination);
        auto it = std::find(src->pipes.begin(), src->pipes.end(), dst);
        if (it != src->pipes.end()) {
            src->pipes.erase(it);
            if (src->onUnpipe) src->onUnpipe(dst);
        }
    } else {
        // Unpipe all
        for (auto* pipe : src->pipes) {
            if (src->onUnpipe) src->onUnpipe(pipe);
        }
        src->pipes.clear();
    }
}

// readable.setEncoding(encoding)
void nova_stream_Readable_setEncoding(void* stream, const char* encoding) {
    auto* s = static_cast<ReadableStream*>(stream);
    s->defaultEncoding = encoding ? encoding : "utf8";
}

// readable.destroy(error)
void nova_stream_Readable_destroy(void* stream, const char* error) {
    auto* s = static_cast<ReadableStream*>(stream);

    if (s->state & STREAM_STATE_DESTROYED) return;

    s->state |= STREAM_STATE_DESTROYED;
    s->buffer.clear();
    s->bufferSize = 0;

    if (error) {
        s->lastError = error;
        s->state |= STREAM_STATE_ERROR;
        if (s->onError) s->onError(error);
    }

    if (s->destroyImpl) s->destroyImpl();
    if (s->onClose) s->onClose();
}

// readable.readableLength
size_t nova_stream_Readable_readableLength(void* stream) {
    auto* s = static_cast<ReadableStream*>(stream);
    return s->readableLength;
}

// readable.readableEnded
bool nova_stream_Readable_readableEnded(void* stream) {
    auto* s = static_cast<ReadableStream*>(stream);
    return s->readableEnded;
}

// readable.readableFlowing
bool nova_stream_Readable_readableFlowing(void* stream) {
    auto* s = static_cast<ReadableStream*>(stream);
    return s->readableFlowing;
}

// readable.readableHighWaterMark
size_t nova_stream_Readable_readableHighWaterMark(void* stream) {
    auto* s = static_cast<ReadableStream*>(stream);
    return s->highWaterMark;
}

// readable.readableObjectMode
bool nova_stream_Readable_readableObjectMode(void* stream) {
    auto* s = static_cast<ReadableStream*>(stream);
    return s->objectMode;
}

// ============================================================================
// Writable Stream
// ============================================================================

// Create writable stream
void* nova_stream_Writable_new(size_t highWaterMark, bool objectMode, const char* encoding) {
    auto* stream = new WritableStream();
    stream->highWaterMark = highWaterMark > 0 ? highWaterMark :
                           (objectMode ? defaultObjectHighWaterMark : defaultHighWaterMark);
    stream->objectMode = objectMode;
    stream->defaultEncoding = encoding ? encoding : "utf8";

    streams.push_back(stream);
    return stream;
}

// writable.write(chunk, encoding, callback)
bool nova_stream_Writable_write(void* stream, const uint8_t* data, size_t len,
                                 const char* encoding, void (*callback)()) {
    auto* s = static_cast<WritableStream*>(stream);

    if (s->writableEnded) return false;

    s->writableLength += len;

    if (s->writableCorked > 0) {
        // Buffer the write
        StreamChunk chunk;
        chunk.data.assign(data, data + len);
        chunk.encoding = encoding ? encoding : s->defaultEncoding;
        s->buffer.push_back(chunk);
    } else if (s->writeImpl) {
        s->writeImpl(data, len, encoding ? encoding : s->defaultEncoding.c_str(),
                    callback ? callback : []() {});
    }

    bool needDrain = s->writableLength >= s->highWaterMark;
    s->writableNeedDrain = needDrain;

    return !needDrain;
}

// writable.end(chunk, encoding, callback)
void nova_stream_Writable_end(void* stream, const uint8_t* data, size_t len,
                               const char* encoding, void (*callback)()) {
    auto* s = static_cast<WritableStream*>(stream);

    if (s->writableEnded) return;

    if (data && len > 0) {
        nova_stream_Writable_write(stream, data, len, encoding, nullptr);
    }

    s->writableEnded = true;
    s->state |= STREAM_STATE_ENDED;

    if (s->finalImpl) {
        s->finalImpl([s, callback]() {
            s->writableFinished = true;
            s->state |= STREAM_STATE_FINISHED;
            if (s->onFinish) s->onFinish();
            if (callback) callback();
        });
    } else {
        s->writableFinished = true;
        s->state |= STREAM_STATE_FINISHED;
        if (s->onFinish) s->onFinish();
        if (callback) callback();
    }
}

// writable.cork()
void nova_stream_Writable_cork(void* stream) {
    auto* s = static_cast<WritableStream*>(stream);
    s->writableCorked++;
}

// writable.uncork()
void nova_stream_Writable_uncork(void* stream) {
    auto* s = static_cast<WritableStream*>(stream);

    if (s->writableCorked > 0) {
        s->writableCorked--;

        if (s->writableCorked == 0) {
            // Flush buffered writes
            while (!s->buffer.empty()) {
                auto& chunk = s->buffer.front();
                if (s->writeImpl) {
                    s->writeImpl(chunk.data.data(), chunk.data.size(),
                                chunk.encoding.c_str(), []() {});
                }
                s->buffer.pop_front();
            }
        }
    }
}

// writable.setDefaultEncoding(encoding)
void nova_stream_Writable_setDefaultEncoding(void* stream, const char* encoding) {
    auto* s = static_cast<WritableStream*>(stream);
    s->defaultEncoding = encoding ? encoding : "utf8";
}

// writable.destroy(error)
void nova_stream_Writable_destroy(void* stream, const char* error) {
    auto* s = static_cast<WritableStream*>(stream);

    if (s->state & STREAM_STATE_DESTROYED) return;

    s->state |= STREAM_STATE_DESTROYED;
    s->buffer.clear();

    if (error) {
        s->lastError = error;
        s->state |= STREAM_STATE_ERROR;
        if (s->onError) s->onError(error);
    }

    if (s->destroyImpl) s->destroyImpl();
    if (s->onClose) s->onClose();
}

// writable.writableLength
size_t nova_stream_Writable_writableLength(void* stream) {
    auto* s = static_cast<WritableStream*>(stream);
    return s->writableLength;
}

// writable.writableEnded
bool nova_stream_Writable_writableEnded(void* stream) {
    auto* s = static_cast<WritableStream*>(stream);
    return s->writableEnded;
}

// writable.writableFinished
bool nova_stream_Writable_writableFinished(void* stream) {
    auto* s = static_cast<WritableStream*>(stream);
    return s->writableFinished;
}

// writable.writableHighWaterMark
size_t nova_stream_Writable_writableHighWaterMark(void* stream) {
    auto* s = static_cast<WritableStream*>(stream);
    return s->highWaterMark;
}

// writable.writableObjectMode
bool nova_stream_Writable_writableObjectMode(void* stream) {
    auto* s = static_cast<WritableStream*>(stream);
    return s->objectMode;
}

// writable.writableCorked
int nova_stream_Writable_writableCorked(void* stream) {
    auto* s = static_cast<WritableStream*>(stream);
    return s->writableCorked;
}

// writable.writableNeedDrain
bool nova_stream_Writable_writableNeedDrain(void* stream) {
    auto* s = static_cast<WritableStream*>(stream);
    return s->writableNeedDrain;
}

// ============================================================================
// Duplex Stream
// ============================================================================

// Create duplex stream
void* nova_stream_Duplex_new(size_t highWaterMark, bool objectMode,
                              bool allowHalfOpen, const char* encoding) {
    auto* stream = new DuplexStream();
    stream->highWaterMark = highWaterMark > 0 ? highWaterMark :
                           (objectMode ? defaultObjectHighWaterMark : defaultHighWaterMark);
    stream->objectMode = objectMode;
    stream->allowHalfOpen = allowHalfOpen;
    stream->defaultEncoding = encoding ? encoding : "utf8";

    stream->readable.highWaterMark = stream->highWaterMark;
    stream->readable.objectMode = objectMode;
    stream->writable.highWaterMark = stream->highWaterMark;
    stream->writable.objectMode = objectMode;

    streams.push_back(stream);
    return stream;
}

// Get readable side
void* nova_stream_Duplex_readable(void* stream) {
    auto* s = static_cast<DuplexStream*>(stream);
    return &s->readable;
}

// Get writable side
void* nova_stream_Duplex_writable(void* stream) {
    auto* s = static_cast<DuplexStream*>(stream);
    return &s->writable;
}

// duplex.allowHalfOpen
bool nova_stream_Duplex_allowHalfOpen(void* stream) {
    auto* s = static_cast<DuplexStream*>(stream);
    return s->allowHalfOpen;
}

// ============================================================================
// Transform Stream
// ============================================================================

// Create transform stream
void* nova_stream_Transform_new(size_t highWaterMark, bool objectMode, const char* encoding) {
    auto* stream = new TransformStream();
    stream->highWaterMark = highWaterMark > 0 ? highWaterMark :
                           (objectMode ? defaultObjectHighWaterMark : defaultHighWaterMark);
    stream->objectMode = objectMode;
    stream->defaultEncoding = encoding ? encoding : "utf8";

    streams.push_back(stream);
    return stream;
}

// Global callback storage for transform
static std::function<void(const uint8_t*, size_t)> g_transformPushFunc = nullptr;
static std::function<void()> g_flushDoneFunc = nullptr;

static void transformPushCallback(const uint8_t* d, size_t l) {
    if (g_transformPushFunc) g_transformPushFunc(d, l);
}

static void flushDoneCallback() {
    if (g_flushDoneFunc) g_flushDoneFunc();
}

// Set transform function
void nova_stream_Transform_setTransform(void* stream,
    void (*transform)(const uint8_t*, size_t, const char*, void (*)(const uint8_t*, size_t))) {
    auto* s = static_cast<TransformStream*>(stream);

    if (transform) {
        s->transformImpl = [transform](const uint8_t* data, size_t len, const char* enc,
                                        std::function<void(const uint8_t*, size_t)> push) {
            g_transformPushFunc = push;
            transform(data, len, enc, transformPushCallback);
        };
    }
}

// Set flush function
void nova_stream_Transform_setFlush(void* stream, void (*flush)(void (*)())) {
    auto* s = static_cast<TransformStream*>(stream);

    if (flush) {
        s->flushImpl = [flush](std::function<void()> done) {
            g_flushDoneFunc = done;
            flush(flushDoneCallback);
        };
    }
}

// ============================================================================
// PassThrough Stream
// ============================================================================

// Create passthrough stream
void* nova_stream_PassThrough_new(size_t highWaterMark, bool objectMode) {
    auto* stream = new PassThroughStream();
    stream->highWaterMark = highWaterMark > 0 ? highWaterMark :
                           (objectMode ? defaultObjectHighWaterMark : defaultHighWaterMark);
    stream->objectMode = objectMode;

    streams.push_back(stream);
    return stream;
}

// ============================================================================
// Event Handlers
// ============================================================================

void nova_stream_onClose(void* stream, void (*callback)()) {
    auto* s = static_cast<StreamBase*>(stream);
    s->onClose = callback;
}

void nova_stream_onError(void* stream, void (*callback)(const char*)) {
    auto* s = static_cast<StreamBase*>(stream);
    s->onError = callback;
}

void nova_stream_onDrain(void* stream, void (*callback)()) {
    auto* s = static_cast<StreamBase*>(stream);
    s->onDrain = callback;
}

void nova_stream_onFinish(void* stream, void (*callback)()) {
    auto* s = static_cast<StreamBase*>(stream);
    s->onFinish = callback;
}

void nova_stream_onEnd(void* stream, void (*callback)()) {
    auto* s = static_cast<StreamBase*>(stream);
    s->onEnd = callback;
}

void nova_stream_onData(void* stream, void (*callback)(const uint8_t*, size_t)) {
    auto* s = static_cast<StreamBase*>(stream);
    s->onData = callback;
}

void nova_stream_onReadable(void* stream, void (*callback)()) {
    auto* s = static_cast<StreamBase*>(stream);
    s->onReadable = callback;
}

// ============================================================================
// Utility Functions
// ============================================================================

// stream.pipeline(streams..., callback)
void nova_stream_pipeline(void** streamArray, int count, void (*callback)(const char*)) {
    if (count < 2) {
        if (callback) callback("Pipeline requires at least 2 streams");
        return;
    }

    // Connect streams in sequence
    for (int i = 0; i < count - 1; i++) {
        nova_stream_Readable_pipe(streamArray[i], streamArray[i + 1]);
    }

    // Set up completion callback on last stream
    auto* last = static_cast<StreamBase*>(streamArray[count - 1]);
    last->onFinish = [callback]() {
        if (callback) callback(nullptr);
    };
    last->onError = [callback](const char* err) {
        if (callback) callback(err);
    };
}

// stream.finished(stream, callback)
void nova_stream_finished(void* stream, void (*callback)(const char*)) {
    auto* s = static_cast<StreamBase*>(stream);

    if (s->state & STREAM_STATE_DESTROYED) {
        if (callback) callback(s->lastError.empty() ? nullptr : s->lastError.c_str());
        return;
    }

    s->onFinish = [callback]() {
        if (callback) callback(nullptr);
    };
    s->onEnd = [callback]() {
        if (callback) callback(nullptr);
    };
    s->onError = [callback](const char* err) {
        if (callback) callback(err);
    };
    s->onClose = [callback, s]() {
        if (!(s->state & (STREAM_STATE_FINISHED | STREAM_STATE_ENDED))) {
            if (callback) callback("Stream closed prematurely");
        }
    };
}

// stream.Readable.from(iterable)
void* nova_stream_Readable_from(const uint8_t** chunks, size_t* lengths, int count) {
    auto* stream = static_cast<ReadableStream*>(nova_stream_Readable_new(0, false, "utf8"));

    for (int i = 0; i < count; i++) {
        nova_stream_Readable_push(stream, chunks[i], lengths[i]);
    }
    nova_stream_Readable_push(stream, nullptr, 0);  // End stream

    return stream;
}

// stream.Readable.fromString(string)
void* nova_stream_Readable_fromString(const char* str) {
    auto* stream = static_cast<ReadableStream*>(nova_stream_Readable_new(0, false, "utf8"));

    if (str) {
        nova_stream_Readable_push(stream, reinterpret_cast<const uint8_t*>(str), strlen(str));
    }
    nova_stream_Readable_push(stream, nullptr, 0);

    return stream;
}

// stream.addAbortSignal(signal, stream)
void nova_stream_addAbortSignal(void* stream, bool* aborted) {
    auto* s = static_cast<StreamBase*>(stream);

    // Check periodically if aborted
    if (aborted && *aborted) {
        s->state |= STREAM_STATE_DESTROYED;
        if (s->onError) s->onError("AbortError: The operation was aborted");
    }
}

// stream.isReadable(stream)
bool nova_stream_isReadable(void* stream) {
    auto* s = static_cast<StreamBase*>(stream);
    return (s->state & STREAM_STATE_READABLE) &&
           !(s->state & (STREAM_STATE_DESTROYED | STREAM_STATE_ENDED));
}

// stream.isWritable(stream)
bool nova_stream_isWritable(void* stream) {
    auto* s = static_cast<StreamBase*>(stream);
    return (s->state & STREAM_STATE_WRITABLE) &&
           !(s->state & (STREAM_STATE_DESTROYED | STREAM_STATE_FINISHED));
}

// stream.isDisturbed(stream)
bool nova_stream_isDisturbed(void* stream) {
    auto* s = static_cast<ReadableStream*>(stream);
    return s->readableFlowing || s->readableLength < s->highWaterMark;
}

// stream.isErrored(stream)
bool nova_stream_isErrored(void* stream) {
    auto* s = static_cast<StreamBase*>(stream);
    return (s->state & STREAM_STATE_ERROR) != 0;
}

// ============================================================================
// Stream Consumers (stream/consumers)
// ============================================================================

// Internal buffer for consumers
static std::vector<uint8_t> consumerBuffer;
static std::string consumerTextBuffer;

// consumers.arrayBuffer(stream)
void nova_stream_consumers_arrayBuffer(void* stream, void (*callback)(const uint8_t*, size_t)) {
    auto* s = static_cast<ReadableStream*>(stream);
    consumerBuffer.clear();

    while (!s->readableEnded && !s->buffer.empty()) {
        auto& chunk = s->buffer.front();
        consumerBuffer.insert(consumerBuffer.end(), chunk.data.begin(), chunk.data.end());
        s->bufferSize -= chunk.data.size();
        s->buffer.pop_front();
    }

    if (callback) callback(consumerBuffer.data(), consumerBuffer.size());
}

// consumers.text(stream)
void nova_stream_consumers_text(void* stream, void (*callback)(const char*)) {
    auto* s = static_cast<ReadableStream*>(stream);
    consumerTextBuffer.clear();

    while (!s->readableEnded && !s->buffer.empty()) {
        auto& chunk = s->buffer.front();
        consumerTextBuffer.append(reinterpret_cast<const char*>(chunk.data.data()), chunk.data.size());
        s->bufferSize -= chunk.data.size();
        s->buffer.pop_front();
    }

    if (callback) callback(consumerTextBuffer.c_str());
}

// consumers.json(stream)
void nova_stream_consumers_json(void* stream, void (*callback)(const char*)) {
    // Returns raw JSON string - parsing done in JS layer
    nova_stream_consumers_text(stream, callback);
}

// consumers.blob(stream)
void nova_stream_consumers_blob(void* stream, void (*callback)(const uint8_t*, size_t, const char*)) {
    auto* s = static_cast<ReadableStream*>(stream);
    consumerBuffer.clear();

    while (!s->readableEnded && !s->buffer.empty()) {
        auto& chunk = s->buffer.front();
        consumerBuffer.insert(consumerBuffer.end(), chunk.data.begin(), chunk.data.end());
        s->bufferSize -= chunk.data.size();
        s->buffer.pop_front();
    }

    if (callback) callback(consumerBuffer.data(), consumerBuffer.size(), "application/octet-stream");
}

// ============================================================================
// Promises API (stream/promises)
// ============================================================================

// Storage for promise callbacks
static void (*g_pipelineResolve)() = nullptr;
static void (*g_pipelineReject)(const char*) = nullptr;
static void (*g_finishedResolve)() = nullptr;
static void (*g_finishedReject)(const char*) = nullptr;

static void pipelineCallback(const char* err) {
    if (err) {
        if (g_pipelineReject) g_pipelineReject(err);
    } else {
        if (g_pipelineResolve) g_pipelineResolve();
    }
}

static void finishedCallback(const char* err) {
    if (err) {
        if (g_finishedReject) g_finishedReject(err);
    } else {
        if (g_finishedResolve) g_finishedResolve();
    }
}

// promises.pipeline(streams...)
void nova_stream_promises_pipeline(void** streamArray, int count,
                                    void (*resolve)(), void (*reject)(const char*)) {
    g_pipelineResolve = resolve;
    g_pipelineReject = reject;
    nova_stream_pipeline(streamArray, count, pipelineCallback);
}

// promises.pipeline with options (signal support)
void nova_stream_promises_pipelineWithOptions(void** streamArray, int count,
                                               bool* aborted,
                                               void (*resolve)(), void (*reject)(const char*)) {
    // Check abort signal
    if (aborted && *aborted) {
        if (reject) reject("AbortError: The operation was aborted");
        return;
    }

    g_pipelineResolve = resolve;
    g_pipelineReject = reject;
    nova_stream_pipeline(streamArray, count, pipelineCallback);
}

// promises.finished(stream)
void nova_stream_promises_finished(void* stream,
                                    void (*resolve)(), void (*reject)(const char*)) {
    g_finishedResolve = resolve;
    g_finishedReject = reject;
    nova_stream_finished(stream, finishedCallback);
}

// promises.finished with options
void nova_stream_promises_finishedWithOptions(void* stream,
                                               bool cleanup, bool readable, bool writable,
                                               bool* aborted,
                                               void (*resolve)(), void (*reject)(const char*)) {
    // Check abort signal
    if (aborted && *aborted) {
        if (reject) reject("AbortError: The operation was aborted");
        return;
    }

    auto* s = static_cast<StreamBase*>(stream);

    // Apply options
    if (cleanup) {
        // Would set up cleanup on error
    }

    // Check specific states based on options
    if (readable && !(s->state & STREAM_STATE_READABLE)) {
        if (reject) reject("Stream is not readable");
        return;
    }

    if (writable && !(s->state & STREAM_STATE_WRITABLE)) {
        if (reject) reject("Stream is not writable");
        return;
    }

    g_finishedResolve = resolve;
    g_finishedReject = reject;
    nova_stream_finished(stream, finishedCallback);
}

// Async iterator for readable streams (for await...of)
struct StreamAsyncIterator {
    ReadableStream* stream;
    bool done;
    std::vector<uint8_t> currentChunk;
};

void* nova_stream_promises_createAsyncIterator(void* stream) {
    auto* iter = new StreamAsyncIterator();
    iter->stream = static_cast<ReadableStream*>(stream);
    iter->done = false;
    return iter;
}

void nova_stream_promises_asyncIteratorNext(void* iterator,
                                             void (*resolve)(const uint8_t*, size_t, bool),
                                             void (*reject)(const char*)) {
    auto* iter = static_cast<StreamAsyncIterator*>(iterator);

    if (iter->done || iter->stream->readableEnded) {
        if (resolve) resolve(nullptr, 0, true);  // done: true
        return;
    }

    if (!iter->stream->buffer.empty()) {
        auto& chunk = iter->stream->buffer.front();
        iter->currentChunk = chunk.data;
        iter->stream->bufferSize -= chunk.data.size();
        iter->stream->buffer.pop_front();

        if (resolve) resolve(iter->currentChunk.data(), iter->currentChunk.size(), false);
    } else if (iter->stream->readableEnded) {
        iter->done = true;
        if (resolve) resolve(nullptr, 0, true);
    } else {
        // No data available yet, would need to wait
        // In sync implementation, just return done
        iter->done = true;
        if (resolve) resolve(nullptr, 0, true);
    }

    (void)reject;
}

void nova_stream_promises_asyncIteratorReturn(void* iterator) {
    auto* iter = static_cast<StreamAsyncIterator*>(iterator);
    iter->done = true;
}

void nova_stream_promises_asyncIteratorFree(void* iterator) {
    delete static_cast<StreamAsyncIterator*>(iterator);
}

// Read all data from readable stream as Promise
void nova_stream_promises_text(void* stream,
                                void (*resolve)(const char*), void (*reject)(const char*)) {
    auto* s = static_cast<ReadableStream*>(stream);

    if (s->state & STREAM_STATE_ERROR) {
        if (reject) reject(s->lastError.c_str());
        return;
    }

    consumerTextBuffer.clear();

    while (!s->buffer.empty()) {
        auto& chunk = s->buffer.front();
        consumerTextBuffer.append(reinterpret_cast<const char*>(chunk.data.data()), chunk.data.size());
        s->bufferSize -= chunk.data.size();
        s->buffer.pop_front();
    }

    if (resolve) resolve(consumerTextBuffer.c_str());
}

// Read all data as ArrayBuffer
void nova_stream_promises_arrayBuffer(void* stream,
                                       void (*resolve)(const uint8_t*, size_t),
                                       void (*reject)(const char*)) {
    auto* s = static_cast<ReadableStream*>(stream);

    if (s->state & STREAM_STATE_ERROR) {
        if (reject) reject(s->lastError.c_str());
        return;
    }

    consumerBuffer.clear();

    while (!s->buffer.empty()) {
        auto& chunk = s->buffer.front();
        consumerBuffer.insert(consumerBuffer.end(), chunk.data.begin(), chunk.data.end());
        s->bufferSize -= chunk.data.size();
        s->buffer.pop_front();
    }

    if (resolve) resolve(consumerBuffer.data(), consumerBuffer.size());
}

// Read all data as JSON
void nova_stream_promises_json(void* stream,
                                void (*resolve)(const char*), void (*reject)(const char*)) {
    // Same as text - parsing happens in JS layer
    nova_stream_promises_text(stream, resolve, reject);
}

// Read all data as Blob
void nova_stream_promises_blob(void* stream,
                                void (*resolve)(const uint8_t*, size_t, const char*),
                                void (*reject)(const char*)) {
    auto* s = static_cast<ReadableStream*>(stream);

    if (s->state & STREAM_STATE_ERROR) {
        if (reject) reject(s->lastError.c_str());
        return;
    }

    consumerBuffer.clear();

    while (!s->buffer.empty()) {
        auto& chunk = s->buffer.front();
        consumerBuffer.insert(consumerBuffer.end(), chunk.data.begin(), chunk.data.end());
        s->bufferSize -= chunk.data.size();
        s->buffer.pop_front();
    }

    if (resolve) resolve(consumerBuffer.data(), consumerBuffer.size(), "application/octet-stream");
}

// ============================================================================
// Cleanup
// ============================================================================

void nova_stream_free(void* stream) {
    auto* s = static_cast<StreamBase*>(stream);

    auto it = std::find(streams.begin(), streams.end(), s);
    if (it != streams.end()) {
        streams.erase(it);
    }

    delete s;
}

void nova_stream_cleanup() {
    for (auto* s : streams) {
        delete s;
    }
    streams.clear();
}

} // extern "C"
