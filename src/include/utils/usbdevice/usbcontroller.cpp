#include "usbcontroller.h"

#include "appstate.h"

bool USBController::anyProfileReady() {
    for (auto &device : devices) {
        if (device->profileReady) {
            return true;
        }
    }

    return false;
}

void USBController::connectAllDevices() {
    AppState::getInstance()->executeAfter(0, this, [this]() {
        enumerateDevices();
    });
}

void USBController::releaseDisabledDevices() {
    std::lock_guard<std::mutex> lock(devicesMutex);
    for (auto it = devices.begin(); it != devices.end();) {
        USBDevice *device = *it;
        if (USBDevice::IsProductEnabled(device->productId)) {
            ++it;
            continue;
        }

        Logger::getInstance()->info("Releasing %s (productId: 0x%04X), it was switched off in the " FRIENDLY_NAME " menu\n", device->productName.c_str(), device->productId);
        device->blackout();
        device->disconnect();
        // Drops platform-side path/pending tracking, or the device could never
        // be re-added after being switched back on.
        forgetDevice(device);
        delete device;
        it = devices.erase(it);
    }
}

void USBController::disconnectAllDevices() {
    std::lock_guard<std::mutex> lock(devicesMutex);
    for (auto ptr : devices) {
        ptr->blackout();
        ptr->disconnect();
        // Drop platform-side path/pending tracking, otherwise the device is
        // considered still present and can never be re-added until reload.
        forgetDevice(ptr);
        delete ptr;
    }
    devices.clear();
}
