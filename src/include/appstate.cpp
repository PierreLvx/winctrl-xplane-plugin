#include "appstate.h"

#include "config.h"
#include "dataref.h"
#include "path.hpp"
#include "power-scheme.h"
#include "SimpleIni.h"
#include "usbcontroller.h"
#include "usbdevice.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <system_error>
#include <XPLMProcessing.h>

AppState *AppState::instance = nullptr;

AppState::AppState() {
    pluginInitialized = false;
}

AppState::~AppState() {
    instance = nullptr;
}

AppState *AppState::getInstance() {
    if (instance == nullptr) {
        instance = new AppState();
    }

    return instance;
}

bool AppState::initialize() {
    if (pluginInitialized) {
        return false;
    }

    XPLMRegisterFlightLoopCallback(AppState::Update, REFRESH_INTERVAL_SECONDS_FAST, nullptr);

    WindowsPowerScheme::enableHighPerformance();

    pluginInitialized = true;

#ifdef DEBUG
    Dataref::getInstance()->createCommand(
        PRODUCT_NAME "/debug/disconnect_all_devices", "Disconnects all devices", [this](XPLMCommandPhase inPhase) {
            if (inPhase != xplm_CommandBegin) {
                return;
            }

            Logger::getInstance()->info("Disconnecting all devices via debug command...\n");
            USBController::getInstance()->disconnectAllDevices();
        });
#endif

    Logger::getInstance()->info("Plugin initialized.\n");
    return true;
}

void AppState::deinitialize() {
    if (!pluginInitialized) {
        return;
    }

    Logger::getInstance()->info("Plugin deinitializing...\n");

    WindowsPowerScheme::restorePrevious();

    XPLMUnregisterFlightLoopCallback(AppState::Update, nullptr);

    // destroy() resets the singleton pointer but does not free the object;
    // delete it here or it leaks once per enable/disable cycle. Safe because
    // the destructor's own destroy() call is idempotent and the task queue is
    // cleared below before any queued lambda capturing the controller can run.
    USBController *usbController = USBController::getInstance();
    usbController->destroy();
    delete usbController;

    Dataref::getInstance()->destroyAllBindings();

    pluginInitialized = false;

    {
        std::lock_guard<std::mutex> lock(taskQueueMutex);
        taskQueue.clear();
    }

    instance = nullptr;
}

float AppState::Update(float inElapsedSinceLastCall, float inElapsedTimeSinceLastFlightLoop, int inCounter, void *inRefcon) {
    auto appstate = AppState::getInstance();

    appstate->update();

    if (!USBController::getInstance()->anyProfileReady()) {
        return REFRESH_INTERVAL_SECONDS_SLOW;
    }

    return REFRESH_INTERVAL_SECONDS_FAST;
}

void AppState::update() {
    auto now = std::chrono::steady_clock::now();

    // Collect ready tasks under the lock, leaving non-ready tasks in the queue.
    // Executing outside the lock lets callbacks safely call executeAfter without
    // risk of reallocation invalidating the functor currently on the call stack.
    std::vector<DelayedTask> readyTasks;
    {
        std::lock_guard<std::mutex> lock(taskQueueMutex);
        cancelledOwners.clear();
        std::vector<DelayedTask> remaining;
        remaining.reserve(taskQueue.size());
        for (auto &task : taskQueue) {
            if (now >= task.runAt) {
                readyTasks.push_back(std::move(task));
            } else {
                remaining.push_back(std::move(task));
            }
        }
        taskQueue = std::move(remaining);
    }

    for (auto &task : readyTasks) {
        if (!task.func) {
            continue;
        }

        // An earlier task in this batch may have destroyed this task's owner
        // (e.g. a deferred device deletion); cancelTasksForOwner records the
        // owner so tasks already extracted into the batch are skipped too.
        if (task.owner) {
            std::lock_guard<std::mutex> lock(taskQueueMutex);
            if (std::find(cancelledOwners.begin(), cancelledOwners.end(), task.owner) != cancelledOwners.end()) {
                continue;
            }
        }

        task.func();
    }

    if (!pluginInitialized) {
        return;
    }

    Dataref::getInstance()->update();

    for (auto *device : USBController::getInstance()->devices) {
        device->update();
    }
}

void AppState::executeAfter(int milliseconds, void *owner, std::function<void()> func) {
    std::lock_guard<std::mutex> lock(taskQueueMutex);
    taskQueue.push_back({"", owner, std::chrono::steady_clock::now() + std::chrono::milliseconds(milliseconds), func});
}

void AppState::executeAfterDebounced(std::string taskName, int milliseconds, void *owner, std::function<void()> func) {
    auto now = std::chrono::steady_clock::now();
    std::lock_guard<std::mutex> lock(taskQueueMutex);
    auto it = std::find_if(taskQueue.begin(), taskQueue.end(), [&](const DelayedTask &t) {
        return t.owner == owner && t.name == taskName;
    });

    if (it != taskQueue.end()) {
        it->runAt = now + std::chrono::milliseconds(milliseconds);
        it->owner = owner;
        it->func = func;
    } else {
        taskQueue.push_back({taskName, owner, now + std::chrono::milliseconds(milliseconds), func});
    }
}

void AppState::cancelTasksForOwner(void *owner) {
    if (!owner) {
        return;
    }

    std::lock_guard<std::mutex> lock(taskQueueMutex);
    taskQueue.erase(
        std::remove_if(taskQueue.begin(), taskQueue.end(),
            [owner](const DelayedTask &t) {
                return t.owner == owner;
            }),
        taskQueue.end());
    cancelledOwners.push_back(owner);
}

// Looks up a key in one INI file. Returns false when the file or the key is absent,
// which is what lets the caller fall through to the legacy location.
static bool readPreferenceFromFile(const std::string &path, const std::string &key, std::string &outValue) {
    CSimpleIniA ini;
    ini.SetUnicode();
    if (ini.LoadFile(utf8ToPath(path).c_str()) < 0) {
        return false;
    }

    const char *value = ini.GetValue("Preferences", key.c_str(), nullptr);
    if (!value) {
        return false;
    }

    outValue = value;
    return true;
}

std::string AppState::readPreference(const std::string &key, const std::string &defaultValue) {
    migrateLegacyPreferences();

    std::string value;
    if (readPreferenceFromFile(getPreferencesFilePath(), key, value)) {
        return value;
    }

    // Legacy fallback for installs whose migration could not complete (read-only
    // Output directory, for instance). Safe to delete along with the migration.
    if (readPreferenceFromFile(getLegacyPreferencesFilePath(), key, value)) {
        return value;
    }

    return defaultValue;
}

void AppState::writePreference(const std::string &key, const std::string &value) {
    migrateLegacyPreferences();

    const std::filesystem::path path = utf8ToPath(getPreferencesFilePath());

    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);

    CSimpleIniA ini;
    ini.SetUnicode();
    ini.LoadFile(path.c_str()); // File might not exist yet, continue

    ini.SetValue("Preferences", key.c_str(), value.c_str());

    if (ini.SaveFile(path.c_str()) < 0) {
        Logger::getInstance()->info("Failed to save preferences file.\n");
    }
}

// Moves <plugin>/preferences.ini to <X-Plane>/Output/preferences/winctrl.ini once.
// Never overwrites an existing new-location file: that one is authoritative.
void AppState::migrateLegacyPreferences() {
    static std::once_flag migrationFlag;
    std::call_once(migrationFlag, [this]() {
        const std::filesystem::path legacyPath = utf8ToPath(getLegacyPreferencesFilePath());
        const std::filesystem::path newPath = utf8ToPath(getPreferencesFilePath());

        std::error_code ec;
        if (!std::filesystem::exists(legacyPath, ec) || std::filesystem::exists(newPath, ec)) {
            return;
        }

        std::filesystem::create_directories(newPath.parent_path(), ec);

        std::filesystem::rename(legacyPath, newPath, ec);
        if (ec) {
            // Rename fails across volumes; fall back to copy + remove.
            ec.clear();
            std::filesystem::copy_file(legacyPath, newPath, ec);
            if (ec) {
                Logger::getInstance()->info("Failed to migrate preferences to %s: %s\n", pathToUtf8(newPath).c_str(), ec.message().c_str());
                return;
            }

            std::filesystem::remove(legacyPath, ec);
        }

        Logger::getInstance()->info("Migrated preferences to %s\n", pathToUtf8(newPath).c_str());
    });
}

std::string AppState::getPreferencesFilePath() {
    return getXPlaneDirectory() + "/Output/preferences/" PRODUCT_NAME ".ini";
}

std::string AppState::getLegacyPreferencesFilePath() {
    return getPluginDirectory() + "/preferences.ini";
}

std::string AppState::getPluginDirectory() {
    return getXPlaneDirectory() + PLUGIN_DIRECTORY;
}

std::string AppState::getXPlaneDirectory() {
    char systemPath[512];
    XPLMGetSystemPath(systemPath);
    std::string rootDirectory = systemPath;
    while (rootDirectory.ends_with("/") || rootDirectory.ends_with("\\")) {
        rootDirectory = rootDirectory.substr(0, rootDirectory.length() - 1);
    }

    return rootDirectory;
}
