#include "a220-fcu-efis-profile.h"

#include "appstate.h"
#include "dataref.h"
#include "product-fcu-efis.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <XPLMUtilities.h>

// Deanarica A220-300 (SASL). The autoflight panel is driven by a220/autopilot/* commands
// that wrap the stock autopilot datarefs; the barometer knob commands are animation only,
// so pressure is set on the stock barometer dataref instead. The aircraft models a single
// baro state (both PFDs read barometer_setting_in_hg_pilot) and a single FD state.

static constexpr const char *kBaroPilot = "sim/cockpit2/gauges/actuators/barometer_setting_in_hg_pilot";
static constexpr const char *kBaroMode = "a220/barometer/mode";              // 0 = inHg, 1 = hPa
static constexpr const char *kAltStep = "sim/aircraft/autopilot/alt_step_ft"; // 10, 100, 500 or 1000

A220FCUEfisProfile::A220FCUEfisProfile(ProductFCUEfis *product) : FCUEfisAircraftProfile(product) {
    // The aircraft has no panel dimming logic, so backlights simply follow avionics power.
    Dataref::getInstance()->monitorExistingDataref<int>("a220/electrical/power_status", [product](int powered) {
        uint8_t target = powered ? 255 : 0;

        product->setLedBrightness(FCUEfisLed::BACKLIGHT, target);
        product->setLedBrightness(FCUEfisLed::EFISL_BACKLIGHT, target);
        product->setLedBrightness(FCUEfisLed::EFISR_BACKLIGHT, target);
        product->setLedBrightness(FCUEfisLed::EXPED_BACKLIGHT, target);

        product->setLedBrightness(FCUEfisLed::SCREEN_BACKLIGHT, target);
        product->setLedBrightness(FCUEfisLed::EFISL_SCREEN_BACKLIGHT, target);
        product->setLedBrightness(FCUEfisLed::EFISR_SCREEN_BACKLIGHT, target);

        product->setLedBrightness(FCUEfisLed::OVERALL_GREEN, target);
        product->setLedBrightness(FCUEfisLed::EFISL_OVERALL_GREEN, target);
        product->setLedBrightness(FCUEfisLed::EFISR_OVERALL_GREEN, target);

        product->forceStateSync();
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<int>("a220/autopilot/APLightStatus", [product](int engaged) {
        product->setLedBrightness(FCUEfisLed::AP1_GREEN, engaged ? 255 : 0);
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<int>("a220/autopilot/ATLightStatus", [product](int engaged) {
        product->setLedBrightness(FCUEfisLed::ATHR_GREEN, engaged ? 255 : 0);
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<int>("a220/autopilot/NAVLightStatus", [product](int engaged) {
        product->setLedBrightness(FCUEfisLed::LOC_GREEN, engaged ? 255 : 0);
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<int>("a220/autopilot/APPRLightStatus", [product](int engaged) {
        product->setLedBrightness(FCUEfisLed::APPR_GREEN, engaged ? 255 : 0);
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<int>("sim/cockpit2/autopilot/flight_director_command_bars_pilot", [product](int on) {
        product->setLedBrightness(FCUEfisLed::EFISL_FD_GREEN, on ? 255 : 0);
        product->setLedBrightness(FCUEfisLed::EFISR_FD_GREEN, on ? 255 : 0);
    },
        this);

    // No second autopilot channel and no expedite mode on the A220.
    product->setLedBrightness(FCUEfisLed::AP2_GREEN, 0);
    product->setLedBrightness(FCUEfisLed::EXPED_GREEN, 0);

    Dataref::getInstance()->executeChangedCallbacksForDataref("a220/electrical/power_status");
}

bool A220FCUEfisProfile::IsEligible() {
    return Dataref::getInstance()->exists("a220/autopilot/HalfBankLightStatus");
}

const std::vector<std::string> &A220FCUEfisProfile::displayDatarefs() const {
    static const std::vector<std::string> datarefs = {
        "a220/electrical/power_status",
        "sim/cockpit2/autopilot/airspeed_is_mach",
        "sim/cockpit2/autopilot/airspeed_dial_kts_mach",
        "sim/cockpit2/autopilot/heading_dial_deg_mag_pilot",
        "sim/cockpit2/autopilot/altitude_dial_ft",
        "sim/cockpit/autopilot/vertical_velocity",
        kBaroPilot,
        kBaroMode,
    };

    return datarefs;
}

const std::unordered_map<uint16_t, FCUEfisButtonDef> &A220FCUEfisProfile::buttonDefs() const {
    static const std::unordered_map<uint16_t, FCUEfisButtonDef> buttons = {
        // FCU mode keys. AP2 (4), EXPED (6) and METRIC ALT (7) stay unmapped: the A220 has a
        // single AP channel and no expedite mode, and its VNAV / 1-2 BANK keys have no
        // matching hardware key.
        {0, {"SPD/MACH", "a220/autopilot/speedAPPush", FCUEfisDatarefType::EXECUTE_CMD_PHASED}},
        {1, {"LOC", "a220/autopilot/NAVActivate", FCUEfisDatarefType::EXECUTE_CMD_PHASED}},
        {2, {"TRK", "a220/autopilot/FPAActivate", FCUEfisDatarefType::EXECUTE_CMD_PHASED}},
        {3, {"AP1", "a220/autopilot/APActivate", FCUEfisDatarefType::EXECUTE_CMD_PHASED}},
        {5, {"A/THR", "a220/autopilot/ATActivate", FCUEfisDatarefType::EXECUTE_CMD_PHASED}},
        {8, {"APPR", "a220/autopilot/APPRActivate", FCUEfisDatarefType::EXECUTE_CMD_PHASED}},

        // Speed encoder. Push is the IAS/MACH changeover, pull selects FLC.
        {9, {"SPD DEC", "a220/autopilot/speedAPDecrease", FCUEfisDatarefType::EXECUTE_CMD_PHASED}},
        {10, {"SPD INC", "a220/autopilot/speedAPIncrease", FCUEfisDatarefType::EXECUTE_CMD_PHASED}},
        {11, {"SPD PUSH", "a220/autopilot/speedAPPush", FCUEfisDatarefType::EXECUTE_CMD_PHASED}},
        {12, {"SPD PULL", "a220/autopilot/FLCActivate", FCUEfisDatarefType::EXECUTE_CMD_PHASED}},

        // Heading encoder. Push syncs the bug to the current heading.
        {13, {"HDG DEC", "a220/autopilot/hdgAPDecrease", FCUEfisDatarefType::EXECUTE_CMD_PHASED}},
        {14, {"HDG INC", "a220/autopilot/hdgAPIncrease", FCUEfisDatarefType::EXECUTE_CMD_PHASED}},
        {15, {"HDG PUSH", "a220/autopilot/hdgAPSynchPush", FCUEfisDatarefType::EXECUTE_CMD_PHASED}},
        {16, {"HDG PULL", "a220/autopilot/HDGActivate", FCUEfisDatarefType::EXECUTE_CMD_PHASED}},

        // Altitude encoder. Push cycles the aircraft's 10/100/500/1000 ft step.
        {17, {"ALT DEC", "a220/autopilot/altAPDecrease", FCUEfisDatarefType::EXECUTE_CMD_PHASED}},
        {18, {"ALT INC", "a220/autopilot/altAPIncrease", FCUEfisDatarefType::EXECUTE_CMD_PHASED}},
        {19, {"ALT PUSH", "a220/autopilot/altAPPush", FCUEfisDatarefType::EXECUTE_CMD_PHASED}},
        {20, {"ALT PULL", "a220/autopilot/ALTActivate", FCUEfisDatarefType::EXECUTE_CMD_PHASED}},

        // Step selector, handled in buttonPressed.
        {25, {"ALT STEP", kAltStep, FCUEfisDatarefType::SET_VALUE, 100.0}},
        {26, {"ALT STEP", kAltStep, FCUEfisDatarefType::SET_VALUE, 1000.0}},

        // Vertical speed encoder. The A220 has no VS sync, so push (23) is unmapped.
        {21, {"VS DEC", "a220/autopilot/vsAPDecrease", FCUEfisDatarefType::EXECUTE_CMD_PHASED}},
        {22, {"VS INC", "a220/autopilot/vsAPIncrease", FCUEfisDatarefType::EXECUTE_CMD_PHASED}},
        {24, {"VS PULL", "a220/autopilot/VSActivate", FCUEfisDatarefType::EXECUTE_CMD_PHASED}},

        // EFIS left. The ND mode selector and the VOR/ADF source selectors are unmapped:
        // the aircraft pins sim/cockpit2/EFIS/map_mode to 3 and does not model nav sources.
        {32, {"L_FD", "a220/autopilot/FDActivate", FCUEfisDatarefType::EXECUTE_CMD_PHASED}},
        {39, {"L_STD PUSH", "a220/barometer/button_press", FCUEfisDatarefType::EXECUTE_CMD_PHASED}},
        {41, {"L_PRESS DEC", "custom", FCUEfisDatarefType::BAROMETER_PILOT, -1.0}},
        {42, {"L_PRESS INC", "custom", FCUEfisDatarefType::BAROMETER_PILOT, 1.0}},
        {43, {"L_inHg", kBaroMode, FCUEfisDatarefType::SET_VALUE, 0.0}},
        {44, {"L_hPa", kBaroMode, FCUEfisDatarefType::SET_VALUE, 1.0}},

        // Range selector. The A220 range list is 1/4/10/20/40/80/160 NM (index 0-6), so the
        // six detents step 1:1 through indices 1-6; the labels sit one notch off.
        {50, {"L_RANGE 10", "sim/cockpit2/EFIS/map_range", FCUEfisDatarefType::SET_VALUE, 1.0}},
        {51, {"L_RANGE 20", "sim/cockpit2/EFIS/map_range", FCUEfisDatarefType::SET_VALUE, 2.0}},
        {52, {"L_RANGE 40", "sim/cockpit2/EFIS/map_range", FCUEfisDatarefType::SET_VALUE, 3.0}},
        {53, {"L_RANGE 80", "sim/cockpit2/EFIS/map_range", FCUEfisDatarefType::SET_VALUE, 4.0}},
        {54, {"L_RANGE 160", "sim/cockpit2/EFIS/map_range", FCUEfisDatarefType::SET_VALUE, 5.0}},
        {55, {"L_RANGE 320", "sim/cockpit2/EFIS/map_range", FCUEfisDatarefType::SET_VALUE, 6.0}},

        // EFIS right. Both PFDs read the captain barometer and there is one FD state, so the
        // right unit drives the same aircraft state. map_range_copilot is force-synced to
        // map_range by the aircraft, so the range selector writes map_range here too.
        {64, {"R_FD", "a220/autopilot/FDActivate", FCUEfisDatarefType::EXECUTE_CMD_PHASED}},
        {71, {"R_STD PUSH", "a220/barometer/button_press", FCUEfisDatarefType::EXECUTE_CMD_PHASED}},
        {73, {"R_PRESS DEC", "custom", FCUEfisDatarefType::BAROMETER_FO, -1.0}},
        {74, {"R_PRESS INC", "custom", FCUEfisDatarefType::BAROMETER_FO, 1.0}},
        {75, {"R_inHg", kBaroMode, FCUEfisDatarefType::SET_VALUE, 0.0}},
        {76, {"R_hPa", kBaroMode, FCUEfisDatarefType::SET_VALUE, 1.0}},

        {82, {"R_RANGE 10", "sim/cockpit2/EFIS/map_range", FCUEfisDatarefType::SET_VALUE, 1.0}},
        {83, {"R_RANGE 20", "sim/cockpit2/EFIS/map_range", FCUEfisDatarefType::SET_VALUE, 2.0}},
        {84, {"R_RANGE 40", "sim/cockpit2/EFIS/map_range", FCUEfisDatarefType::SET_VALUE, 3.0}},
        {85, {"R_RANGE 80", "sim/cockpit2/EFIS/map_range", FCUEfisDatarefType::SET_VALUE, 4.0}},
        {86, {"R_RANGE 160", "sim/cockpit2/EFIS/map_range", FCUEfisDatarefType::SET_VALUE, 5.0}},
        {87, {"R_RANGE 320", "sim/cockpit2/EFIS/map_range", FCUEfisDatarefType::SET_VALUE, 6.0}},
    };

    return buttons;
}

void A220FCUEfisProfile::updateDisplayData(FCUDisplayData &data) {
    auto datarefManager = Dataref::getInstance();

    if (!datarefManager->getCached<int>("a220/electrical/power_status")) {
        data.speed = "";
        data.heading = "";
        data.altitude = "";
        data.verticalSpeed = "";
        data.efisLeft.baro = "";
        data.efisRight.baro = "";
        data.displayEnabled = false;
        return;
    }

    data.displayEnabled = true;
    data.headingHdg = true;
    data.headingLat = true;
    data.vsIndication = true;

    // The A220 FCU has no level change arrows.
    data.displayEnabledWindowsFlag &= ~FCUDisplayData::Window::LevelChangeHeader;

    data.spdMach = datarefManager->getCached<bool>("sim/cockpit2/autopilot/airspeed_is_mach");
    float speed = datarefManager->getCached<float>("sim/cockpit2/autopilot/airspeed_dial_kts_mach");
    if (speed > 0) {
        std::ostringstream ss;
        if (data.spdMach) {
            ss << std::setfill('0') << std::setw(3) << static_cast<int>(std::round(speed * 100));
        } else {
            ss << std::setfill('0') << std::setw(3) << static_cast<int>(speed);
        }
        data.speed = ss.str();
    } else {
        data.speed = "---";
    }

    float heading = datarefManager->getCached<float>("sim/cockpit2/autopilot/heading_dial_deg_mag_pilot");
    std::ostringstream headingSs;
    headingSs << std::setfill('0') << std::setw(3) << (static_cast<int>(std::round(heading)) % 360 + 360) % 360;
    data.heading = headingSs.str();

    float altitude = datarefManager->getCached<float>("sim/cockpit2/autopilot/altitude_dial_ft");
    std::ostringstream altSs;
    altSs << std::setfill('0') << std::setw(5) << std::max(0, static_cast<int>(std::round(altitude)));
    data.altitude = altSs.str();

    float vs = datarefManager->getCached<float>("sim/cockpit/autopilot/vertical_velocity");
    int absVs = std::abs(static_cast<int>(std::round(vs)));
    std::ostringstream vsSs;
    vsSs << std::setfill('0') << std::setw(4) << absVs;
    data.verticalSpeed = vsSs.str();
    data.vsSign = (vs >= 0);

    // Single baro state: both PFDs read the captain dataref, and the IN/HPA selection is a
    // single aircraft-wide mode. The A220 PFD never shows STD, only the numeric value.
    bool unitIsInHg = datarefManager->getCached<int>(kBaroMode) == 0;
    float baro = datarefManager->getCached<float>(kBaroPilot);

    EfisDisplayValue baroValue = {};
    if (baro > 0) {
        baroValue.setBaro(baro, unitIsInHg);
    }

    data.efisLeft = baroValue;
    data.efisRight = baroValue;
}

void A220FCUEfisProfile::buttonPressed(const FCUEfisButtonDef *button, XPLMCommandPhase phase) {
    if (!button || button->dataref.empty() || phase == xplm_CommandContinue) {
        return;
    }

    auto datarefManager = Dataref::getInstance();

    if (button->name == "ALT STEP") {
        if (phase != xplm_CommandBegin) {
            return;
        }

        // The aircraft's own knob handler steps by a Lua-local index, not by alt_step_ft, so
        // writing the dataref alone would not change the step. altAPPush advances that index
        // and publishes the new value to alt_step_ft, so pushing until the dataref matches
        // brings the aircraft's step in line with the hardware selector. Steps cycle through
        // 10/100/500/1000, so the target is always reached within one full cycle.
        bool matched = false;
        for (int attempt = 0; attempt < 5 && !matched; ++attempt) {
            matched = altStepSynced && std::fabs(datarefManager->get<float>(kAltStep) - button->value) < 0.5f;
            if (!matched) {
                datarefManager->executeCommand("a220/autopilot/altAPPush");
                altStepSynced = true;
            }
        }

        if (!matched) {
            datarefManager->set<float>(kAltStep, button->value);
        }

        return;
    }

    if (button->datarefType == FCUEfisDatarefType::BAROMETER_PILOT || button->datarefType == FCUEfisDatarefType::BAROMETER_FO) {
        if (phase != xplm_CommandBegin) {
            return;
        }

        // Both EFIS units drive the captain barometer: the A220 models only one.
        bool unitIsInHg = datarefManager->getCached<int>(kBaroMode) == 0;
        bool increase = button->value > 0;
        float baro = datarefManager->getCached<float>(kBaroPilot);

        if (unitIsInHg) {
            baro += increase ? 0.01f : -0.01f;
        } else {
            float hpa = std::round(baro * 33.8639f) + (increase ? 1.0f : -1.0f);
            baro = hpa / 33.8639f;
        }

        datarefManager->set<float>(kBaroPilot, baro);
    } else if (button->datarefType == FCUEfisDatarefType::SET_VALUE) {
        if (phase == xplm_CommandBegin) {
            datarefManager->set<float>(button->dataref.c_str(), button->value);
        }
    } else if (button->datarefType == FCUEfisDatarefType::EXECUTE_CMD_ONCE) {
        if (phase == xplm_CommandBegin) {
            datarefManager->executeCommand(button->dataref.c_str());
        }
    } else {
        datarefManager->executeCommand(button->dataref.c_str(), phase);
    }
}
