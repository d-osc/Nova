/**
 * nova:diagnostics_channel - Diagnostics Channel Module Implementation
 *
 * Provides publish-subscribe diagnostic channels for Nova programs.
 * Compatible with Node.js diagnostics_channel module.
 */

#include "nova/runtime/BuiltinModules.h"
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cstdint>
#include <string>
#include <map>
#include <vector>
#include <functional>

namespace nova {
namespace runtime {
namespace diagnostics_channel {

// Helper to allocate and copy string
static char* allocString(const std::string& str) {
    char* result = (char*)malloc(str.length() + 1);
    if (result) {
        strcpy(result, str.c_str());
    }
    return result;
}

// ============================================================================
// Subscriber callback type
// ============================================================================

typedef void (*SubscriberCallback)(void* message, const char* name);

// ============================================================================
// Channel Structure
// ============================================================================

struct Channel {
    char* name;
    std::vector<SubscriberCallback> subscribers;
    std::vector<void*> stores;  // AsyncLocalStorage bindings
    std::vector<void* (*)(void*)> storeTransforms;
};

// Global channel registry
static std::map<std::string, Channel*> channels;

// ============================================================================
// TracingChannel Structure
// ============================================================================

struct TracingChannel {
    char* name;
    Channel* start;
    Channel* end;
    Channel* asyncStart;
    Channel* asyncEnd;
    Channel* error;
};

static std::map<std::string, TracingChannel*> tracingChannels;

extern "C" {

// ============================================================================
// Channel Functions
// ============================================================================

// Get or create a channel by name
void* nova_diagnostics_channel_channel(const char* name) {
    if (!name) return nullptr;

    std::string nameStr(name);
    auto it = channels.find(nameStr);
    if (it != channels.end()) {
        return it->second;
    }

    // Create new channel
    Channel* channel = new Channel();
    channel->name = allocString(name);
    channels[nameStr] = channel;
    return channel;
}

// Check if channel has subscribers (by name)
int nova_diagnostics_channel_hasSubscribers(const char* name) {
    if (!name) return 0;

    auto it = channels.find(std::string(name));
    if (it != channels.end()) {
        return !it->second->subscribers.empty() ? 1 : 0;
    }
    return 0;
}

// Subscribe to a channel by name
void nova_diagnostics_channel_subscribe(const char* name, void* onMessage) {
    if (!name || !onMessage) return;

    Channel* channel = (Channel*)nova_diagnostics_channel_channel(name);
    if (channel) {
        channel->subscribers.push_back((SubscriberCallback)onMessage);
    }
}

// Unsubscribe from a channel by name
int nova_diagnostics_channel_unsubscribe(const char* name, void* onMessage) {
    if (!name || !onMessage) return 0;

    auto it = channels.find(std::string(name));
    if (it == channels.end()) return 0;

    Channel* channel = it->second;
    auto& subs = channel->subscribers;
    for (auto subIt = subs.begin(); subIt != subs.end(); ++subIt) {
        if (*subIt == (SubscriberCallback)onMessage) {
            subs.erase(subIt);
            return 1;
        }
    }
    return 0;
}

// ============================================================================
// Channel Object Methods
// ============================================================================

// Get channel name
char* nova_diagnostics_channel_Channel_name(void* channelPtr) {
    if (!channelPtr) return nullptr;
    Channel* channel = (Channel*)channelPtr;
    return channel->name ? allocString(channel->name) : nullptr;
}

// Check if channel has subscribers
int nova_diagnostics_channel_Channel_hasSubscribers(void* channelPtr) {
    if (!channelPtr) return 0;
    Channel* channel = (Channel*)channelPtr;
    return !channel->subscribers.empty() ? 1 : 0;
}

// Publish message to channel
void nova_diagnostics_channel_Channel_publish(void* channelPtr, void* message) {
    if (!channelPtr) return;

    Channel* channel = (Channel*)channelPtr;
    for (auto& subscriber : channel->subscribers) {
        if (subscriber) {
            subscriber(message, channel->name);
        }
    }
}

// Subscribe to channel
void nova_diagnostics_channel_Channel_subscribe(void* channelPtr, void* onMessage) {
    if (!channelPtr || !onMessage) return;

    Channel* channel = (Channel*)channelPtr;
    channel->subscribers.push_back((SubscriberCallback)onMessage);
}

// Unsubscribe from channel
int nova_diagnostics_channel_Channel_unsubscribe(void* channelPtr, void* onMessage) {
    if (!channelPtr || !onMessage) return 0;

    Channel* channel = (Channel*)channelPtr;
    auto& subs = channel->subscribers;
    for (auto it = subs.begin(); it != subs.end(); ++it) {
        if (*it == (SubscriberCallback)onMessage) {
            subs.erase(it);
            return 1;
        }
    }
    return 0;
}

// Bind AsyncLocalStorage to channel
void nova_diagnostics_channel_Channel_bindStore(void* channelPtr, void* store, void* transform) {
    if (!channelPtr || !store) return;

    Channel* channel = (Channel*)channelPtr;
    channel->stores.push_back(store);
    channel->storeTransforms.push_back((void* (*)(void*))transform);
}

// Unbind store from channel
int nova_diagnostics_channel_Channel_unbindStore(void* channelPtr, void* store) {
    if (!channelPtr || !store) return 0;

    Channel* channel = (Channel*)channelPtr;
    for (size_t i = 0; i < channel->stores.size(); i++) {
        if (channel->stores[i] == store) {
            channel->stores.erase(channel->stores.begin() + i);
            channel->storeTransforms.erase(channel->storeTransforms.begin() + i);
            return 1;
        }
    }
    return 0;
}

// Run function with stores
void nova_diagnostics_channel_Channel_runStores(
    void* channelPtr,
    void* context,
    void (*fn)(void*),
    void* thisArg
) {
    if (!channelPtr || !fn) return;

    Channel* channel = (Channel*)channelPtr;

    // Apply transforms and enter stores
    std::vector<void*> transformedContexts;
    for (size_t i = 0; i < channel->stores.size(); i++) {
        void* transformed = context;
        if (channel->storeTransforms[i]) {
            transformed = channel->storeTransforms[i](context);
        }
        transformedContexts.push_back(transformed);
        // In a full implementation, would call store.enterWith(transformed)
    }

    // Run the function
    fn(thisArg);

    // Exit stores (in reverse order)
    // In a full implementation, would restore previous store values
}

// ============================================================================
// TracingChannel Functions
// ============================================================================

// Create or get a TracingChannel
void* nova_diagnostics_channel_tracingChannel(const char* name) {
    if (!name) return nullptr;

    std::string nameStr(name);
    auto it = tracingChannels.find(nameStr);
    if (it != tracingChannels.end()) {
        return it->second;
    }

    // Create new tracing channel with sub-channels
    TracingChannel* tc = new TracingChannel();
    tc->name = allocString(name);

    std::string startName = nameStr + ":start";
    std::string endName = nameStr + ":end";
    std::string asyncStartName = nameStr + ":asyncStart";
    std::string asyncEndName = nameStr + ":asyncEnd";
    std::string errorName = nameStr + ":error";

    tc->start = (Channel*)nova_diagnostics_channel_channel(startName.c_str());
    tc->end = (Channel*)nova_diagnostics_channel_channel(endName.c_str());
    tc->asyncStart = (Channel*)nova_diagnostics_channel_channel(asyncStartName.c_str());
    tc->asyncEnd = (Channel*)nova_diagnostics_channel_channel(asyncEndName.c_str());
    tc->error = (Channel*)nova_diagnostics_channel_channel(errorName.c_str());

    tracingChannels[nameStr] = tc;
    return tc;
}

// Get TracingChannel name
char* nova_diagnostics_channel_TracingChannel_name(void* tcPtr) {
    if (!tcPtr) return nullptr;
    TracingChannel* tc = (TracingChannel*)tcPtr;
    return tc->name ? allocString(tc->name) : nullptr;
}

// Get sub-channels
void* nova_diagnostics_channel_TracingChannel_start(void* tcPtr) {
    if (!tcPtr) return nullptr;
    return ((TracingChannel*)tcPtr)->start;
}

void* nova_diagnostics_channel_TracingChannel_end(void* tcPtr) {
    if (!tcPtr) return nullptr;
    return ((TracingChannel*)tcPtr)->end;
}

void* nova_diagnostics_channel_TracingChannel_asyncStart(void* tcPtr) {
    if (!tcPtr) return nullptr;
    return ((TracingChannel*)tcPtr)->asyncStart;
}

void* nova_diagnostics_channel_TracingChannel_asyncEnd(void* tcPtr) {
    if (!tcPtr) return nullptr;
    return ((TracingChannel*)tcPtr)->asyncEnd;
}

void* nova_diagnostics_channel_TracingChannel_error(void* tcPtr) {
    if (!tcPtr) return nullptr;
    return ((TracingChannel*)tcPtr)->error;
}

// Subscribe to all tracing channel events
void nova_diagnostics_channel_TracingChannel_subscribe(
    void* tcPtr,
    void* onStart,
    void* onEnd,
    void* onAsyncStart,
    void* onAsyncEnd,
    void* onError
) {
    if (!tcPtr) return;

    TracingChannel* tc = (TracingChannel*)tcPtr;

    if (onStart && tc->start) {
        nova_diagnostics_channel_Channel_subscribe(tc->start, onStart);
    }
    if (onEnd && tc->end) {
        nova_diagnostics_channel_Channel_subscribe(tc->end, onEnd);
    }
    if (onAsyncStart && tc->asyncStart) {
        nova_diagnostics_channel_Channel_subscribe(tc->asyncStart, onAsyncStart);
    }
    if (onAsyncEnd && tc->asyncEnd) {
        nova_diagnostics_channel_Channel_subscribe(tc->asyncEnd, onAsyncEnd);
    }
    if (onError && tc->error) {
        nova_diagnostics_channel_Channel_subscribe(tc->error, onError);
    }
}

// Unsubscribe from all tracing channel events
void nova_diagnostics_channel_TracingChannel_unsubscribe(
    void* tcPtr,
    void* onStart,
    void* onEnd,
    void* onAsyncStart,
    void* onAsyncEnd,
    void* onError
) {
    if (!tcPtr) return;

    TracingChannel* tc = (TracingChannel*)tcPtr;

    if (onStart && tc->start) {
        nova_diagnostics_channel_Channel_unsubscribe(tc->start, onStart);
    }
    if (onEnd && tc->end) {
        nova_diagnostics_channel_Channel_unsubscribe(tc->end, onEnd);
    }
    if (onAsyncStart && tc->asyncStart) {
        nova_diagnostics_channel_Channel_unsubscribe(tc->asyncStart, onAsyncStart);
    }
    if (onAsyncEnd && tc->asyncEnd) {
        nova_diagnostics_channel_Channel_unsubscribe(tc->asyncEnd, onAsyncEnd);
    }
    if (onError && tc->error) {
        nova_diagnostics_channel_Channel_unsubscribe(tc->error, onError);
    }
}

// Trace synchronous function
void* nova_diagnostics_channel_TracingChannel_traceSync(
    void* tcPtr,
    void* (*fn)(void*),
    void* context,
    void* thisArg
) {
    if (!tcPtr || !fn) return nullptr;

    TracingChannel* tc = (TracingChannel*)tcPtr;

    // Publish start event
    if (tc->start) {
        nova_diagnostics_channel_Channel_publish(tc->start, context);
    }

    // Call function
    void* result = fn(thisArg);

    // Publish end event
    if (tc->end) {
        nova_diagnostics_channel_Channel_publish(tc->end, context);
    }

    return result;
}

// Publish error event (call separately if function throws)
void nova_diagnostics_channel_TracingChannel_publishError(void* tcPtr, void* context) {
    if (!tcPtr) return;
    TracingChannel* tc = (TracingChannel*)tcPtr;
    if (tc->error) {
        nova_diagnostics_channel_Channel_publish(tc->error, context);
    }
}

// Trace function that returns a promise (simplified)
void* nova_diagnostics_channel_TracingChannel_tracePromise(
    void* tcPtr,
    void* (*fn)(void*),
    void* context,
    void* thisArg
) {
    // For promises, we publish start, then return
    // The end/error events should be handled by promise resolution
    if (!tcPtr || !fn) return nullptr;

    TracingChannel* tc = (TracingChannel*)tcPtr;

    // Publish start event
    if (tc->start) {
        nova_diagnostics_channel_Channel_publish(tc->start, context);
    }

    // Call the function (returns a promise)
    void* promise = fn(thisArg);

    // In a full implementation, we would attach .then() and .catch()
    // to publish asyncEnd and error events

    return promise;
}

// Trace callback-based function
void nova_diagnostics_channel_TracingChannel_traceCallback(
    void* tcPtr,
    void (*fn)(void*, void (*)(void*, void*), void*),
    [[maybe_unused]] int position,  // callback position in args
    void* context,
    void* thisArg,
    void (*originalCallback)(void*, void*)
) {
    if (!tcPtr || !fn) return;

    TracingChannel* tc = (TracingChannel*)tcPtr;

    // Publish start event
    if (tc->start) {
        nova_diagnostics_channel_Channel_publish(tc->start, context);
    }

    // Create wrapped callback that publishes asyncEnd
    // In a full implementation, this would wrap the callback properly
    fn(thisArg, originalCallback, context);
}

// ============================================================================
// Utility Functions
// ============================================================================

// Check if any channel has subscribers
int nova_diagnostics_channel_hasAnySubscribers() {
    for (auto& pair : channels) {
        if (!pair.second->subscribers.empty()) {
            return 1;
        }
    }
    return 0;
}

// Get all channel names
char** nova_diagnostics_channel_getChannelNames(int* count) {
    *count = (int)channels.size();
    if (*count == 0) return nullptr;

    char** names = (char**)malloc(*count * sizeof(char*));
    int i = 0;
    for (auto& pair : channels) {
        names[i++] = allocString(pair.first);
    }
    return names;
}

// Free channel names array
void nova_diagnostics_channel_freeChannelNames(char** names, int count) {
    if (names) {
        for (int i = 0; i < count; i++) {
            if (names[i]) free(names[i]);
        }
        free(names);
    }
}

// Free a channel
void nova_diagnostics_channel_Channel_free(void* channelPtr) {
    if (!channelPtr) return;

    Channel* channel = (Channel*)channelPtr;

    // Remove from global registry
    if (channel->name) {
        channels.erase(std::string(channel->name));
        free(channel->name);
    }

    delete channel;
}

// Free a tracing channel
void nova_diagnostics_channel_TracingChannel_free(void* tcPtr) {
    if (!tcPtr) return;

    TracingChannel* tc = (TracingChannel*)tcPtr;

    // Remove from global registry
    if (tc->name) {
        tracingChannels.erase(std::string(tc->name));
        free(tc->name);
    }

    // Note: sub-channels are managed by the global channels map
    delete tc;
}

// Cleanup all channels
void nova_diagnostics_channel_cleanup() {
    for (auto& pair : channels) {
        if (pair.second->name) free(pair.second->name);
        delete pair.second;
    }
    channels.clear();

    for (auto& pair : tracingChannels) {
        if (pair.second->name) free(pair.second->name);
        delete pair.second;
    }
    tracingChannels.clear();
}

} // extern "C"

} // namespace diagnostics_channel
} // namespace runtime
} // namespace nova
