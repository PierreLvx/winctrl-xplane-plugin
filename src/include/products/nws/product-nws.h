#ifndef PRODUCT_NWS_H
#define PRODUCT_NWS_H

#include "nws-aircraft-profile.h"
#include "usbdevice.h"

#include <cstdint>
#include <string>
#include <unordered_map>

enum class NWSLed : int {
    BACKLIGHT = 0,
};

class ProductNWS : public USBDevice {
    private:
        NWSAircraftProfile *profile = nullptr;
        int menuItemId = -1;
        std::unordered_map<int, uint8_t> lastLedBrightness;

        void setProfileForCurrentAircraft();

    public:
        ProductNWS(HIDDeviceHandle hidDevice, uint16_t vendorId, uint16_t productId, std::string vendorName, std::string productName);
        ~ProductNWS();

        static constexpr unsigned char IdentifierByte = 0x60;

        const char *classIdentifier() override;
        const char *activeProfileName() const override;
        bool connect() override;
        void blackout() override;

        void setLedBrightness(NWSLed led, uint8_t brightness);
};

#endif
