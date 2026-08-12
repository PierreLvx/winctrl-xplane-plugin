#ifndef PRODUCT_ECAM_H
#define PRODUCT_ECAM_H

#include "ecam-aircraft-profile.h"
#include "usbdevice.h"

#include <set>

enum class ECAMLed : int {
    BACKLIGHT = 0,
    OVERALL_LEDS_BRIGHTNESS = 1,
    //UNUSED = 2,
    EMER_CANC_BRIGHTNESS = 3,

    _START = 4,
    ENG = 4,
    BLEED = 5,
    PRESS = 6,
    ELEC = 7,
    HYD = 8,
    FUEL = 9,
    APU = 10,
    COND = 11,
    DOOR = 12,
    WHEEL = 13,
    F_CTL = 14,
    CLR_LEFT = 15,
    STS = 16,
    CLR_RIGHT = 17,
    _END = 17,
};

class ProductECAM : public USBDevice {
    private:
        ECAMAircraftProfile *profile = nullptr;
        int menuItemId = -1;
        uint64_t lastButtonStateLo = 0;
        uint32_t lastButtonStateHi = 0;
        std::set<int> pressedButtonIndices;

        void setProfileForCurrentAircraft();

    public:
        ProductECAM(HIDDeviceHandle hidDevice, uint16_t vendorId, uint16_t productId, std::string vendorName, std::string productName);
        ~ProductECAM();

        static constexpr unsigned char IdentifierByte = 0x70;

        const char *classIdentifier() override;

        const char *activeProfileName() const override;

        bool connect() override;
        void blackout() override;
        void didReceiveData(int reportId, uint8_t *report, int reportLength) override;
        void didReceiveButton(uint16_t hardwareButtonIndex, bool pressed, uint8_t count = 1) override;

        void setAllLedsEnabled(bool enabled);
        void setLedBrightness(ECAMLed led, uint8_t brightness);
};

#endif
