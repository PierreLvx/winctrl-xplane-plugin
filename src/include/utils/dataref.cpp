#include "dataref.h"

#include "appstate.h"
#include "config.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <XPLMDisplay.h>
#include <XPLMProcessing.h>
#include <XPLMUtilities.h>

using namespace std;

Dataref *Dataref::instance = nullptr;

int handleCommandCallback(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void *inRefcon) {
    return Dataref::getInstance()->_commandCallback(inCommand, inPhase, inRefcon);
}

Dataref::Dataref() {
    cachedValues = {};
    refs = {};
    mainThreadId = std::this_thread::get_id();
}

Dataref::~Dataref() {
    instance = nullptr;
}

Dataref *Dataref::getInstance() {
    if (instance == nullptr) {
        instance = new Dataref();
    }

    return instance;
}

template void Dataref::createDataref<int>(
    const char *ref, int *value, bool writable = false, DatarefShouldChangeCallback<int> changeCallback = nullptr);
template void Dataref::createDataref<bool>(
    const char *ref, bool *value, bool writable = false, DatarefShouldChangeCallback<bool> changeCallback = nullptr);
template void Dataref::createDataref<float>(
    const char *ref, float *value, bool writable = false, DatarefShouldChangeCallback<float> changeCallback = nullptr);
template void Dataref::createDataref<double>(const char *ref,
    double *value,
    bool writable = false,
    DatarefShouldChangeCallback<double> changeCallback = nullptr);
template void Dataref::createDataref<std::string>(const char *ref,
    std::string *value,
    bool writable = false,
    DatarefShouldChangeCallback<std::string> changeCallback = nullptr);

template<typename T>
void Dataref::createDataref(const char *ref, T *value, bool writable, DatarefShouldChangeCallback<T> changeCallback) {
    unbind(ref);

    XPLMDataRef handle = nullptr;
    boundRefs[ref] = {handle, value, {{nullptr, [changeCallback](DataRefValueType newValue) -> bool {
                                           if (!changeCallback) {
                                               return true;
                                           } else if constexpr (std::is_same_v<T, std::string>) {
                                               if (std::holds_alternative<std::string>(newValue)) {
                                                   return changeCallback(std::get<std::string>(newValue));
                                               }
                                           } else if constexpr (std::is_same_v<T, int> || std::is_same_v<T, bool>) {
                                               if (std::holds_alternative<int>(newValue)) {
                                                   return changeCallback(std::get<int>(newValue));
                                               }
                                           } else if constexpr (std::is_same_v<T, float>) {
                                               if (std::holds_alternative<float>(newValue)) {
                                                   return changeCallback(std::get<float>(newValue));
                                               }
                                           } else if constexpr (std::is_same_v<T, double>) {
                                               if (std::holds_alternative<double>(newValue)) {
                                                   return changeCallback(std::get<double>(newValue));
                                               }
                                           }
                                           return false;
                                       }}}};

    if constexpr ((std::is_same_v<T, int>) || (std::is_same_v<T, bool>) ) {
        handle = XPLMRegisterDataAccessor(
            ref,
            xplmType_Int,
            writable ? 1 : 0,
            [](void *inRefcon) -> int {
                return *static_cast<T *>(inRefcon);
            },
            [](void *inRefcon, int inValue) {
                BoundRef *info = static_cast<BoundRef *>(inRefcon);
                T *valuePtr = static_cast<T *>(info->valuePointer);

                if (info->changeCallbacks.size()) {
                    if (info->changeCallbacks[0].func(inValue)) {
                        *valuePtr = inValue;
                    }
                } else {
                    *valuePtr = inValue;
                }
            },
            nullptr,
            nullptr, // Float
            nullptr,
            nullptr, // Double
            nullptr,
            nullptr, // Int array
            nullptr,
            nullptr, // Float array
            nullptr,
            nullptr,          // Binary
            value,            // Read refcon
            &boundRefs[ref]); // Write refcon
    } else if constexpr (std::is_same_v<T, float>) {
        handle = XPLMRegisterDataAccessor(
            ref,
            xplmType_Float,
            writable ? 1 : 0,
            nullptr,
            nullptr, // Int
            [](void *inRefcon) -> T {
                return *static_cast<T *>(inRefcon);
            },
            [](void *inRefcon, T inValue) {
                BoundRef *info = static_cast<BoundRef *>(inRefcon);
                T *valuePtr = static_cast<T *>(info->valuePointer);

                if (info->changeCallbacks.size()) {
                    if (info->changeCallbacks[0].func(inValue)) {
                        *valuePtr = inValue;
                    }
                } else {
                    *valuePtr = inValue;
                }
            },
            nullptr,
            nullptr, // Double
            nullptr,
            nullptr, // Int array
            nullptr,
            nullptr, // Float array
            nullptr,
            nullptr,          // Binary
            value,            // Read refcon
            &boundRefs[ref]); // Write refcon
    } else if constexpr (std::is_same_v<T, double>) {
        handle = XPLMRegisterDataAccessor(
            ref,
            xplmType_Double,
            writable ? 1 : 0,
            nullptr,
            nullptr, // Int
            nullptr,
            nullptr, // Float
            [](void *inRefcon) -> T {
                return *static_cast<T *>(inRefcon);
            },
            [](void *inRefcon, T inValue) {
                BoundRef *info = static_cast<BoundRef *>(inRefcon);
                T *valuePtr = static_cast<T *>(info->valuePointer);

                if (info->changeCallbacks.size()) {
                    if (info->changeCallbacks[0].func(inValue)) {
                        *valuePtr = inValue;
                    }
                } else {
                    *valuePtr = inValue;
                }
            },
            nullptr,
            nullptr, // Int array
            nullptr,
            nullptr, // Float array
            nullptr,
            nullptr,          // Binary
            value,            // Read refcon
            &boundRefs[ref]); // Write refcon
    } else if constexpr (std::is_same_v<T, std::string>) {
        handle = XPLMRegisterDataAccessor(
            ref,
            xplmType_Data,
            writable ? 1 : 0,
            nullptr,
            nullptr, // Int
            nullptr,
            nullptr, // Float
            nullptr,
            nullptr, // Double
            nullptr,
            nullptr, // Int array
            nullptr,
            nullptr, // Float array
            [](void *inRefcon, void *outValue, int inOffset, int inMaxLength) -> int {
                T value = *static_cast<T *>(inRefcon);
                // SDK contract: NULL buffer means "return the total size"
                if (!outValue) {
                    return static_cast<int>(value.length());
                }
                if (inOffset < 0 || inOffset >= static_cast<int>(value.length())) {
                    return 0;
                }
                int copied = std::min(inMaxLength, static_cast<int>(value.length()) - inOffset);
                memcpy(outValue, value.c_str() + inOffset, copied);
                return copied;
            },
            [](void *inRefcon, void *inValue, int inOffset, int inMaxLength) {
                BoundRef *info = static_cast<BoundRef *>(inRefcon);
                T *valuePtr = static_cast<T *>(info->valuePointer);

                if (info->changeCallbacks.size()) {
                    std::string newValue = std::string(static_cast<const char *>(inValue));
                    if (info->changeCallbacks[0].func(newValue)) {
                        *valuePtr = (const char *) inValue;
                    }
                } else {
                    *valuePtr = (const char *) inValue;
                }
            },
            value,            // Read refcon
            &boundRefs[ref]); // Write refcon
    }

    boundRefs[ref].handle = handle;
}

template void Dataref::monitorExistingDataref<int>(const char *ref, DatarefMonitorChangedCallback<int> changeCallback, void *owner);
template void Dataref::monitorExistingDataref<bool>(
    const char *ref, DatarefMonitorChangedCallback<bool> changeCallback, void *owner);
template void Dataref::monitorExistingDataref<float>(
    const char *ref, DatarefMonitorChangedCallback<float> changeCallback, void *owner);
template void Dataref::monitorExistingDataref<double>(
    const char *ref, DatarefMonitorChangedCallback<double> changeCallback, void *owner);
template void Dataref::monitorExistingDataref<std::string>(
    const char *ref, DatarefMonitorChangedCallback<std::string> changeCallback, void *owner);
template void Dataref::monitorExistingDataref<std::vector<float>>(
    const char *ref, DatarefMonitorChangedCallback<std::vector<float>> changeCallback, void *owner);
template void Dataref::monitorExistingDataref<std::vector<int>>(
    const char *ref, DatarefMonitorChangedCallback<std::vector<int>> changeCallback, void *owner);

template<typename T>
void Dataref::monitorExistingDataref(const char *ref, DatarefMonitorChangedCallback<T> changeCallback, void *owner) {
    // Prime the cache with a default so update() starts polling this ref and
    // delivers the live value to every subscriber on the next tick. Do not
    // write the cache through set(): that fired all existing subscribers with
    // a fabricated default (blanking LEDs and crashing vector callbacks that
    // index an empty array), and bailed out entirely for datarefs the
    // aircraft plugin has not registered yet, so those monitors never fired.
    cachedValues[ref] = {.value = T{}, .lastUpdateCycleNumber = XPLMGetCycleNumber()};

    auto callback = [changeCallback](DataRefValueType newValue) -> bool {
        if constexpr (std::is_same_v<T, bool>) {
            if (std::holds_alternative<int>(newValue)) {
                changeCallback(std::get<int>(newValue));
            } else if (std::holds_alternative<bool>(newValue)) {
                changeCallback(std::get<bool>(newValue));
            } else if (std::holds_alternative<float>(newValue)) {
                changeCallback(std::get<float>(newValue));
            } else if (std::holds_alternative<double>(newValue)) {
                changeCallback(std::get<double>(newValue));
            }
        } else {
            if (std::holds_alternative<T>(newValue)) {
                changeCallback(std::get<T>(newValue));
            }
        }

        return false;
    };

    if (boundRefs.find(ref) != boundRefs.end()) {
        boundRefs[ref].changeCallbacks.push_back({owner, callback});
    } else {
        boundRefs[ref] = {0, nullptr, {{owner, callback}}};
    }
}

void Dataref::destroyAllBindings() {
    for (auto &[key, ref] : boundRefs) {
        // Monitor-only entries have no accessor registered
        if (ref.handle) {
            XPLMUnregisterDataAccessor(ref.handle);
        }
    }
    boundRefs.clear();

    for (auto &[key, ref] : boundCommands) {
        XPLMUnregisterCommandHandler(ref.handle, handleCommandCallback, 1, nullptr);
    }
    boundCommands.clear();
}

void Dataref::unbind(const char *ref) {
    auto it = boundRefs.find(ref);
    if (it != boundRefs.end()) {
        if (it->second.handle) {
            XPLMUnregisterDataAccessor(it->second.handle);
        }
        boundRefs.erase(it);
    }

    auto it2 = boundCommands.find(ref);
    if (it2 != boundCommands.end()) {
        XPLMUnregisterCommandHandler(it2->second.handle, handleCommandCallback, 1, nullptr);
        boundCommands.erase(it2);
    }

    refs.erase(ref);
    cachedValues.erase(ref);
}

void Dataref::clearCache() {
    cachedValues.clear();
    // Cached XPLMDataRef handles of an unloaded aircraft plugin are stale;
    // drop them so the next access re-resolves against the new aircraft.
    refs.clear();
}

void Dataref::drainMainThreadQueue() {
    std::vector<std::function<void()>> tasks;
    {
        std::lock_guard<std::mutex> lock(taskQueueMutex);
        tasks.swap(taskQueue);
    }
    for (auto &task : tasks) {
        task();
    }
}

// Above the largest interval getDisplayUpdateFrameInterval can return, so it
// only ever fires for entries whose product stopped pulling them.
static constexpr int kOrphanedDisplayRefCycles = 120;

bool Dataref::pollCachedValue(const std::string &name, CachedValue &entry, int cycle) {
    entry.lastPollCycleNumber = cycle;

    if (!entry.resolved.handle) {
        const ResolvedRef *resolved = findRef(name);
        if (!resolved) {
            return false;
        }
        entry.resolved = *resolved;
    }

    bool didChange = false;
    std::visit(
        [&](auto &&value) {
            using T = std::decay_t<decltype(value)>;
            T newValue = readValue<T>(entry.resolved);
            if constexpr (std::is_floating_point_v<T>) {
                didChange = std::fabs(value - newValue) > std::numeric_limits<T>::epsilon();
            } else {
                didChange = value != newValue;
            }

            if (didChange) {
                // Through the reference, so a string or vector reuses its capacity
                value = std::move(newValue);
                entry.lastUpdateCycleNumber = cycle;
            }
        },
        entry.value);

    return didChange;
}

void Dataref::update() {
    drainMainThreadQueue();

    int cycle = XPLMGetCycleNumber();
    std::vector<std::string> changed;

    for (auto &[key, data] : cachedValues) {
        // Nothing clears the claim when a product unloads its profile or its
        // device is unplugged, so an unpulled entry falls back to slow polling
        // here instead of freezing.
        if (data.displayOnly && cycle - data.lastPollCycleNumber < kOrphanedDisplayRefCycles) {
            continue;
        }

        if (pollCachedValue(key, data, cycle)) {
            changed.push_back(key);
        }
    }

    for (const std::string &key : changed) {
        // Unbound by a callback earlier in this loop; don't resurrect it
        if (cachedValues.find(key) == cachedValues.end()) {
            continue;
        }
        executeChangedCallbacksForDataref(key.c_str());
    }
}

void Dataref::pollDisplayDatarefs(const std::vector<std::string> &refsToPoll) {
    int cycle = XPLMGetCycleNumber();
    for (const std::string &name : refsToPoll) {
        auto it = cachedValues.find(name);
        if (it == cachedValues.end()) {
            continue;
        }

        CachedValue &entry = it->second;
        entry.displayOnly = boundRefs.find(name) == boundRefs.end();
        if (!entry.displayOnly || entry.lastPollCycleNumber == cycle) {
            continue;
        }

        pollCachedValue(name, entry, cycle);
    }
}

const ResolvedRef *Dataref::findRef(std::string_view ref) {
    auto it = refs.find(ref);
    if (it != refs.end()) {
        return &it->second;
    }

    // Only materialised on the miss path, where XPLMFindDataRef needs it
    // null-terminated.
    std::string name(ref);
    XPLMDataRef handle = XPLMFindDataRef(name.c_str());
    if (!handle) {
        return nullptr;
    }

    ResolvedRef resolved{handle, XPLMGetDataRefTypes(handle)};
    return &refs.emplace(std::move(name), resolved).first->second;
}

bool Dataref::exists(const char *ref) {
    return XPLMFindDataRef(ref) != nullptr;
}

void Dataref::executeChangedCallbacksForDataref(const char *ref) {
    auto it = boundRefs.find(ref);
    if (it == boundRefs.end()) {
        return;
    }

    auto cacheIt = cachedValues.find(ref);
    if (cacheIt == cachedValues.end()) {
        // No cached value to deliver; operator[] here used to default-insert
        // a float{0} entry that was then polled forever with the wrong type.
        return;
    }

    // Iterate a copy: a callback may register or unbind monitors on this ref,
    // which would invalidate the live vector mid-iteration.
    if (callbackDepth >= callbackPool.size()) {
        callbackPool.resize(callbackDepth + 1);
    }

    std::vector<TaggedCallback> &callbacks = callbackPool[callbackDepth];
    callbacks.assign(it->second.changeCallbacks.begin(), it->second.changeCallbacks.end());
    DataRefValueType value = cacheIt->second.value;

    struct DepthGuard {
            size_t &depth;
            std::vector<TaggedCallback> &slot;
            ~DepthGuard() {
                depth--;
                slot.clear();
            }
    } guard{callbackDepth, callbacks};
    callbackDepth++;

    for (auto &tc : callbacks) {
        tc.func(value);
    }
}

void Dataref::unbindAll(void *owner) {
    for (auto it = boundRefs.begin(); it != boundRefs.end();) {
        auto &cbs = it->second.changeCallbacks;
        cbs.erase(std::remove_if(cbs.begin(), cbs.end(),
                      [owner](const TaggedCallback &tc) {
                          return tc.owner == owner;
                      }),
            cbs.end());
        // Remove monitor-only entries that now have no callbacks, including
        // their cache entries, so update() stops polling them and aircraft
        // switches don't accrete dead refs.
        if (cbs.empty() && !it->second.handle) {
            cachedValues.erase(it->first);
            refs.erase(it->first);
            it = boundRefs.erase(it);
        } else {
            ++it;
        }
    }

    for (auto it = boundCommands.begin(); it != boundCommands.end();) {
        auto &cbs = it->second.callbacks;
        cbs.erase(std::remove_if(cbs.begin(), cbs.end(),
                      [owner](const TaggedCommandCallback &tc) {
                          return tc.owner == owner;
                      }),
            cbs.end());
        if (cbs.empty()) {
            XPLMUnregisterCommandHandler(it->second.handle, handleCommandCallback, 1, nullptr);
            it = boundCommands.erase(it);
        } else {
            ++it;
        }
    }
}

int Dataref::getCachedLastUpdate(const char *ref) {
    auto it = cachedValues.find(ref);
    if (it == cachedValues.end()) {
        return 0;
    }

    return it->second.lastUpdateCycleNumber;
}

template float Dataref::getCached<float>(const char *ref);
template double Dataref::getCached<double>(const char *ref);
template int Dataref::getCached<int>(const char *ref);
template bool Dataref::getCached<bool>(const char *ref);
template std::vector<int> Dataref::getCached<std::vector<int>>(const char *ref);
template std::vector<float> Dataref::getCached<std::vector<float>>(const char *ref);
template std::vector<unsigned char> Dataref::getCached<std::vector<unsigned char>>(const char *ref);
template std::string Dataref::getCached<std::string>(const char *ref);

template<typename T>
T Dataref::getCached(const char *ref) {
    auto it = cachedValues.find(ref);
    if (it == cachedValues.end()) {
        auto val = get<T>(ref);
        CachedValue entry{.value = val, .lastUpdateCycleNumber = XPLMGetCycleNumber()};
        if (const ResolvedRef *resolved = findRef(ref)) {
            entry.resolved = *resolved;
        }
        cachedValues.emplace(std::string(ref), std::move(entry));
        return val;
    }

    if (!std::holds_alternative<T>(it->second.value)) {
        if constexpr (std::is_same_v<T, bool>) {
            if (std::holds_alternative<int>(it->second.value)) {
                return std::get<int>(it->second.value) > 0;
            } else if (std::holds_alternative<double>(it->second.value)) {
                return std::get<double>(it->second.value) > std::numeric_limits<double>::epsilon();
            } else if (std::holds_alternative<float>(it->second.value)) {
                return std::get<float>(it->second.value) > std::numeric_limits<float>::epsilon();
            }

            return false;
        } else if constexpr (std::is_same_v<T, std::string>) {
            return "";
        } else if constexpr (std::is_same_v<T, std::vector<int>> || std::is_same_v<T, std::vector<float>> ||
                             std::is_same_v<T, std::vector<unsigned char>>) {
            return {};
        } else {
            return 0;
        }
    }

    return std::get<T>(it->second.value);
}

template float Dataref::get<float>(const char *ref);
template double Dataref::get<double>(const char *ref);
template int Dataref::get<int>(const char *ref);
template bool Dataref::get<bool>(const char *ref);
template std::vector<int> Dataref::get<std::vector<int>>(const char *ref);
template std::vector<float> Dataref::get<std::vector<float>>(const char *ref);
template std::vector<unsigned char> Dataref::get<std::vector<unsigned char>>(const char *ref);
template std::string Dataref::get<std::string>(const char *ref);

template<typename T>
T Dataref::get(const char *ref) {
    if (std::this_thread::get_id() != mainThreadId) {
        std::promise<T> promise;
        auto future = promise.get_future();
        std::string refStr(ref);
        {
            std::lock_guard<std::mutex> lock(taskQueueMutex);
            taskQueue.push_back([this, refStr, &promise]() {
                try {
                    promise.set_value(get<T>(refStr.c_str()));
                } catch (...) {
                    promise.set_exception(std::current_exception());
                }
            });
        }
        return future.get();
    }

    const ResolvedRef *resolved = findRef(ref);
    if (!resolved) {
        if constexpr (std::is_same_v<T, std::string>) {
            return "";
        } else if constexpr (std::is_same_v<T, std::vector<int>> || std::is_same_v<T, std::vector<float>> ||
                             std::is_same_v<T, std::vector<unsigned char>>) {
            return {};
        } else {
            return 0;
        }
    }

    return readValue<T>(*resolved);
}

template float Dataref::readValue<float>(const ResolvedRef &resolved);
template double Dataref::readValue<double>(const ResolvedRef &resolved);
template int Dataref::readValue<int>(const ResolvedRef &resolved);
template bool Dataref::readValue<bool>(const ResolvedRef &resolved);
template std::vector<int> Dataref::readValue<std::vector<int>>(const ResolvedRef &resolved);
template std::vector<float> Dataref::readValue<std::vector<float>>(const ResolvedRef &resolved);
template std::vector<unsigned char> Dataref::readValue<std::vector<unsigned char>>(const ResolvedRef &resolved);
template std::string Dataref::readValue<std::string>(const ResolvedRef &resolved);

// A result that exactly fills the scratch buffer means the ref may be longer,
// which is the only case that pays for the size query.
template<typename T>
T Dataref::readValue(const ResolvedRef &resolved) {
    XPLMDataRef handle = resolved.handle;

    if constexpr (std::is_same_v<T, bool>) {
        if ((resolved.types & xplmType_Float) == xplmType_Float) {
            return XPLMGetDataf(handle) > std::numeric_limits<float>::epsilon();
        } else if ((resolved.types & xplmType_Double) == xplmType_Double) {
            return XPLMGetDatad(handle) > std::numeric_limits<double>::epsilon();
        } else {
            return XPLMGetDatai(handle) > 0;
        }
    } else if constexpr (std::is_same_v<T, int> || std::is_same_v<T, float> || std::is_same_v<T, double>) {
        if ((resolved.types & xplmType_Float) == xplmType_Float) {
            return XPLMGetDataf(handle);
        } else if ((resolved.types & xplmType_Double) == xplmType_Double) {
            return XPLMGetDatad(handle);
        } else {
            return XPLMGetDatai(handle);
        }
    } else if constexpr (std::is_same_v<T, std::vector<int>>) {
        int copied = XPLMGetDatavi(handle, scratchInts, 0, kScratchElements);
        if (copied <= 0) {
            return {};
        } else if (copied < kScratchElements) {
            return std::vector<int>(scratchInts, scratchInts + copied);
        }

        int size = XPLMGetDatavi(handle, nullptr, 0, 0);
        std::vector<int> outValues(size);
        XPLMGetDatavi(handle, outValues.data(), 0, size);
        return outValues;
    } else if constexpr (std::is_same_v<T, std::vector<float>>) {
        int copied = XPLMGetDatavf(handle, scratchFloats, 0, kScratchElements);
        if (copied <= 0) {
            return {};
        } else if (copied < kScratchElements) {
            return std::vector<float>(scratchFloats, scratchFloats + copied);
        }

        int size = XPLMGetDatavf(handle, nullptr, 0, 0);
        std::vector<float> outValues(size);
        XPLMGetDatavf(handle, outValues.data(), 0, size);
        return outValues;
    } else if constexpr (std::is_same_v<T, std::vector<unsigned char>>) {
        int copied = XPLMGetDatab(handle, scratchBytes, 0, kScratchBytes);
        if (copied <= 0) {
            return {};
        } else if (copied < kScratchBytes) {
            auto *bytes = reinterpret_cast<unsigned char *>(scratchBytes);
            return std::vector<unsigned char>(bytes, bytes + copied);
        }

        int size = XPLMGetDatab(handle, nullptr, 0, 0);
        std::vector<unsigned char> outValues(size);
        XPLMGetDatab(handle, outValues.data(), 0, size);
        return outValues;
    } else if constexpr (std::is_same_v<T, std::string>) {
        int copied = XPLMGetDatab(handle, scratchBytes, 0, kScratchBytes);
        if (copied <= 0) {
            return "";
        } else if (copied < kScratchBytes) {
            auto *nul = static_cast<const char *>(memchr(scratchBytes, '\0', copied));
            return std::string(scratchBytes, nul ? nul - scratchBytes : copied);
        }

        int size = XPLMGetDatab(handle, nullptr, 0, 0);
        std::vector<char> str(size);
        XPLMGetDatab(handle, str.data(), 0, size);
        auto it = std::find(str.begin(), str.end(), '\0');
        return std::string(str.begin(), it);
    }

    if constexpr (std::is_same_v<T, std::string>) {
        return "";
    } else if constexpr (std::is_same_v<T, std::vector<int>> || std::is_same_v<T, std::vector<float>> ||
                         std::is_same_v<T, std::vector<unsigned char>>) {
        return {};
    } else {
        return 0;
    }
}

template void Dataref::set<float>(const char *ref, float value, bool setCacheOnly);
template void Dataref::set<double>(const char *ref, double value, bool setCacheOnly);
template void Dataref::set<int>(const char *ref, int value, bool setCacheOnly);
template void Dataref::set<bool>(const char *ref, bool value, bool setCacheOnly);
template void Dataref::set<std::vector<int>>(const char *ref, std::vector<int> value, bool setCacheOnly);
template void Dataref::set<std::vector<float>>(const char *ref, std::vector<float> value, bool setCacheOnly);
template void Dataref::set<std::vector<unsigned char>>(
    const char *ref, std::vector<unsigned char> value, bool setCacheOnly);
template void Dataref::set<std::string>(const char *ref, std::string value, bool setCacheOnly);

template<typename T>
void Dataref::set(const char *ref, T value, bool setCacheOnly) {
    const ResolvedRef *resolved = findRef(ref);
    if (!resolved) {
        return;
    }

    // Copied: a callback below may unbind this ref and erase what resolved
    // points into.
    const ResolvedRef resolvedCopy = *resolved;
    XPLMDataRef handle = resolvedCopy.handle;

    // Updated in place so an existing entry keeps its resolved handle and claim
    CachedValue &entry = cachedValues[ref];
    entry.value = value;
    entry.lastUpdateCycleNumber = XPLMGetCycleNumber();
    if (!entry.resolved.handle) {
        entry.resolved = resolvedCopy;
    }

    executeChangedCallbacksForDataref(ref);

    if (setCacheOnly) {
        return;
    }

    if constexpr (std::is_same_v<T, bool> || std::is_same_v<T, int> || std::is_same_v<T, float> ||
                  std::is_same_v<T, double>) {
        XPLMDataTypeID refType = resolvedCopy.types;
        if ((refType & xplmType_Float) == xplmType_Float) {
            XPLMSetDataf(handle, value);
        } else if ((refType & xplmType_Double) == xplmType_Double) {
            XPLMSetDatad(handle, value);
        } else {
            XPLMSetDatai(handle, value);
        }
    } else if constexpr (std::is_same_v<T, std::vector<int>>) {
        XPLMSetDatavi(handle, const_cast<int *>(value.data()), 0, static_cast<int>(value.size()));
    } else if constexpr (std::is_same_v<T, std::vector<float>>) {
        XPLMSetDatavf(handle, const_cast<float *>(value.data()), 0, static_cast<int>(value.size()));
    } else if constexpr (std::is_same_v<T, std::vector<unsigned char>>) {
        XPLMSetDatab(handle, const_cast<unsigned char *>(value.data()), 0, static_cast<int>(value.size()));
    } else if constexpr (std::is_same_v<T, std::string>) {
        XPLMSetDatab(handle, (char *) value.c_str(), 0, (unsigned int) value.length());
    }
}

void Dataref::executeCommand(const char *command, XPLMCommandPhase phase) {
    XPLMCommandRef handle = XPLMFindCommand(command);
    if (!handle) {
        Logger::getInstance()->info("Command not found: %s\n", command);
        return;
    }

    if (phase == -1) {
        XPLMCommandOnce(handle);
    } else if (phase == xplm_CommandBegin) {
        XPLMCommandBegin(handle);
    } else if (phase == xplm_CommandEnd) {
        XPLMCommandEnd(handle);
    }
}

void Dataref::bindExistingCommand(const char *command, CommandExecutedCallback callback, void *owner) {
    XPLMCommandRef handle = XPLMFindCommand(command);
    if (!handle) {
        return;
    }

    auto it = boundCommands.find(command);
    if (it != boundCommands.end()) {
        it->second.callbacks.push_back({owner, callback});
        return;
    }

    boundCommands[command] = {handle, {{owner, callback}}};

    XPLMRegisterCommandHandler(handle, handleCommandCallback, 1, nullptr);
}

void Dataref::createCommand(const char *command, const char *description, CommandExecutedCallback callback) {
    XPLMCommandRef handle = XPLMCreateCommand(command, description);
    if (!handle) {
        return;
    }

    auto it = boundCommands.find(command);
    if (it != boundCommands.end()) {
        XPLMUnregisterCommandHandler(handle, handleCommandCallback, 1, nullptr);
        boundCommands.erase(it);
    }

    bindExistingCommand(command, callback);
}

int Dataref::_commandCallback(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void *inRefcon) {
    for (const auto &entry : boundCommands) {
        XPLMCommandRef handle = entry.second.handle;
        if (inCommand == handle) {
            // Iterate a copy: a callback may bind or unbind commands,
            // which would invalidate the live vector mid-iteration.
            std::vector<TaggedCommandCallback> callbacks = entry.second.callbacks;
            for (auto &tc : callbacks) {
                tc.func(inPhase);
            }
            break;
        }
    }

    return 1;
}
