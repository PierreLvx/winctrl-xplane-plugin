#ifndef DATAREF_H
#define DATAREF_H

#include <deque>
#include <functional>
#include <future>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <variant>
#include <vector>
#include <XPLMDataAccess.h>
#include <XPLMUtilities.h>

using DataRefValueType = std::
    variant<float, double, int, bool, std::string, std::vector<int>, std::vector<float>, std::vector<unsigned char>>;
template<typename T>
using DatarefShouldChangeCallback = std::function<bool(T)>;
template<typename T>
using DatarefMonitorChangedCallback = std::function<void(T)>;

struct TaggedCallback {
        void *owner = nullptr;
        DatarefShouldChangeCallback<DataRefValueType> func;
};

struct BoundRef {
        XPLMDataRef handle = nullptr;
        void *valuePointer = nullptr;
        std::vector<TaggedCallback> changeCallbacks;
};

typedef std::function<void(XPLMCommandPhase inPhase)> CommandExecutedCallback;

struct TaggedCommandCallback {
        void *owner = nullptr;
        CommandExecutedCallback func;
};

struct BoundCommand {
        XPLMCommandRef handle = nullptr;
        std::vector<TaggedCommandCallback> callbacks;
};

// Lets find() take a const char * without allocating a temporary std::string.
struct DatarefNameHash {
        using is_transparent = void;
        size_t operator()(std::string_view name) const noexcept {
            return std::hash<std::string_view>{}(name);
        }
};

template<typename V>
using DatarefMap = std::unordered_map<std::string, V, DatarefNameHash, std::equal_to<>>;

struct ResolvedRef {
        XPLMDataRef handle = nullptr;
        XPLMDataTypeID types = 0;
};

struct CachedValue {
        DataRefValueType value;
        int lastUpdateCycleNumber = 0;
        int lastPollCycleNumber = -1;
        ResolvedRef resolved;
        // Pulled by its owning product at render cadence instead of by update().
        // Recomputed from boundRefs on every pull, never latched.
        bool displayOnly = false;
};

class Dataref {
    private:
        Dataref();
        ~Dataref();
        static Dataref *instance;
        DatarefMap<BoundRef> boundRefs;
        DatarefMap<BoundCommand> boundCommands;
        DatarefMap<ResolvedRef> refs;
        DatarefMap<CachedValue> cachedValues;
        const ResolvedRef *findRef(std::string_view ref);
        // Never fires callbacks; the caller decides when it is safe to dispatch.
        bool pollCachedValue(const std::string &name, CachedValue &entry, int cycle);
        template<typename T>
        T readValue(const ResolvedRef &resolved);
        std::thread::id mainThreadId;
        std::mutex taskQueueMutex;
        std::vector<std::function<void()>> taskQueue;
        void drainMainThreadQueue();

        // Sized past the largest CDU display buffer so the fallback size query
        // in readValue() effectively never runs.
        static constexpr int kScratchBytes = 2048;
        static constexpr int kScratchElements = 512;
        char scratchBytes[kScratchBytes];
        int scratchInts[kScratchElements];
        float scratchFloats[kScratchElements];

        // Deque, not vector: a nested dispatch grows this while an outer level
        // still holds a reference into it.
        std::deque<std::vector<TaggedCallback>> callbackPool;
        size_t callbackDepth = 0;

    public:
        static Dataref *getInstance();

        template<typename T>
        void monitorExistingDataref(const char *ref, DatarefMonitorChangedCallback<T> callback, void *owner = nullptr);
        template<typename T>
        void createDataref(
            const char *ref, T *value, bool writable = false, DatarefShouldChangeCallback<T> changeCallback = nullptr);
        void bindExistingCommand(const char *command, CommandExecutedCallback callback, void *owner = nullptr);
        void createCommand(const char *command, const char *description, CommandExecutedCallback callback);
        void unbind(const char *ref);
        void unbindAll(void *owner);
        void destroyAllBindings();
        int _commandCallback(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void *inRefcon);

        void update();
        // Call immediately before reading a display list. Refs that also carry a
        // monitor are left to update() and keep frame cadence.
        void pollDisplayDatarefs(const std::vector<std::string> &refsToPoll);
        bool exists(const char *ref);
        void executeChangedCallbacksForDataref(const char *ref);
        int getCachedLastUpdate(const char *ref);
        template<typename T>
        T getCached(const char *ref);
        template<typename T>
        T get(const char *ref);
        template<typename T>
        void set(const char *ref, T value, bool setCacheOnly = false);

        void executeCommand(const char *command, XPLMCommandPhase phase = -1);

        void clearCache();
};

#endif
