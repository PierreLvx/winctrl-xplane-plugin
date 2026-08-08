#include "kingair350-fcu-efis-profile.h"

#include "appstate.h"
#include "dataref.h"
#include "product-fcu-efis.h"

#include <XPLMUtilities.h>

static constexpr const char *kPanelLightKnob = "KA350/ianim/ohPanel/pilotInstrPanelLight";
static constexpr const char *kMasterPanelLights = "KA350/ianim/ohPanel/masterPanelLights";
static constexpr const char *kBatteryOn = "sim/cockpit/electrical/battery_on";

// Mirrors the aircraft's own rheostat chain: knob position, zeroed when the master panel lights
// switch or the bus is off. Deliberately not sim/cockpit2/switches/instrument_brightness_ratio,
// which is the rendered light level and tracks ambient sunlight.
static void applyBacklight(ProductFCUEfis *product) {
    auto datarefManager = Dataref::getInstance();
    bool powered = datarefManager->get<bool>(kMasterPanelLights) && datarefManager->get<bool>(kBatteryOn);
    uint8_t target = powered ? datarefManager->get<float>(kPanelLightKnob) * 255 : 0;

    product->setLedBrightness(FCUEfisLed::BACKLIGHT, target);
    product->setLedBrightness(FCUEfisLed::EFISL_BACKLIGHT, target);
    product->setLedBrightness(FCUEfisLed::EFISR_BACKLIGHT, 0);
    product->setLedBrightness(FCUEfisLed::EXPED_BACKLIGHT, 0);

    product->setLedBrightness(FCUEfisLed::OVERALL_GREEN, target);
    product->setLedBrightness(FCUEfisLed::EFISR_OVERALL_GREEN, 0);
    product->setLedBrightness(FCUEfisLed::EFISL_OVERALL_GREEN, target);
    product->setLedBrightness(FCUEfisLed::SCREEN_BACKLIGHT, target);
    product->setLedBrightness(FCUEfisLed::EFISR_SCREEN_BACKLIGHT, 0);
    product->setLedBrightness(FCUEfisLed::EFISL_SCREEN_BACKLIGHT, target);

    product->forceStateSync();
}

KingAir350FCUEfisProfile::KingAir350FCUEfisProfile(ProductFCUEfis *product) : FCUEfisAircraftProfile(product) {
    Dataref::getInstance()->monitorExistingDataref<float>(kPanelLightKnob, [product](float) {
        applyBacklight(product);
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<bool>(kMasterPanelLights, [product](bool) {
        applyBacklight(product);
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<bool>(kBatteryOn, [product](bool) {
        applyBacklight(product);
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

    Dataref::getInstance()->monitorExistingDataref<bool>("sim/cockpit2/annunciators/flight_director", [product](bool enabled) {
        product->setLedBrightness(FCUEfisLed::EFISL_FD_GREEN, enabled ? 1 : 0);
    },
        this);

    applyBacklight(product);
}

bool KingAir350FCUEfisProfile::IsEligible() {
    return Dataref::getInstance()->exists("KA350/ianim/pSubpanel/beaconLights");
}

const std::vector<std::string> &KingAir350FCUEfisProfile::displayDatarefs() const {
    static const std::vector<std::string> datarefs = {
        "sim/cockpit2/gauges/actuators/barometer_setting_in_hg_pilot",
        "sim/cockpit2/autopilot/servos_on",
        "sim/cockpit/electrical/battery_on",
    };
    return datarefs;
}

const std::unordered_map<uint16_t, FCUEfisButtonDef> &KingAir350FCUEfisProfile::buttonDefs() const {
    static const std::unordered_map<uint16_t, FCUEfisButtonDef> buttons = {
        // CRS (course) is mapped to the SPD knob
        {9, {"CRS DEC", "KA350/cmd/cPanel/efisSCP/courseDec"}},
        {10, {"CRS INC", "KA350/cmd/cPanel/efisSCP/courseInc"}},
        {11, {"CRS PUSH", "KA350/cmd/cPanel/efisSCP/courseDirect"}}, // push to CRS direct

        {13, {"HDG DEC", "KA350/cmd/cPanel/efisSCP/headingDec"}},
        {14, {"HDG INC", "KA350/cmd/cPanel/efisSCP/headingInc"}},
        // {15, {"HDG PUSH", ""}}, // "heading to actual heading" sync command not captured yet

        {17, {"ALT DEC", "KA350/cmd/avpanel/alt/altSetDec"}},
        {18, {"ALT INC", "KA350/cmd/avpanel/alt/altSetInc"}},
        {19, {"ALT PUSH", "KA350/cmd/avpanel/alt/mode"}}, // toggle 1000/100 feet step

        {21, {"VS DEC", "KA350/cmd/cPanel/autopilotCP/pitchSwitchDec"}}, // nose down
        {22, {"VS INC", "KA350/cmd/cPanel/autopilotCP/pitchSwitchInc"}}, // nose up

        {41, {"BARO DEC", "sim/instruments/barometer_down"}},
        {42, {"BARO INC", "sim/instruments/barometer_up"}},
        {43, {"L_inHg", "sim/physics/metric_press", FCUEfisDatarefType::SET_VALUE, 0.0}},
        {44, {"L_hPa", "sim/physics/metric_press", FCUEfisDatarefType::SET_VALUE, 1.0}},
    };

    return buttons;
}

void KingAir350FCUEfisProfile::updateDisplayData(FCUDisplayData &data) {
    auto datarefManager = Dataref::getInstance();

    data.displayEnabled = false;
    data.displayTest = false;
    data.speed = "";
    data.heading = "";
    data.altitude = "";
    data.verticalSpeed = "";
    data.efisRight.baro = "";

    data.efisLeft.displayEnabled = datarefManager->getCached<bool>("sim/cockpit/electrical/battery_on");
    float baroPilot = datarefManager->getCached<float>("sim/cockpit2/gauges/actuators/barometer_setting_in_hg_pilot");
    data.efisLeft.setBaro(baroPilot, !datarefManager->getCached<bool>("sim/physics/metric_press"));
}

void KingAir350FCUEfisProfile::buttonPressed(const FCUEfisButtonDef *button, XPLMCommandPhase phase) {
    if (!button || button->dataref.empty() || phase == xplm_CommandContinue) {
        return;
    }

    auto datarefManager = Dataref::getInstance();
    if (phase == xplm_CommandBegin && button->datarefType == FCUEfisDatarefType::SET_VALUE) {
        datarefManager->set<float>(button->dataref.c_str(), button->value);
    } else if (phase == xplm_CommandBegin) {
        datarefManager->executeCommand(button->dataref.c_str());
    }
}
