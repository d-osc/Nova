/**
 * nova:worker_threads - Worker Threads Module Implementation
 *
 * Provides Node.js-compatible worker threads for CPU-intensive operations.
 * Enables parallel JavaScript execution with message passing.
 */

#include <string>
#include <vector>
#include <map>
#include <deque>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <functional>
#include <cstring>
#include <cstdint>

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#include <unistd.h>
#endif

namespace nova {
namespace runtime {
namespace worker_threads {

// ============================================================================
// Forward Declarations
// ============================================================================

struct MessagePort;
struct Worker;
struct MessageChannel;

// ============================================================================
// Message Structure
// ============================================================================

struct Message {
    std::vector<uint8_t> data;
    std::string type;  // "message", "error", "messageerror"
    std::vector<MessagePort*> transferList;

    Message() {}
    Message(const uint8_t* d, size_t len, const char* t = "message")
        : data(d, d + len), type(t ? t : "message") {}
};

// ============================================================================
// MessagePort
// ============================================================================

struct MessagePort {
    int64_t id;
    std::deque<Message> messageQueue;
    std::mutex queueMutex;
    std::condition_variable queueCondition;
    bool started;
    bool closed;
    MessagePort* remotePort;  // Connected port

    // Event callbacks
    std::function<void(const uint8_t*, size_t)> onMessage;
    std::function<void(const char*)> onMessageError;
    std::function<void()> onClose;

    MessagePort() : id(0), started(false), closed(false), remotePort(nullptr) {}
};

// ============================================================================
// MessageChannel
// ============================================================================

struct MessageChannel {
    MessagePort* port1;
    MessagePort* port2;

    MessageChannel() : port1(nullptr), port2(nullptr) {}
};

// ============================================================================
// Worker
// ============================================================================

struct Worker {
    int64_t threadId;
    std::string filename;
    std::vector<uint8_t> workerData;
    bool isRunning;
    int exitCode;
    std::thread* thread;
    MessagePort* parentPort;  // Port to communicate with parent
    MessagePort* workerPort;  // Port in worker context

    // Resource limits
    double maxYoungGenerationSizeMb;
    double maxOldGenerationSizeMb;
    double codeRangeSizeMb;
    double stackSizeMb;

    // Environment
    std::map<std::string, std::string> env;
    bool shareEnv;
    std::vector<std::string> argv;
    std::vector<std::string> execArgv;
    std::string name;
    bool stdin_;
    bool stdout_;
    bool stderr_;
    bool trackUnmanagedFds;

    // Event callbacks
    std::function<void()> onOnline;
    std::function<void(const uint8_t*, size_t)> onMessage;
    std::function<void(const char*)> onMessageError;
    std::function<void(const char*)> onError;
    std::function<void(int)> onExit;

    Worker() : threadId(0), isRunning(false), exitCode(0), thread(nullptr),
               parentPort(nullptr), workerPort(nullptr),
               maxYoungGenerationSizeMb(0), maxOldGenerationSizeMb(0),
               codeRangeSizeMb(0), stackSizeMb(4),
               shareEnv(false), stdin_(false), stdout_(false), stderr_(false),
               trackUnmanagedFds(false) {}
};

// ============================================================================
// BroadcastChannel
// ============================================================================

struct BroadcastChannel {
    std::string name;
    bool closed;
    std::function<void(const uint8_t*, size_t)> onMessage;
    std::function<void(const char*)> onMessageError;

    BroadcastChannel() : closed(false) {}
};

// ============================================================================
// Global State
// ============================================================================

static std::atomic<int64_t> nextPortId{1};
static std::atomic<int64_t> nextThreadId{1};
static std::mutex globalMutex;

// Main thread state
static bool isMainThread_ = true;
static int64_t currentThreadId_ = 0;
static MessagePort* parentPort_ = nullptr;
static std::vector<uint8_t> workerData_;
static std::map<std::string, std::string> environmentData_;
static std::map<std::string, std::vector<BroadcastChannel*>> broadcastChannels_;

// All workers
static std::map<int64_t, Worker*> workers_;

extern "C" {

// ============================================================================
// Module Properties
// ============================================================================

int nova_worker_threads_isMainThread() {
    return isMainThread_ ? 1 : 0;
}

int64_t nova_worker_threads_threadId() {
    return currentThreadId_;
}

void* nova_worker_threads_parentPort() {
    return parentPort_;
}

const uint8_t* nova_worker_threads_workerData(size_t* outLen) {
    if (outLen) *outLen = workerData_.size();
    return workerData_.empty() ? nullptr : workerData_.data();
}

// ============================================================================
// Environment Data
// ============================================================================

void nova_worker_threads_setEnvironmentData(const char* key, const uint8_t* value, size_t len) {
    if (!key) return;
    std::lock_guard<std::mutex> lock(globalMutex);
    environmentData_[key] = std::string(reinterpret_cast<const char*>(value), len);
}

const uint8_t* nova_worker_threads_getEnvironmentData(const char* key, size_t* outLen) {
    if (!key || !outLen) return nullptr;
    std::lock_guard<std::mutex> lock(globalMutex);

    auto it = environmentData_.find(key);
    if (it == environmentData_.end()) {
        *outLen = 0;
        return nullptr;
    }

    *outLen = it->second.size();
    return reinterpret_cast<const uint8_t*>(it->second.data());
}

// ============================================================================
// MessagePort API
// ============================================================================

void* nova_worker_threads_MessagePort_new() {
    auto* port = new MessagePort();
    port->id = nextPortId++;
    return port;
}

void nova_worker_threads_MessagePort_postMessage(void* portPtr, const uint8_t* data, size_t len,
                                                  void** transferList, int transferCount) {
    auto* port = static_cast<MessagePort*>(portPtr);
    if (!port || port->closed) return;

    Message msg(data, len);

    // Handle transfer list
    for (int i = 0; i < transferCount; i++) {
        if (transferList && transferList[i]) {
            msg.transferList.push_back(static_cast<MessagePort*>(transferList[i]));
        }
    }

    // Send to remote port if connected
    if (port->remotePort && !port->remotePort->closed) {
        std::lock_guard<std::mutex> lock(port->remotePort->queueMutex);
        port->remotePort->messageQueue.push_back(msg);
        port->remotePort->queueCondition.notify_one();

        // If started and has callback, dispatch immediately
        if (port->remotePort->started && port->remotePort->onMessage) {
            port->remotePort->onMessage(data, len);
        }
    }
}

void nova_worker_threads_MessagePort_start(void* portPtr) {
    auto* port = static_cast<MessagePort*>(portPtr);
    if (!port) return;

    port->started = true;

    // Process any queued messages
    std::lock_guard<std::mutex> lock(port->queueMutex);
    while (!port->messageQueue.empty() && port->onMessage) {
        auto& msg = port->messageQueue.front();
        port->onMessage(msg.data.data(), msg.data.size());
        port->messageQueue.pop_front();
    }
}

void nova_worker_threads_MessagePort_close(void* portPtr) {
    auto* port = static_cast<MessagePort*>(portPtr);
    if (!port || port->closed) return;

    port->closed = true;
    port->queueCondition.notify_all();

    if (port->onClose) port->onClose();
}

int nova_worker_threads_MessagePort_hasRef(void* portPtr) {
    auto* port = static_cast<MessagePort*>(portPtr);
    return (port && !port->closed) ? 1 : 0;
}

void nova_worker_threads_MessagePort_ref(void* portPtr) {
    (void)portPtr;  // Keep-alive reference
}

void nova_worker_threads_MessagePort_unref(void* portPtr) {
    (void)portPtr;  // Release keep-alive
}

void nova_worker_threads_MessagePort_onMessage(void* portPtr, void (*callback)(const uint8_t*, size_t)) {
    auto* port = static_cast<MessagePort*>(portPtr);
    if (port) port->onMessage = callback;
}

void nova_worker_threads_MessagePort_onMessageError(void* portPtr, void (*callback)(const char*)) {
    auto* port = static_cast<MessagePort*>(portPtr);
    if (port) port->onMessageError = callback;
}

void nova_worker_threads_MessagePort_onClose(void* portPtr, void (*callback)()) {
    auto* port = static_cast<MessagePort*>(portPtr);
    if (port) port->onClose = callback;
}

void nova_worker_threads_MessagePort_free(void* portPtr) {
    auto* port = static_cast<MessagePort*>(portPtr);
    if (port) {
        port->closed = true;
        delete port;
    }
}

// ============================================================================
// MessageChannel API
// ============================================================================

void* nova_worker_threads_MessageChannel_new() {
    auto* channel = new MessageChannel();

    channel->port1 = static_cast<MessagePort*>(nova_worker_threads_MessagePort_new());
    channel->port2 = static_cast<MessagePort*>(nova_worker_threads_MessagePort_new());

    // Connect the ports
    channel->port1->remotePort = channel->port2;
    channel->port2->remotePort = channel->port1;

    return channel;
}

void* nova_worker_threads_MessageChannel_port1(void* channelPtr) {
    auto* channel = static_cast<MessageChannel*>(channelPtr);
    return channel ? channel->port1 : nullptr;
}

void* nova_worker_threads_MessageChannel_port2(void* channelPtr) {
    auto* channel = static_cast<MessageChannel*>(channelPtr);
    return channel ? channel->port2 : nullptr;
}

void nova_worker_threads_MessageChannel_free(void* channelPtr) {
    auto* channel = static_cast<MessageChannel*>(channelPtr);
    if (channel) {
        // Don't delete ports - they may be transferred
        delete channel;
    }
}

// ============================================================================
// Worker API
// ============================================================================

void* nova_worker_threads_Worker_new(const char* filename,
                                      const uint8_t* workerData, size_t workerDataLen,
                                      const char** argv, int argc,
                                      const char** execArgv, int execArgc,
                                      const char** envKeys, const char** envValues, int envCount,
                                      int shareEnv, const char* name,
                                      double stackSizeMb) {
    auto* worker = new Worker();
    worker->threadId = nextThreadId++;
    worker->filename = filename ? filename : "";

    if (workerData && workerDataLen > 0) {
        worker->workerData.assign(workerData, workerData + workerDataLen);
    }

    for (int i = 0; i < argc; i++) {
        if (argv && argv[i]) worker->argv.push_back(argv[i]);
    }

    for (int i = 0; i < execArgc; i++) {
        if (execArgv && execArgv[i]) worker->execArgv.push_back(execArgv[i]);
    }

    for (int i = 0; i < envCount; i++) {
        if (envKeys && envKeys[i]) {
            worker->env[envKeys[i]] = (envValues && envValues[i]) ? envValues[i] : "";
        }
    }

    worker->shareEnv = shareEnv != 0;
    worker->name = name ? name : "";
    worker->stackSizeMb = stackSizeMb > 0 ? stackSizeMb : 4;

    // Create communication channel
    auto* channel = static_cast<MessageChannel*>(nova_worker_threads_MessageChannel_new());
    worker->parentPort = channel->port1;
    worker->workerPort = channel->port2;

    // Store in global map
    {
        std::lock_guard<std::mutex> lock(globalMutex);
        workers_[worker->threadId] = worker;
    }

    // Start the worker thread
    worker->isRunning = true;
    worker->thread = new std::thread([worker]() {
        // Set thread-local state
        isMainThread_ = false;
        currentThreadId_ = worker->threadId;
        parentPort_ = worker->workerPort;
        workerData_ = worker->workerData;

        // Notify online
        if (worker->onOnline) worker->onOnline();

        // In real implementation, load and execute the script file
        // For now, just simulate running
        // The actual script execution would happen here

        // Wait for messages (simplified event loop)
        while (worker->isRunning) {
            std::unique_lock<std::mutex> lock(worker->workerPort->queueMutex);
            worker->workerPort->queueCondition.wait_for(lock, std::chrono::milliseconds(100));

            while (!worker->workerPort->messageQueue.empty()) {
                auto msg = worker->workerPort->messageQueue.front();
                worker->workerPort->messageQueue.pop_front();
                lock.unlock();

                if (worker->workerPort->onMessage) {
                    worker->workerPort->onMessage(msg.data.data(), msg.data.size());
                }

                lock.lock();
            }
        }

        worker->exitCode = 0;
    });

    return worker;
}

int64_t nova_worker_threads_Worker_threadId(void* workerPtr) {
    auto* worker = static_cast<Worker*>(workerPtr);
    return worker ? worker->threadId : 0;
}

void nova_worker_threads_Worker_postMessage(void* workerPtr, const uint8_t* data, size_t len,
                                             void** transferList, int transferCount) {
    auto* worker = static_cast<Worker*>(workerPtr);
    if (!worker || !worker->parentPort) return;

    nova_worker_threads_MessagePort_postMessage(worker->parentPort, data, len,
                                                 transferList, transferCount);
}

void nova_worker_threads_Worker_terminate(void* workerPtr,
                                           void (*resolve)(int), void (*reject)(const char*)) {
    auto* worker = static_cast<Worker*>(workerPtr);
    if (!worker) {
        if (reject) reject("Invalid worker");
        return;
    }

    worker->isRunning = false;

    if (worker->workerPort) {
        worker->workerPort->queueCondition.notify_all();
    }

    if (worker->thread && worker->thread->joinable()) {
        worker->thread->join();
    }

    if (resolve) resolve(worker->exitCode);
}

void* nova_worker_threads_Worker_getStdin(void* workerPtr) {
    auto* worker = static_cast<Worker*>(workerPtr);
    return worker && worker->stdin_ ? worker : nullptr;
}

void* nova_worker_threads_Worker_getStdout(void* workerPtr) {
    auto* worker = static_cast<Worker*>(workerPtr);
    return worker && worker->stdout_ ? worker : nullptr;
}

void* nova_worker_threads_Worker_getStderr(void* workerPtr) {
    auto* worker = static_cast<Worker*>(workerPtr);
    return worker && worker->stderr_ ? worker : nullptr;
}

double nova_worker_threads_Worker_performance_eventLoopUtilization(void* workerPtr) {
    (void)workerPtr;
    return 0.5;  // Simulated
}

void nova_worker_threads_Worker_ref(void* workerPtr) {
    (void)workerPtr;
}

void nova_worker_threads_Worker_unref(void* workerPtr) {
    (void)workerPtr;
}

// Event handlers
void nova_worker_threads_Worker_onOnline(void* workerPtr, void (*callback)()) {
    auto* worker = static_cast<Worker*>(workerPtr);
    if (worker) worker->onOnline = callback;
}

void nova_worker_threads_Worker_onMessage(void* workerPtr, void (*callback)(const uint8_t*, size_t)) {
    auto* worker = static_cast<Worker*>(workerPtr);
    if (worker && worker->parentPort) {
        worker->parentPort->onMessage = callback;
    }
}

void nova_worker_threads_Worker_onMessageError(void* workerPtr, void (*callback)(const char*)) {
    auto* worker = static_cast<Worker*>(workerPtr);
    if (worker) worker->onMessageError = callback;
}

void nova_worker_threads_Worker_onError(void* workerPtr, void (*callback)(const char*)) {
    auto* worker = static_cast<Worker*>(workerPtr);
    if (worker) worker->onError = callback;
}

void nova_worker_threads_Worker_onExit(void* workerPtr, void (*callback)(int)) {
    auto* worker = static_cast<Worker*>(workerPtr);
    if (worker) worker->onExit = callback;
}

void nova_worker_threads_Worker_free(void* workerPtr) {
    auto* worker = static_cast<Worker*>(workerPtr);
    if (!worker) return;

    // Terminate if still running
    worker->isRunning = false;
    if (worker->workerPort) {
        worker->workerPort->queueCondition.notify_all();
    }

    if (worker->thread) {
        if (worker->thread->joinable()) {
            worker->thread->join();
        }
        delete worker->thread;
    }

    // Remove from global map
    {
        std::lock_guard<std::mutex> lock(globalMutex);
        workers_.erase(worker->threadId);
    }

    if (worker->parentPort) nova_worker_threads_MessagePort_free(worker->parentPort);
    if (worker->workerPort) nova_worker_threads_MessagePort_free(worker->workerPort);

    delete worker;
}

// ============================================================================
// BroadcastChannel API
// ============================================================================

void* nova_worker_threads_BroadcastChannel_new(const char* name) {
    auto* channel = new BroadcastChannel();
    channel->name = name ? name : "";

    // Register in global map
    std::lock_guard<std::mutex> lock(globalMutex);
    broadcastChannels_[channel->name].push_back(channel);

    return channel;
}

const char* nova_worker_threads_BroadcastChannel_name(void* channelPtr) {
    auto* channel = static_cast<BroadcastChannel*>(channelPtr);
    return channel ? channel->name.c_str() : "";
}

void nova_worker_threads_BroadcastChannel_postMessage(void* channelPtr,
                                                       const uint8_t* data, size_t len) {
    auto* channel = static_cast<BroadcastChannel*>(channelPtr);
    if (!channel || channel->closed) return;

    std::lock_guard<std::mutex> lock(globalMutex);

    // Send to all channels with same name except sender
    auto& channels = broadcastChannels_[channel->name];
    for (auto* other : channels) {
        if (other != channel && !other->closed && other->onMessage) {
            other->onMessage(data, len);
        }
    }
}

void nova_worker_threads_BroadcastChannel_close(void* channelPtr) {
    auto* channel = static_cast<BroadcastChannel*>(channelPtr);
    if (!channel) return;

    channel->closed = true;

    // Remove from global map
    std::lock_guard<std::mutex> lock(globalMutex);
    auto& channels = broadcastChannels_[channel->name];
    channels.erase(std::remove(channels.begin(), channels.end(), channel), channels.end());
}

void nova_worker_threads_BroadcastChannel_ref(void* channelPtr) {
    (void)channelPtr;
}

void nova_worker_threads_BroadcastChannel_unref(void* channelPtr) {
    (void)channelPtr;
}

void nova_worker_threads_BroadcastChannel_onMessage(void* channelPtr,
                                                     void (*callback)(const uint8_t*, size_t)) {
    auto* channel = static_cast<BroadcastChannel*>(channelPtr);
    if (channel) channel->onMessage = callback;
}

void nova_worker_threads_BroadcastChannel_onMessageError(void* channelPtr,
                                                          void (*callback)(const char*)) {
    auto* channel = static_cast<BroadcastChannel*>(channelPtr);
    if (channel) channel->onMessageError = callback;
}

void nova_worker_threads_BroadcastChannel_free(void* channelPtr) {
    auto* channel = static_cast<BroadcastChannel*>(channelPtr);
    if (channel) {
        nova_worker_threads_BroadcastChannel_close(channel);
        delete channel;
    }
}

// ============================================================================
// Utility Functions
// ============================================================================

// receiveMessageOnPort - Synchronously receive a message
int nova_worker_threads_receiveMessageOnPort(void* portPtr, uint8_t** outData, size_t* outLen) {
    auto* port = static_cast<MessagePort*>(portPtr);
    if (!port || port->closed) return 0;

    std::lock_guard<std::mutex> lock(port->queueMutex);

    if (port->messageQueue.empty()) {
        return 0;  // No message available
    }

    auto& msg = port->messageQueue.front();

    // Allocate and copy data
    *outLen = msg.data.size();
    *outData = (uint8_t*)malloc(*outLen);
    memcpy(*outData, msg.data.data(), *outLen);

    port->messageQueue.pop_front();
    return 1;
}

// markAsUntransferable
void nova_worker_threads_markAsUntransferable(void* object) {
    (void)object;  // Mark object as not transferable
}

// moveMessagePortToContext
void* nova_worker_threads_moveMessagePortToContext(void* portPtr, void* context) {
    (void)context;
    return portPtr;  // In real implementation, move port to different context
}

// getHeapSnapshot (returns serialized heap snapshot)
void nova_worker_threads_getHeapSnapshot(void* workerPtr,
                                          void (*callback)(const uint8_t*, size_t)) {
    (void)workerPtr;
    // Return empty snapshot for now
    if (callback) callback(nullptr, 0);
}

// SHARE_ENV symbol constant
int nova_worker_threads_SHARE_ENV() {
    return 1;  // Symbol value for sharing environment
}

// resourceLimits
void nova_worker_threads_Worker_setResourceLimits(void* workerPtr,
                                                   double maxYoungGenerationSizeMb,
                                                   double maxOldGenerationSizeMb,
                                                   double codeRangeSizeMb,
                                                   double stackSizeMb) {
    auto* worker = static_cast<Worker*>(workerPtr);
    if (!worker) return;

    worker->maxYoungGenerationSizeMb = maxYoungGenerationSizeMb;
    worker->maxOldGenerationSizeMb = maxOldGenerationSizeMb;
    worker->codeRangeSizeMb = codeRangeSizeMb;
    worker->stackSizeMb = stackSizeMb;
}

void nova_worker_threads_Worker_getResourceLimits(void* workerPtr,
                                                   double* maxYoungGenerationSizeMb,
                                                   double* maxOldGenerationSizeMb,
                                                   double* codeRangeSizeMb,
                                                   double* stackSizeMb) {
    auto* worker = static_cast<Worker*>(workerPtr);
    if (!worker) return;

    if (maxYoungGenerationSizeMb) *maxYoungGenerationSizeMb = worker->maxYoungGenerationSizeMb;
    if (maxOldGenerationSizeMb) *maxOldGenerationSizeMb = worker->maxOldGenerationSizeMb;
    if (codeRangeSizeMb) *codeRangeSizeMb = worker->codeRangeSizeMb;
    if (stackSizeMb) *stackSizeMb = worker->stackSizeMb;
}

} // extern "C"

} // namespace worker_threads
} // namespace runtime
} // namespace nova
