// Timers Runtime Implementation for Nova Compiler
// Web APIs: setTimeout, setInterval, clearTimeout, clearInterval

#include <cstdint>
#include <cstdlib>
#include <unordered_map>
#include <thread>
#include <chrono>
#include <mutex>
#include <atomic>
#include <functional>
#include <queue>
#include <condition_variable>

extern "C" {

// ============================================================================
// Timer Entry Structure
// ============================================================================

struct NovaTimerEntry {
    int64_t id;
    void* callback;          // Function pointer
    int64_t delay;           // Delay in milliseconds
    bool isInterval;         // true for setInterval, false for setTimeout
    bool cancelled;
    std::thread* thread;
};

// ============================================================================
// Timer Manager (Singleton)
// ============================================================================

static std::unordered_map<int64_t, NovaTimerEntry*>* timerRegistry = nullptr;
static std::mutex timerMutex;
static std::atomic<int64_t> nextTimerId{1};

static void ensureTimerRegistry() {
    if (!timerRegistry) {
        timerRegistry = new std::unordered_map<int64_t, NovaTimerEntry*>();
    }
}

// ============================================================================
// Timer execution thread function
// ============================================================================

typedef void (*NovaCallback)();

static void timerThreadFunc(int64_t timerId, int64_t delay, bool isInterval) {
    do {
        std::this_thread::sleep_for(std::chrono::milliseconds(delay));

        std::lock_guard<std::mutex> lock(timerMutex);
        if (!timerRegistry) return;

        auto it = timerRegistry->find(timerId);
        if (it == timerRegistry->end() || it->second->cancelled) {
            return;
        }

        // Execute callback
        NovaCallback cb = reinterpret_cast<NovaCallback>(it->second->callback);
        if (cb) {
            cb();
        }

        if (!isInterval) {
            return;
        }
    } while (isInterval);
}

// ============================================================================
// setTimeout(callback, delay)
// Executes callback after delay milliseconds
// Returns timer ID
// ============================================================================

int64_t nova_setTimeout(void* callback, int64_t delay) {
    std::lock_guard<std::mutex> lock(timerMutex);
    ensureTimerRegistry();

    int64_t id = nextTimerId++;

    NovaTimerEntry* entry = new NovaTimerEntry();
    entry->id = id;
    entry->callback = callback;
    entry->delay = delay < 0 ? 0 : delay;
    entry->isInterval = false;
    entry->cancelled = false;

    (*timerRegistry)[id] = entry;

    // Start timer thread
    entry->thread = new std::thread(timerThreadFunc, id, entry->delay, false);
    entry->thread->detach();

    return id;
}

// ============================================================================
// setInterval(callback, delay)
// Executes callback repeatedly every delay milliseconds
// Returns timer ID
// ============================================================================

int64_t nova_setInterval(void* callback, int64_t delay) {
    std::lock_guard<std::mutex> lock(timerMutex);
    ensureTimerRegistry();

    int64_t id = nextTimerId++;

    NovaTimerEntry* entry = new NovaTimerEntry();
    entry->id = id;
    entry->callback = callback;
    entry->delay = delay < 4 ? 4 : delay;  // Minimum 4ms per spec
    entry->isInterval = true;
    entry->cancelled = false;

    (*timerRegistry)[id] = entry;

    // Start interval thread
    entry->thread = new std::thread(timerThreadFunc, id, entry->delay, true);
    entry->thread->detach();

    return id;
}

// ============================================================================
// clearTimeout(id)
// Cancels a timeout set by setTimeout
// ============================================================================

void nova_clearTimeout(int64_t timerId) {
    std::lock_guard<std::mutex> lock(timerMutex);
    if (!timerRegistry) return;

    auto it = timerRegistry->find(timerId);
    if (it != timerRegistry->end()) {
        it->second->cancelled = true;
    }
}

// ============================================================================
// clearInterval(id)
// Cancels an interval set by setInterval
// ============================================================================

void nova_clearInterval(int64_t timerId) {
    std::lock_guard<std::mutex> lock(timerMutex);
    if (!timerRegistry) return;

    auto it = timerRegistry->find(timerId);
    if (it != timerRegistry->end()) {
        it->second->cancelled = true;
    }
}

// ============================================================================
// queueMicrotask(callback)
// Queues a microtask to be executed
// ============================================================================

static std::queue<void*>* microtaskQueue = nullptr;
static std::mutex microtaskMutex;

void nova_queueMicrotask(void* callback) {
    std::lock_guard<std::mutex> lock(microtaskMutex);
    if (!microtaskQueue) {
        microtaskQueue = new std::queue<void*>();
    }
    microtaskQueue->push(callback);
}

// Process all queued microtasks
void nova_processMicrotasks() {
    std::lock_guard<std::mutex> lock(microtaskMutex);
    if (!microtaskQueue) return;

    while (!microtaskQueue->empty()) {
        void* cb = microtaskQueue->front();
        microtaskQueue->pop();

        NovaCallback callback = reinterpret_cast<NovaCallback>(cb);
        if (callback) {
            callback();
        }
    }
}

// ============================================================================
// requestAnimationFrame(callback) - simulated at 60fps
// ============================================================================

static std::atomic<int64_t> nextAnimFrameId{1};
static std::unordered_map<int64_t, void*>* animFrameCallbacks = nullptr;

int64_t nova_requestAnimationFrame(void* callback) {
    std::lock_guard<std::mutex> lock(timerMutex);

    if (!animFrameCallbacks) {
        animFrameCallbacks = new std::unordered_map<int64_t, void*>();
    }

    int64_t id = nextAnimFrameId++;
    (*animFrameCallbacks)[id] = callback;

    // Simulate ~60fps (16.67ms)
    std::thread([id]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(16));

        std::lock_guard<std::mutex> lock(timerMutex);
        if (!animFrameCallbacks) return;

        auto it = animFrameCallbacks->find(id);
        if (it != animFrameCallbacks->end()) {
            NovaCallback cb = reinterpret_cast<NovaCallback>(it->second);
            if (cb) {
                cb();
            }
            animFrameCallbacks->erase(it);
        }
    }).detach();

    return id;
}

// ============================================================================
// cancelAnimationFrame(id)
// ============================================================================

void nova_cancelAnimationFrame(int64_t id) {
    std::lock_guard<std::mutex> lock(timerMutex);
    if (!animFrameCallbacks) return;

    animFrameCallbacks->erase(id);
}

// ============================================================================
// Cleanup helpers
// ============================================================================

void nova_timers_cleanup() {
    std::lock_guard<std::mutex> lock(timerMutex);

    if (timerRegistry) {
        for (auto& pair : *timerRegistry) {
            pair.second->cancelled = true;
            delete pair.second;
        }
        delete timerRegistry;
        timerRegistry = nullptr;
    }

    if (animFrameCallbacks) {
        delete animFrameCallbacks;
        animFrameCallbacks = nullptr;
    }
}

// ============================================================================
// setImmediate / clearImmediate
// ============================================================================

static std::atomic<int64_t> nextImmediateId{1};
static std::unordered_map<int64_t, void*>* immediateCallbacks = nullptr;
static std::mutex immediateMutex;

int64_t nova_setImmediate(void* callback) {
    std::lock_guard<std::mutex> lock(immediateMutex);
    if (!immediateCallbacks) immediateCallbacks = new std::unordered_map<int64_t, void*>();
    int64_t id = nextImmediateId++;
    (*immediateCallbacks)[id] = callback;
    std::thread([id]() {
        std::this_thread::yield();
        std::lock_guard<std::mutex> lk(immediateMutex);
        if (!immediateCallbacks) return;
        auto it = immediateCallbacks->find(id);
        if (it != immediateCallbacks->end()) {
            NovaCallback cb = reinterpret_cast<NovaCallback>(it->second);
            if (cb) cb();
            immediateCallbacks->erase(it);
        }
    }).detach();
    return id;
}

void nova_clearImmediate(int64_t id) {
    std::lock_guard<std::mutex> lock(immediateMutex);
    if (immediateCallbacks) immediateCallbacks->erase(id);
}

// ============================================================================
// timers/promises API
// ============================================================================

void nova_timers_setTimeout_promise(int64_t delay, int64_t value, bool* aborted,
                                     void (*resolve)(int64_t), void (*reject)(const char*)) {
    std::thread([=]() {
        if (aborted && *aborted) { if (reject) reject("AbortError"); return; }
        int64_t rem = delay;
        while (rem > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(rem > 50 ? 50 : rem));
            rem -= 50;
            if (aborted && *aborted) { if (reject) reject("AbortError"); return; }
        }
        if (resolve) resolve(value);
    }).detach();
}

void nova_timers_setImmediate_promise(int64_t value, bool* aborted,
                                       void (*resolve)(int64_t), void (*reject)(const char*)) {
    std::thread([=]() {
        std::this_thread::yield();
        if (aborted && *aborted) { if (reject) reject("AbortError"); return; }
        if (resolve) resolve(value);
    }).detach();
}

struct IntervalIterator { int64_t delay; int64_t counter; bool cancelled; bool* abortSignal; };

void* nova_timers_setInterval_iterator(int64_t delay, bool* abortSignal) {
    return new IntervalIterator{delay < 4 ? 4 : delay, 0, false, abortSignal};
}

void nova_timers_interval_next(void* iter, void (*resolve)(int64_t), void (*reject)(const char*)) {
    if (!iter) { if (reject) reject("Invalid"); return; }
    auto* it = (IntervalIterator*)iter;
    if (it->cancelled || (it->abortSignal && *it->abortSignal)) { if (reject) reject("AbortError"); return; }
    std::thread([=]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(it->delay));
        if (it->cancelled || (it->abortSignal && *it->abortSignal)) { if (reject) reject("AbortError"); return; }
        it->counter++;
        if (resolve) resolve(it->counter);
    }).detach();
}

void nova_timers_interval_cancel(void* iter) { if (iter) ((IntervalIterator*)iter)->cancelled = true; }
void nova_timers_interval_free(void* iter) { delete (IntervalIterator*)iter; }

// scheduler API
void nova_scheduler_wait(int64_t delay, bool* aborted, void (*resolve)(), void (*reject)(const char*)) {
    std::thread([=]() {
        int64_t rem = delay;
        while (rem > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(rem > 50 ? 50 : rem));
            rem -= 50;
            if (aborted && *aborted) { if (reject) reject("AbortError"); return; }
        }
        if (resolve) resolve();
    }).detach();
}

void nova_scheduler_yield(void (*resolve)()) {
    std::thread([=]() { std::this_thread::yield(); if (resolve) resolve(); }).detach();
}

// ============================================================================
// Timer utilities
// ============================================================================

int nova_timer_isActive(int64_t id) {
    std::lock_guard<std::mutex> lock(timerMutex);
    if (!timerRegistry) return 0;
    auto it = timerRegistry->find(id);
    return (it != timerRegistry->end() && !it->second->cancelled) ? 1 : 0;
}

void nova_timer_refresh(int64_t id) {
    std::lock_guard<std::mutex> lock(timerMutex);
    if (!timerRegistry) return;
    auto it = timerRegistry->find(id);
    if (it != timerRegistry->end() && !it->second->cancelled) {
        auto* old = it->second;
        old->cancelled = true;
        auto* n = new NovaTimerEntry();
        n->id = old->id; n->callback = old->callback; n->delay = old->delay;
        n->isInterval = old->isInterval; n->cancelled = false;
        (*timerRegistry)[id] = n;
        n->thread = new std::thread(timerThreadFunc, id, n->delay, n->isInterval);
        n->thread->detach();
        delete old;
    }
}

int nova_timer_hasRef(int64_t id) {
    std::lock_guard<std::mutex> lock(timerMutex);
    return (timerRegistry && timerRegistry->find(id) != timerRegistry->end()) ? 1 : 0;
}

void nova_timer_ref(int64_t id) { (void)id; }
void nova_timer_unref(int64_t id) { (void)id; }

int64_t nova_timers_activeCount() {
    std::lock_guard<std::mutex> lock(timerMutex);
    if (!timerRegistry) return 0;
    int64_t c = 0;
    for (const auto& p : *timerRegistry) if (!p.second->cancelled) c++;
    return c;
}

void nova_timers_clearAll() {
    std::lock_guard<std::mutex> lock(timerMutex);
    if (timerRegistry) for (auto& p : *timerRegistry) p.second->cancelled = true;
}

} // extern "C"
