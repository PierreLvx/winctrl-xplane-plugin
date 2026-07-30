#include "b58-fcu-efis-profile.h"

#include "appstate.h"
#include "dataref.h"
#include "product-fcu-efis.h"

#include <cmath>
#include <XPLMUtilities.h>

// Laminar Beechcraft Baron 58. The cockpit is a KFC 150 autopilot with a KA 285 annunciator
// panel and a KI 525A HSI, all driven by X-Plane's generic autopilot (verified: the cockpit
// buttons issue sim/autopilot/* commands and the annunciators read sim/cockpit2/autopilot/*).
// The panel has no altitude preselect, no vertical-speed selector and no speed mode, so the
// FCU shows mode headers only; the pitch wheel commands pitch attitude, not vertical speed.

static constexpr const char *kBaroPilot = "sim/cockpit2/gauges/actuators/barometer_setting_in_hg_pilot";
static constexpr const char *kBaroCopilot = "sim/cockpit2/gauges/actuators/barometer_setting_in_hg_copilot";

B58FCUEfisProfile::B58FCUEfisProfile(ProductFCUEfis *product) : FCUEfisAircraftProfile(product) {
    // Index 1 is the flight-instrument panel light: it tracks both the rheostat and the panel
    // light switch (verified by toggling laminar/b58/lighting/panel_light_switch).
    Dataref::getInstance()->monitorExistingDataref<std::vector<float>>("sim/cockpit2/electrical/instrument_brightness_ratio_manual", [product](const std::vector<float> &brightness) {
        if (brightness.size() < 2) {
            return;
        }

        bool hasPower = Dataref::getInstance()->get<bool>("sim/cockpit/electrical/battery_on");

        uint8_t target = hasPower ? brightness[1] * 255 : 0;
        product->setLedBrightness(FCUEfisLed::BACKLIGHT, 0);
        product->setLedBrightness(FCUEfisLed::EFISR_BACKLIGHT, 0);
        product->setLedBrightness(FCUEfisLed::EFISL_BACKLIGHT, 0);
        product->setLedBrightness(FCUEfisLed::EXPED_BACKLIGHT, 0);

        product->setLedBrightness(FCUEfisLed::OVERALL_GREEN, target);
        product->setLedBrightness(FCUEfisLed::EFISR_OVERALL_GREEN, target);
        product->setLedBrightness(FCUEfisLed::EFISL_OVERALL_GREEN, target);
        product->setLedBrightness(FCUEfisLed::SCREEN_BACKLIGHT, target);
        product->setLedBrightness(FCUEfisLed::EFISR_SCREEN_BACKLIGHT, target);
        product->setLedBrightness(FCUEfisLed::EFISL_SCREEN_BACKLIGHT, target);

        product->forceStateSync();
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<bool>("sim/cockpit/electrical/battery_on", [](bool batteryOn) {
        Dataref::getInstance()->executeChangedCallbacksForDataref("sim/cockpit2/electrical/instrument_brightness_ratio_manual");
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<bool>("sim/physics/metric_press", [product](bool isMetric) {
        product->updateDisplays();
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<bool>("sim/cockpit2/autopilot/servos_on", [product](bool isAutopilotEngaged) {
        product->setLedBrightness(FCUEfisLed::AP1_GREEN, isAutopilotEngaged ? 1 : 0);
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<int>("sim/cockpit2/autopilot/nav_status", [product](int navStatus) {
        product->setLedBrightness(FCUEfisLed::LOC_GREEN, navStatus > 0 ? 1 : 0);
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<int>("sim/cockpit2/autopilot/approach_status", [product](int approachStatus) {
        product->setLedBrightness(FCUEfisLed::APPR_GREEN, approachStatus > 0 ? 1 : 0);
    },
        this);

    // One flight director, shared by both EFIS units.
    Dataref::getInstance()->monitorExistingDataref<int>("sim/cockpit2/autopilot/flight_director_mode", [product](int fdMode) {
        product->setLedBrightness(FCUEfisLed::EFISL_FD_GREEN, fdMode > 0 ? 1 : 0);
        product->setLedBrightness(FCUEfisLed::EFISR_FD_GREEN, fdMode > 0 ? 1 : 0);
    },
        this);

    // Single autopilot channel, no autothrottle and no expedite mode.
    product->setLedBrightness(FCUEfisLed::AP2_GREEN, 0);
    product->setLedBrightness(FCUEfisLed::ATHR_GREEN, 0);
    product->setLedBrightness(FCUEfisLed::EXPED_GREEN, 0);

    Dataref::getInstance()->executeChangedCallbacksForDataref("sim/cockpit2/electrical/instrument_brightness_ratio_manual");
}

bool B58FCUEfisProfile::IsEligible() {
    return Dataref::getInstance()->exists("laminar/b58/annun/yaw_damp_on");
}

const std::vector<std::string> &B58FCUEfisProfile::displayDatarefs() const {
    static const std::vector<std::string> datarefs = {
        "sim/cockpit/electrical/battery_on",
        "sim/cockpit2/autopilot/autopilot_has_power",
        "sim/cockpit2/autopilot/heading_status",
        "sim/cockpit2/autopilot/nav_status",
        "sim/cockpit2/autopilot/approach_status",
        "sim/cockpit2/autopilot/altitude_hold_status",
        "sim/physics/metric_press",
        kBaroPilot,
        kBaroCopilot,
    };
    return datarefs;
}

const std::unordered_map<uint16_t, FCUEfisButtonDef> &B58FCUEfisProfile::buttonDefs() const {
    static const std::unordered_map<uint16_t, FCUEfisButtonDef> buttons = {
        // FCU mode keys. SPD/MACH (0), TRK (2), AP2 (4), A/THR (5), EXPED (6) and METRIC ALT (7)
        // stay unmapped: the KFC 150 has no speed mode, no track mode, a single servo channel,
        // no autothrottle, no expedite and no metric altitude. Back course, yaw damper and
        // go-around are real Baron functions but have no matching key on this hardware.
        {1, {"LOC", "sim/autopilot/NAV"}},
        {3, {"AP1", "sim/autopilot/servos_toggle"}},
        {8, {"APPR", "sim/autopilot/approach"}},

        // Heading bug on the KI 525A. Pull engages HDG mode; the HSI has no bug-sync function,
        // so push (15) stays unmapped.
        {13, {"HDG DEC", "sim/autopilot/heading_down"}},
        {14, {"HDG INC", "sim/autopilot/heading_up"}},
        {16, {"HDG PULL", "sim/autopilot/heading"}},

        // ALT captures the altitude the aircraft is at. There is no preselector, so the altitude
        // encoder (17/18) and the 100/1000 ft step selector (25/26) stay unmapped.
        {19, {"ALT PUSH", "sim/autopilot/altitude_hold"}},
        {20, {"ALT PULL", "sim/autopilot/altitude_hold"}},

        // KFC 150 pitch wheel: it trims the pitch attitude the flight director commands, not a
        // vertical speed. Push/pull (23/24) stay unmapped, there is no V/S mode to select.
        {21, {"VS DEC", "sim/autopilot/nose_down_pitch_mode"}},
        {22, {"VS INC", "sim/autopilot/nose_up_pitch_mode"}},

        // EFIS left drives the pilot altimeter, EFIS right the copilot altimeter. Both Kollsman
        // windows read in inHg only, so IN/HPA changes the hardware readout, not the cockpit.
        // The altimeters have no STD function, so STD push/pull (39/40, 71/72) stay unmapped, and
        // with no ND in the aircraft neither do the ND mode, range and VOR/ADF selectors.
        {32, {"L_FD", "sim/autopilot/fdir_toggle"}},
        {41, {"L_PRESS DEC", kBaroPilot, FCUEfisDatarefType::BAROMETER_PILOT, -1.0}},
        {42, {"L_PRESS INC", kBaroPilot, FCUEfisDatarefType::BAROMETER_PILOT, 1.0}},
        {43, {"L_inHg", "sim/physics/metric_press", FCUEfisDatarefType::SET_VALUE, 0.0}},
        {44, {"L_hPa", "sim/physics/metric_press", FCUEfisDatarefType::SET_VALUE, 1.0}},

        {64, {"R_FD", "sim/autopilot/fdir_toggle"}},
        {73, {"R_PRESS DEC", kBaroCopilot, FCUEfisDatarefType::BAROMETER_FO, -1.0}},
        {74, {"R_PRESS INC", kBaroCopilot, FCUEfisDatarefType::BAROMETER_FO, 1.0}},
        {75, {"R_inHg", "sim/physics/metric_press", FCUEfisDatarefType::SET_VALUE, 0.0}},
        {76, {"R_hPa", "sim/physics/metric_press", FCUEfisDatarefType::SET_VALUE, 1.0}},
    };

    return buttons;
}

void B58FCUEfisProfile::updateDisplayData(FCUDisplayData &data) {
    auto datarefManager = Dataref::getInstance();

    // Mode headers only: the aircraft selects nothing the numeric windows could show. Modes
    // annunciate as soon as the flight director is on, so the LCD follows autopilot power
    // rather than servo engagement.
    data.displayEnabled = datarefManager->getCached<bool>("sim/cockpit2/autopilot/autopilot_has_power");
    data.displayTest = false;
    data.displayEnabledWindowsFlag = FCUDisplayData::Window::None;
    data.speed = "";
    data.heading = "";
    data.altitude = "";
    data.verticalSpeed = "";

    // Lateral: HDG label for heading mode, LAT label for NAV/APR tracking.
    bool hdgActive = datarefManager->getCached<int>("sim/cockpit2/autopilot/heading_status") > 0;
    bool navActive = datarefManager->getCached<int>("sim/cockpit2/autopilot/nav_status") > 0 ||
                     datarefManager->getCached<int>("sim/cockpit2/autopilot/approach_status") > 0;
    if (hdgActive || navActive) {
        data.displayEnabledWindowsFlag |= FCUDisplayData::Window::HeadingTrackHeader;
        data.headingHdg = hdgActive;
        data.headingTrk = false;
        data.headingLat = navActive;
    }

    // Vertical: only ALT hold annunciates. The pitch wheel never sets a V/S mode, so
    // vvi_status stays 0 on this aircraft and the V/S label would never light.
    if (datarefManager->getCached<int>("sim/cockpit2/autopilot/altitude_hold_status") > 0) {
        data.displayEnabledWindowsFlag |= FCUDisplayData::Window::AltitudeHeader;
        data.altIndication = true;
    }

    bool unitIsInHg = !datarefManager->getCached<bool>("sim/physics/metric_press");
    bool powered = datarefManager->getCached<bool>("sim/cockpit/electrical/battery_on");

    data.efisLeft.displayEnabled = powered;
    data.efisLeft.setBaro(datarefManager->getCached<float>(kBaroPilot), unitIsInHg);

    data.efisRight.displayEnabled = powered;
    data.efisRight.setBaro(datarefManager->getCached<float>(kBaroCopilot), unitIsInHg);
}

void B58FCUEfisProfile::buttonPressed(const FCUEfisButtonDef *button, XPLMCommandPhase phase) {
    if (!button || button->dataref.empty() || phase != xplm_CommandBegin) {
        return;
    }

    auto datarefManager = Dataref::getInstance();

    if (button->datarefType == FCUEfisDatarefType::BAROMETER_PILOT || button->datarefType == FCUEfisDatarefType::BAROMETER_FO) {
        // The Kollsman knob steps 0.01 inHg. In hPa the readout is stepped a whole hectopascal
        // at a time instead, which the aircraft's own barometer commands cannot do.
        bool increase = button->value > 0;
        float baro = datarefManager->getCached<float>(button->dataref.c_str());

        if (!datarefManager->getCached<bool>("sim/physics/metric_press")) {
            baro += increase ? 0.01f : -0.01f;
        } else {
            baro = (std::round(baro * 33.8639f) + (increase ? 1.0f : -1.0f)) / 33.8639f;
        }

        datarefManager->set<float>(button->dataref.c_str(), baro);
    } else if (button->datarefType == FCUEfisDatarefType::SET_VALUE) {
        datarefManager->set<float>(button->dataref.c_str(), button->value);
    } else {
        datarefManager->executeCommand(button->dataref.c_str());
    }
}
