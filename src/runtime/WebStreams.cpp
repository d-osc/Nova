/**
 * Web Streams API Implementation (nova:stream/web)
 *
 * Provides WHATWG Streams Standard compatible streaming API.
 * https://streams.spec.whatwg.org/
 */

#include <string>
#include <vector>
#include <deque>
#include <functional>
#include <cstring>
#include <cstdint>
#include <atomic>
#include <algorithm>

namespace nova {
namespace runtime {
namespace webstreams {

// ============================================================================
// Stream States
// ============================================================================

enum class ReadableStreamState {
    Readable,
    Closed,
    Errored
};

enum class WritableStreamState {
    Writable,
    Closed,
    Errored,
    Erroring
};

// ============================================================================
// Queuing Strategies
// ============================================================================

struct QueuingStrategy {
    size_t highWaterMark;
    bool useByteLength;

    QueuingStrategy() : highWaterMark(1), useByteLength(false) {}
};

// ============================================================================
// Stream Chunk
// ============================================================================

struct StreamChunk {
    std::vector<uint8_t> data;
    size_t byteLength;

    StreamChunk() : byteLength(0) {}
    StreamChunk(const uint8_t* d, size_t len) : data(d, d + len), byteLength(len) {}
};

// ============================================================================
// ReadableStream
// ============================================================================

struct WebReadableStream;
struct WebReadableStreamReader;

struct WebReadableStream {
    int64_t id;
    ReadableStreamState state;
    std::deque<StreamChunk> queue;
    size_t queueTotalSize;
    QueuingStrategy strategy;
    bool locked;
    bool disturbed;
    WebReadableStreamReader* reader;
    std::string storedError;

    // Underlying source callbacks
    std::function<void(void*)> pullCallback;
    std::function<void(const char*)> cancelCallback;
    void* controller;

    WebReadableStream() : id(0), state(ReadableStreamState::Readable),
                          queueTotalSize(0), locked(false), disturbed(false),
                          reader(nullptr), controller(nullptr) {}
};

// ============================================================================
// ReadableStreamDefaultReader
// ============================================================================

struct WebReadableStreamReader {
    WebReadableStream* stream;
    bool closed;
    std::function<void()> closedResolve;
    std::function<void(const char*)> closedReject;

    // Pending read requests
    struct ReadRequest {
        std::function<void(const uint8_t*, size_t, bool)> resolve;
        std::function<void(const char*)> reject;
    };
    std::deque<ReadRequest> readRequests;

    WebReadableStreamReader() : stream(nullptr), closed(false) {}
};

// ============================================================================
// ReadableStreamDefaultController
// ============================================================================

struct WebReadableStreamController {
    WebReadableStream* stream;
    bool closeRequested;
    bool pullAgain;
    bool pulling;
    QueuingStrategy strategy;

    WebReadableStreamController() : stream(nullptr), closeRequested(false),
                                    pullAgain(false), pulling(false) {}
};

// ============================================================================
// WritableStream
// ============================================================================

struct WebWritableStream;
struct WebWritableStreamWriter;

struct WebWritableStream {
    int64_t id;
    WritableStreamState state;
    std::deque<StreamChunk> writeQueue;
    size_t queueTotalSize;
    QueuingStrategy strategy;
    bool locked;
    WebWritableStreamWriter* writer;
    std::string storedError;
    bool backpressure;

    // Underlying sink callbacks
    std::function<void(const uint8_t*, size_t, void*)> writeCallback;
    std::function<void()> closeCallback;
    std::function<void(const char*)> abortCallback;
    void* controller;

    WebWritableStream() : id(0), state(WritableStreamState::Writable),
                          queueTotalSize(0), locked(false), writer(nullptr),
                          backpressure(false), controller(nullptr) {}
};

// ============================================================================
// WritableStreamDefaultWriter
// ============================================================================

struct WebWritableStreamWriter {
    WebWritableStream* stream;
    std::function<void()> readyResolve;
    std::function<void(const char*)> readyReject;
    std::function<void()> closedResolve;
    std::function<void(const char*)> closedReject;

    WebWritableStreamWriter() : stream(nullptr) {}
};

// ============================================================================
// WritableStreamDefaultController
// ============================================================================

struct WebWritableStreamController {
    WebWritableStream* stream;
    bool started;
    QueuingStrategy strategy;

    WebWritableStreamController() : stream(nullptr), started(false) {}
};

// ============================================================================
// TransformStream
// ============================================================================

struct WebTransformStream {
    int64_t id;
    WebReadableStream* readable;
    WebWritableStream* writable;
    bool backpressure;

    std::function<void(const uint8_t*, size_t, void*)> transformCallback;
    std::function<void(void*)> flushCallback;
    void* controller;

    WebTransformStream() : id(0), readable(nullptr), writable(nullptr),
                           backpressure(false), controller(nullptr) {}
};

// ============================================================================
// TransformStreamDefaultController
// ============================================================================

struct WebTransformStreamController {
    WebTransformStream* stream;
    std::function<void(const uint8_t*, size_t)> enqueue;
    std::function<void(const char*)> error;
    std::function<void()> terminate;

    WebTransformStreamController() : stream(nullptr) {}
};

// ============================================================================
// Global State
// ============================================================================

static std::atomic<int64_t> nextStreamId{1};

extern "C" {

// ============================================================================
// ReadableStream API
// ============================================================================

void* nova_webstream_ReadableStream_new(size_t highWaterMark, bool useByteLength) {
    auto* stream = new WebReadableStream();
    stream->id = nextStreamId++;
    stream->strategy.highWaterMark = highWaterMark > 0 ? highWaterMark : 1;
    stream->strategy.useByteLength = useByteLength;
    return stream;
}

void* nova_webstream_ReadableStream_newWithSource(
    void (*start)(void*),
    void (*pull)(void*),
    void (*cancel)(const char*),
    size_t highWaterMark) {

    auto* stream = new WebReadableStream();
    stream->id = nextStreamId++;
    stream->strategy.highWaterMark = highWaterMark > 0 ? highWaterMark : 1;

    auto* controller = new WebReadableStreamController();
    controller->stream = stream;
    controller->strategy = stream->strategy;
    stream->controller = controller;

    if (pull) stream->pullCallback = [pull](void* ctrl) { pull(ctrl); };
    if (cancel) stream->cancelCallback = [cancel](const char* reason) { cancel(reason); };

    if (start) start(controller);

    return stream;
}

int nova_webstream_ReadableStream_locked(void* streamPtr) {
    auto* stream = static_cast<WebReadableStream*>(streamPtr);
    return stream->locked ? 1 : 0;
}

void nova_webstream_ReadableStream_cancel(void* streamPtr, const char* reason,
                                           void (*resolve)(), void (*reject)(const char*)) {
    auto* stream = static_cast<WebReadableStream*>(streamPtr);

    if (stream->locked) {
        if (reject) reject("Cannot cancel a locked stream");
        return;
    }

    stream->disturbed = true;

    if (stream->state == ReadableStreamState::Closed) {
        if (resolve) resolve();
        return;
    }

    if (stream->state == ReadableStreamState::Errored) {
        if (reject) reject(stream->storedError.c_str());
        return;
    }

    stream->state = ReadableStreamState::Closed;
    stream->queue.clear();
    stream->queueTotalSize = 0;

    if (stream->cancelCallback) {
        stream->cancelCallback(reason);
    }

    if (resolve) resolve();
}

void* nova_webstream_ReadableStream_getReader(void* streamPtr) {
    auto* stream = static_cast<WebReadableStream*>(streamPtr);

    if (stream->locked) return nullptr;

    auto* reader = new WebReadableStreamReader();
    reader->stream = stream;
    reader->closed = stream->state != ReadableStreamState::Readable;

    stream->locked = true;
    stream->reader = reader;

    return reader;
}

// ReadableStream.tee() - Split into two streams
void nova_webstream_ReadableStream_tee(void* streamPtr, void** branch1, void** branch2) {
    auto* stream = static_cast<WebReadableStream*>(streamPtr);

    if (stream->locked) {
        *branch1 = nullptr;
        *branch2 = nullptr;
        return;
    }

    auto* s1 = new WebReadableStream();
    s1->id = nextStreamId++;
    s1->strategy = stream->strategy;

    auto* s2 = new WebReadableStream();
    s2->id = nextStreamId++;
    s2->strategy = stream->strategy;

    // Copy current queue to both branches
    for (const auto& chunk : stream->queue) {
        s1->queue.push_back(chunk);
        s2->queue.push_back(chunk);
    }
    s1->queueTotalSize = stream->queueTotalSize;
    s2->queueTotalSize = stream->queueTotalSize;

    *branch1 = s1;
    *branch2 = s2;
}

// ReadableStream.pipeThrough(transformStream)
void* nova_webstream_ReadableStream_pipeThrough(void* streamPtr, void* transformPtr) {
    auto* stream = static_cast<WebReadableStream*>(streamPtr);
    auto* transform = static_cast<WebTransformStream*>(transformPtr);

    if (stream->locked || transform->writable->locked) {
        return nullptr;
    }

    stream->disturbed = true;

    // Return the readable side of the transform
    return transform->readable;
}

// ReadableStream.pipeTo(writableStream)
void nova_webstream_ReadableStream_pipeTo(void* streamPtr, void* writablePtr,
                                           int preventClose, int preventAbort, int preventCancel,
                                           void (*resolve)(), void (*reject)(const char*)) {
    auto* readable = static_cast<WebReadableStream*>(streamPtr);
    auto* writable = static_cast<WebWritableStream*>(writablePtr);

    if (readable->locked || writable->locked) {
        if (reject) reject("Cannot pipe locked streams");
        return;
    }

    readable->disturbed = true;
    readable->locked = true;
    writable->locked = true;

    // Transfer data from readable to writable
    while (!readable->queue.empty() && writable->state == WritableStreamState::Writable) {
        auto& chunk = readable->queue.front();

        if (writable->writeCallback) {
            writable->writeCallback(chunk.data.data(), chunk.byteLength, writable->controller);
        }

        readable->queueTotalSize -= chunk.byteLength;
        readable->queue.pop_front();
    }

    if (!preventClose && readable->state == ReadableStreamState::Closed) {
        if (writable->closeCallback) writable->closeCallback();
        writable->state = WritableStreamState::Closed;
    }

    readable->locked = false;
    writable->locked = false;

    if (resolve) resolve();
    (void)preventAbort;
    (void)preventCancel;
}

// ============================================================================
// ReadableStreamDefaultReader API
// ============================================================================

void nova_webstream_Reader_read(void* readerPtr,
                                 void (*resolve)(const uint8_t*, size_t, bool),
                                 void (*reject)(const char*)) {
    auto* reader = static_cast<WebReadableStreamReader*>(readerPtr);

    if (!reader->stream) {
        if (reject) reject("Reader has no associated stream");
        return;
    }

    auto* stream = reader->stream;
    stream->disturbed = true;

    if (stream->state == ReadableStreamState::Closed) {
        if (resolve) resolve(nullptr, 0, true);  // done: true
        return;
    }

    if (stream->state == ReadableStreamState::Errored) {
        if (reject) reject(stream->storedError.c_str());
        return;
    }

    if (!stream->queue.empty()) {
        auto chunk = stream->queue.front();
        stream->queue.pop_front();
        stream->queueTotalSize -= chunk.byteLength;

        if (resolve) resolve(chunk.data.data(), chunk.byteLength, false);
    } else {
        // Queue a read request for later
        WebReadableStreamReader::ReadRequest req;
        req.resolve = resolve;
        req.reject = reject;
        reader->readRequests.push_back(req);

        // Try to pull more data
        if (stream->pullCallback && stream->controller) {
            stream->pullCallback(stream->controller);
        }
    }
}

void nova_webstream_Reader_releaseLock(void* readerPtr) {
    auto* reader = static_cast<WebReadableStreamReader*>(readerPtr);

    if (reader->stream) {
        reader->stream->locked = false;
        reader->stream->reader = nullptr;
        reader->stream = nullptr;
    }
}

void nova_webstream_Reader_cancel(void* readerPtr, const char* reason,
                                   void (*resolve)(), void (*reject)(const char*)) {
    auto* reader = static_cast<WebReadableStreamReader*>(readerPtr);

    if (!reader->stream) {
        if (reject) reject("Reader has no associated stream");
        return;
    }

    nova_webstream_ReadableStream_cancel(reader->stream, reason, resolve, reject);
}

int nova_webstream_Reader_closed(void* readerPtr) {
    auto* reader = static_cast<WebReadableStreamReader*>(readerPtr);
    return reader->closed ? 1 : 0;
}

void nova_webstream_Reader_free(void* readerPtr) {
    auto* reader = static_cast<WebReadableStreamReader*>(readerPtr);
    if (reader->stream) {
        reader->stream->locked = false;
        reader->stream->reader = nullptr;
    }
    delete reader;
}

// ============================================================================
// ReadableStreamDefaultController API
// ============================================================================

void nova_webstream_ReadableController_enqueue(void* controllerPtr,
                                                const uint8_t* data, size_t len) {
    auto* controller = static_cast<WebReadableStreamController*>(controllerPtr);
    auto* stream = controller->stream;

    if (!stream || stream->state != ReadableStreamState::Readable) return;

    StreamChunk chunk(data, len);

    // If there are pending read requests, fulfill them
    if (stream->reader && !stream->reader->readRequests.empty()) {
        auto req = stream->reader->readRequests.front();
        stream->reader->readRequests.pop_front();
        if (req.resolve) req.resolve(data, len, false);
    } else {
        stream->queue.push_back(chunk);
        stream->queueTotalSize += len;
    }
}

void nova_webstream_ReadableController_close(void* controllerPtr) {
    auto* controller = static_cast<WebReadableStreamController*>(controllerPtr);
    auto* stream = controller->stream;

    if (!stream || stream->state != ReadableStreamState::Readable) return;

    controller->closeRequested = true;

    if (stream->queue.empty()) {
        stream->state = ReadableStreamState::Closed;

        // Fulfill pending read requests with done: true
        if (stream->reader) {
            while (!stream->reader->readRequests.empty()) {
                auto req = stream->reader->readRequests.front();
                stream->reader->readRequests.pop_front();
                if (req.resolve) req.resolve(nullptr, 0, true);
            }
            stream->reader->closed = true;
            if (stream->reader->closedResolve) stream->reader->closedResolve();
        }
    }
}

void nova_webstream_ReadableController_error(void* controllerPtr, const char* error) {
    auto* controller = static_cast<WebReadableStreamController*>(controllerPtr);
    auto* stream = controller->stream;

    if (!stream || stream->state != ReadableStreamState::Readable) return;

    stream->state = ReadableStreamState::Errored;
    stream->storedError = error ? error : "Unknown error";
    stream->queue.clear();
    stream->queueTotalSize = 0;

    // Reject pending read requests
    if (stream->reader) {
        while (!stream->reader->readRequests.empty()) {
            auto req = stream->reader->readRequests.front();
            stream->reader->readRequests.pop_front();
            if (req.reject) req.reject(stream->storedError.c_str());
        }
        if (stream->reader->closedReject) {
            stream->reader->closedReject(stream->storedError.c_str());
        }
    }
}

size_t nova_webstream_ReadableController_desiredSize(void* controllerPtr) {
    auto* controller = static_cast<WebReadableStreamController*>(controllerPtr);
    auto* stream = controller->stream;

    if (!stream) return 0;

    size_t queueSize = controller->strategy.useByteLength ?
                       stream->queueTotalSize : stream->queue.size();

    if (queueSize >= controller->strategy.highWaterMark) return 0;
    return controller->strategy.highWaterMark - queueSize;
}

// ============================================================================
// WritableStream API
// ============================================================================

void* nova_webstream_WritableStream_new(size_t highWaterMark) {
    auto* stream = new WebWritableStream();
    stream->id = nextStreamId++;
    stream->strategy.highWaterMark = highWaterMark > 0 ? highWaterMark : 1;
    return stream;
}

void* nova_webstream_WritableStream_newWithSink(
    void (*start)(void*),
    void (*write)(const uint8_t*, size_t, void*),
    void (*close)(),
    void (*abort)(const char*),
    size_t highWaterMark) {

    auto* stream = new WebWritableStream();
    stream->id = nextStreamId++;
    stream->strategy.highWaterMark = highWaterMark > 0 ? highWaterMark : 1;

    auto* controller = new WebWritableStreamController();
    controller->stream = stream;
    controller->strategy = stream->strategy;
    stream->controller = controller;

    if (write) stream->writeCallback = [write](const uint8_t* d, size_t l, void* c) { write(d, l, c); };
    if (close) stream->closeCallback = close;
    if (abort) stream->abortCallback = [abort](const char* r) { abort(r); };

    if (start) start(controller);
    controller->started = true;

    return stream;
}

int nova_webstream_WritableStream_locked(void* streamPtr) {
    auto* stream = static_cast<WebWritableStream*>(streamPtr);
    return stream->locked ? 1 : 0;
}

void nova_webstream_WritableStream_abort(void* streamPtr, const char* reason,
                                          void (*resolve)(), void (*reject)(const char*)) {
    auto* stream = static_cast<WebWritableStream*>(streamPtr);

    if (stream->locked) {
        if (reject) reject("Cannot abort a locked stream");
        return;
    }

    if (stream->state == WritableStreamState::Closed ||
        stream->state == WritableStreamState::Errored) {
        if (resolve) resolve();
        return;
    }

    stream->state = WritableStreamState::Errored;
    stream->storedError = reason ? reason : "Aborted";
    stream->writeQueue.clear();
    stream->queueTotalSize = 0;

    if (stream->abortCallback) {
        stream->abortCallback(reason);
    }

    if (resolve) resolve();
}

void nova_webstream_WritableStream_close(void* streamPtr,
                                          void (*resolve)(), void (*reject)(const char*)) {
    auto* stream = static_cast<WebWritableStream*>(streamPtr);

    if (stream->locked) {
        if (reject) reject("Cannot close a locked stream");
        return;
    }

    if (stream->state != WritableStreamState::Writable) {
        if (reject) reject("Stream is not writable");
        return;
    }

    stream->state = WritableStreamState::Closed;

    if (stream->closeCallback) {
        stream->closeCallback();
    }

    if (resolve) resolve();
}

void* nova_webstream_WritableStream_getWriter(void* streamPtr) {
    auto* stream = static_cast<WebWritableStream*>(streamPtr);

    if (stream->locked) return nullptr;

    auto* writer = new WebWritableStreamWriter();
    writer->stream = stream;

    stream->locked = true;
    stream->writer = writer;

    return writer;
}

// ============================================================================
// WritableStreamDefaultWriter API
// ============================================================================

void nova_webstream_Writer_write(void* writerPtr, const uint8_t* data, size_t len,
                                  void (*resolve)(), void (*reject)(const char*)) {
    auto* writer = static_cast<WebWritableStreamWriter*>(writerPtr);

    if (!writer->stream) {
        if (reject) reject("Writer has no associated stream");
        return;
    }

    auto* stream = writer->stream;

    if (stream->state != WritableStreamState::Writable) {
        if (reject) reject("Stream is not writable");
        return;
    }

    if (stream->writeCallback) {
        stream->writeCallback(data, len, stream->controller);
    }

    stream->queueTotalSize += len;
    stream->backpressure = stream->queueTotalSize >= stream->strategy.highWaterMark;

    if (resolve) resolve();
}

void nova_webstream_Writer_close(void* writerPtr,
                                  void (*resolve)(), void (*reject)(const char*)) {
    auto* writer = static_cast<WebWritableStreamWriter*>(writerPtr);

    if (!writer->stream) {
        if (reject) reject("Writer has no associated stream");
        return;
    }

    nova_webstream_WritableStream_close(writer->stream, resolve, reject);
}

void nova_webstream_Writer_abort(void* writerPtr, const char* reason,
                                  void (*resolve)(), void (*reject)(const char*)) {
    auto* writer = static_cast<WebWritableStreamWriter*>(writerPtr);

    if (!writer->stream) {
        if (reject) reject("Writer has no associated stream");
        return;
    }

    nova_webstream_WritableStream_abort(writer->stream, reason, resolve, reject);
}

void nova_webstream_Writer_releaseLock(void* writerPtr) {
    auto* writer = static_cast<WebWritableStreamWriter*>(writerPtr);

    if (writer->stream) {
        writer->stream->locked = false;
        writer->stream->writer = nullptr;
        writer->stream = nullptr;
    }
}

size_t nova_webstream_Writer_desiredSize(void* writerPtr) {
    auto* writer = static_cast<WebWritableStreamWriter*>(writerPtr);

    if (!writer->stream) return 0;

    auto* stream = writer->stream;
    if (stream->queueTotalSize >= stream->strategy.highWaterMark) return 0;
    return stream->strategy.highWaterMark - stream->queueTotalSize;
}

int nova_webstream_Writer_ready(void* writerPtr) {
    auto* writer = static_cast<WebWritableStreamWriter*>(writerPtr);
    if (!writer->stream) return 0;
    return !writer->stream->backpressure ? 1 : 0;
}

void nova_webstream_Writer_free(void* writerPtr) {
    auto* writer = static_cast<WebWritableStreamWriter*>(writerPtr);
    if (writer->stream) {
        writer->stream->locked = false;
        writer->stream->writer = nullptr;
    }
    delete writer;
}

// ============================================================================
// TransformStream API
// ============================================================================

void* nova_webstream_TransformStream_new(
    void (*transform)(const uint8_t*, size_t, void*),
    void (*flush)(void*),
    size_t writableHighWaterMark,
    size_t readableHighWaterMark) {

    auto* stream = new WebTransformStream();
    stream->id = nextStreamId++;

    stream->readable = static_cast<WebReadableStream*>(
        nova_webstream_ReadableStream_new(readableHighWaterMark, false));
    stream->writable = static_cast<WebWritableStream*>(
        nova_webstream_WritableStream_new(writableHighWaterMark));

    auto* controller = new WebTransformStreamController();
    controller->stream = stream;
    stream->controller = controller;

    // Set up enqueue function
    controller->enqueue = [stream](const uint8_t* data, size_t len) {
        if (stream->readable->controller) {
            nova_webstream_ReadableController_enqueue(stream->readable->controller, data, len);
        }
    };

    if (transform) {
        stream->transformCallback = [transform](const uint8_t* d, size_t l, void* c) {
            transform(d, l, c);
        };
    }

    if (flush) {
        stream->flushCallback = [flush](void* c) { flush(c); };
    }

    // Connect writable to transform
    stream->writable->writeCallback = [stream](const uint8_t* data, size_t len, void*) {
        if (stream->transformCallback) {
            stream->transformCallback(data, len, stream->controller);
        } else {
            // Default: pass through
            if (stream->readable->controller) {
                nova_webstream_ReadableController_enqueue(stream->readable->controller, data, len);
            }
        }
    };

    stream->writable->closeCallback = [stream]() {
        if (stream->flushCallback) {
            stream->flushCallback(stream->controller);
        }
        if (stream->readable->controller) {
            nova_webstream_ReadableController_close(stream->readable->controller);
        }
    };

    return stream;
}

void* nova_webstream_TransformStream_readable(void* streamPtr) {
    auto* stream = static_cast<WebTransformStream*>(streamPtr);
    return stream->readable;
}

void* nova_webstream_TransformStream_writable(void* streamPtr) {
    auto* stream = static_cast<WebTransformStream*>(streamPtr);
    return stream->writable;
}

// ============================================================================
// TransformStreamDefaultController API
// ============================================================================

void nova_webstream_TransformController_enqueue(void* controllerPtr,
                                                 const uint8_t* data, size_t len) {
    auto* controller = static_cast<WebTransformStreamController*>(controllerPtr);
    if (controller->enqueue) {
        controller->enqueue(data, len);
    }
}

void nova_webstream_TransformController_error(void* controllerPtr, const char* error) {
    auto* controller = static_cast<WebTransformStreamController*>(controllerPtr);
    if (controller->error) {
        controller->error(error);
    }
}

void nova_webstream_TransformController_terminate(void* controllerPtr) {
    auto* controller = static_cast<WebTransformStreamController*>(controllerPtr);
    if (controller->terminate) {
        controller->terminate();
    }
}

size_t nova_webstream_TransformController_desiredSize(void* controllerPtr) {
    auto* controller = static_cast<WebTransformStreamController*>(controllerPtr);
    if (controller->stream && controller->stream->readable) {
        return nova_webstream_ReadableController_desiredSize(
            controller->stream->readable->controller);
    }
    return 0;
}

// ============================================================================
// Queuing Strategies
// ============================================================================

void* nova_webstream_ByteLengthQueuingStrategy_new(size_t highWaterMark) {
    auto* strategy = new QueuingStrategy();
    strategy->highWaterMark = highWaterMark;
    strategy->useByteLength = true;
    return strategy;
}

void* nova_webstream_CountQueuingStrategy_new(size_t highWaterMark) {
    auto* strategy = new QueuingStrategy();
    strategy->highWaterMark = highWaterMark;
    strategy->useByteLength = false;
    return strategy;
}

size_t nova_webstream_QueuingStrategy_highWaterMark(void* strategyPtr) {
    auto* strategy = static_cast<QueuingStrategy*>(strategyPtr);
    return strategy->highWaterMark;
}

size_t nova_webstream_QueuingStrategy_size(void* strategyPtr, const uint8_t* chunk, size_t len) {
    auto* strategy = static_cast<QueuingStrategy*>(strategyPtr);
    return strategy->useByteLength ? len : 1;
    (void)chunk;
}

void nova_webstream_QueuingStrategy_free(void* strategyPtr) {
    delete static_cast<QueuingStrategy*>(strategyPtr);
}

// ============================================================================
// Cleanup
// ============================================================================

void nova_webstream_ReadableStream_free(void* streamPtr) {
    auto* stream = static_cast<WebReadableStream*>(streamPtr);
    if (stream->controller) {
        delete static_cast<WebReadableStreamController*>(stream->controller);
    }
    if (stream->reader) {
        delete stream->reader;
    }
    delete stream;
}

void nova_webstream_WritableStream_free(void* streamPtr) {
    auto* stream = static_cast<WebWritableStream*>(streamPtr);
    if (stream->controller) {
        delete static_cast<WebWritableStreamController*>(stream->controller);
    }
    if (stream->writer) {
        delete stream->writer;
    }
    delete stream;
}

void nova_webstream_TransformStream_free(void* streamPtr) {
    auto* stream = static_cast<WebTransformStream*>(streamPtr);
    if (stream->readable) {
        nova_webstream_ReadableStream_free(stream->readable);
    }
    if (stream->writable) {
        nova_webstream_WritableStream_free(stream->writable);
    }
    if (stream->controller) {
        delete static_cast<WebTransformStreamController*>(stream->controller);
    }
    delete stream;
}

} // extern "C"

} // namespace webstreams
} // namespace runtime
} // namespace nova
