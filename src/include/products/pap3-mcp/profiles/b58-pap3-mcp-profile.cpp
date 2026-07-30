#include "b58-pap3-mcp-profile.h"

#include "dataref.h"
#include "product-pap3-mcp.h"

#include <cmath>
#include <XPLMUtilities.h>

// Laminar Beechcraft Baron 58. The KFC 150 autopilot runs on X-Plane's generic autopilot, so
// the mode keys and annunciators use sim/autopilot/* and sim/cockpit2/autopilot/*. Only two of
// the six LCD windows have a source in this cockpit: CRS from the KI 525A course pointer and the
// NAV2 OBS, and HDG from the HSI heading bug. There is no altitude preselect, no vertical-speed
// selector and no speed mode, so those windows stay blank.

static constexpr const char *kCrsCapt = "sim/cockpit2/radios/actuators/hsi_obs_deg_mag_pilot";
static constexpr const char *kCrsFo = "sim/cockpit2/radios/actuators/nav2_obs_deg_mag_pilot";
static constexpr const char *kHeadingBug = "sim/cockpit2/autopilot/heading_dial_deg_mag_pilot";

B58PAP3MCPProfile::B58PAP3MCPProfile(ProductPAP3MCP *product) : PAP3MCPAircraftProfile(product) {
    // Index 1 is the flight-instrument panel light: it tracks both the rheostat and the panel
    // light switch (verified by toggling laminar/b58/lighting/panel_light_switch).
    Dataref::getInstance()->monitorExistingDataref<std::vector<float>>("sim/cockpit2/electrical/instrument_brightness_ratio_manual", [product](const std::vector<float> &brightness) {
        if (brightness.size() < 2) {
            return;
        }

        bool hasPower = Dataref::getInstance()->get<bool>("sim/cockpit/electrical/battery_on");

        uint8_t target = hasPower ? brightness[1] * 255 : 0;
        product->setLedBrightness(PAP3MCPLed::BACKLIGHT, target);
        product->setLedBrightness(PAP3MCPLed::LCD_BACKLIGHT, target);
        product->setLedBrightness(PAP3MCPLed::OVERALL_LED_BRIGHTNESS, target);

        product->forceStateSync();
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<bool>("sim/cockpit/electrical/battery_on", [](bool batteryOn) {
        Dataref::getInstance()->executeChangedCallbacksForDataref("sim/cockpit2/electrical/instrument_brightness_ratio_manual");
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<bool>("sim/cockpit2/autopilot/servos_on", [product](bool isAutopilotEngaged) {
        product->setLedBrightness(PAP3MCPLed::CMD_A, isAutopilotEngaged ? 1 : 0);
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<int>("sim/cockpit2/autopilot/heading_status", [product](int headingStatus) {
        product->setLedBrightness(PAP3MCPLed::HDG_SEL, headingStatus > 0 ? 1 : 0);
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<int>("sim/cockpit2/autopilot/nav_status", [product](int navStatus) {
        product->setLedBrightness(PAP3MCPLed::VORLOC, navStatus > 0 ? 1 : 0);
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<int>("sim/cockpit2/autopilot/approach_status", [product](int approachStatus) {
        product->setLedBrightness(PAP3MCPLed::APP, approachStatus > 0 ? 1 : 0);
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<int>("sim/cockpit2/autopilot/altitude_hold_status", [product](int altitudeHoldStatus) {
        product->setLedBrightness(PAP3MCPLed::ALT_HLD, altitudeHoldStatus > 0 ? 1 : 0);
    },
        this);

    // No autothrottle, no N1/VNAV/LNAV/LVL CHG modes, no V/S mode and a single servo channel.
    product->setLedBrightness(PAP3MCPLed::N1, 0);
    product->setLedBrightness(PAP3MCPLed::SPEED, 0);
    product->setLedBrightness(PAP3MCPLed::VNAV, 0);
    product->setLedBrightness(PAP3MCPLed::LVL_CHG, 0);
    product->setLedBrightness(PAP3MCPLed::LNAV, 0);
    product->setLedBrightness(PAP3MCPLed::VS, 0);
    product->setLedBrightness(PAP3MCPLed::CWS_A, 0);
    product->setLedBrightness(PAP3MCPLed::CMD_B, 0);
    product->setLedBrightness(PAP3MCPLed::CWS_B, 0);
    product->setLedBrightness(PAP3MCPLed::AT_ARM, 0);
    product->setLedBrightness(PAP3MCPLed::MA_CAPT, 0);
    product->setLedBrightness(PAP3MCPLed::MA_FO, 0);

    Dataref::getInstance()->executeChangedCallbacksForDataref("sim/cockpit2/electrical/instrument_brightness_ratio_manual");
}

bool B58PAP3MCPProfile::IsEligible() {
    return Dataref::getInstance()->exists("laminar/b58/annun/yaw_damp_on");
}

const std::vector<std::string> &B58PAP3MCPProfile::displayDatarefs() const {
    static const std::vector<std::string> datarefs = {
        "sim/cockpit/electrical/battery_on",
        kCrsCapt,
        kCrsFo,
        kHeadingBug,
        "sim/cockpit2/autopilot/servos_on",
        "sim/cockpit2/autopilot/heading_status",
        "sim/cockpit2/autopilot/nav_status",
        "sim/cockpit2/autopilot/approach_status",
        "sim/cockpit2/autopilot/altitude_hold_status",
    };
    return datarefs;
}

const std::unordered_map<uint16_t, PAP3MCPButtonDef> &B58PAP3MCPProfile::buttonDefs() const {
    static const std::unordered_map<uint16_t, PAP3MCPButtonDef> buttons = {
        // Mode keys. N1 (0), SPEED (1), VNAV (2), LVL CHG (3), LNAV (5), V/S (9), CWS A (11),
        // CMD B (12), CWS B (13), C/O (14), SPD INTV (15) and ALT INTV (16) stay unmapped: the
        // KFC 150 has none of those modes and only one servo channel. Back course, yaw damper
        // and go-around are real Baron functions but have no matching key on this hardware.
        {4, {"HDG SEL", "sim/autopilot/heading"}},
        {6, {"VOR LOC", "sim/autopilot/NAV"}},
        {7, {"APP", "sim/autopilot/approach"}},
        {8, {"ALT HLD", "sim/autopilot/altitude_hold"}},
        {10, {"CMD A", "sim/autopilot/servos_toggle"}},

        // Encoder rotations (exposed as button pairs by the hardware).
        // CRS CAPT is the KI 525A course pointer, CRS FO the NAV2 OBS.
        {17, {"CRS CAPT DEC", "sim/radios/obs_HSI_down"}},
        {18, {"CRS CAPT INC", "sim/radios/obs_HSI_up"}},
        // SPD (19/20) unmapped: no speed mode.
        {21, {"HDG DEC", "sim/autopilot/heading_down"}},
        {22, {"HDG INC", "sim/autopilot/heading_up"}},
        // ALT (23/24) unmapped: no altitude preselect.
        {25, {"CRS FO DEC", "sim/radios/obs2_down"}},
        {26, {"CRS FO INC", "sim/radios/obs2_up"}},

        // KFC 150 pitch wheel: it trims the pitch attitude the flight director commands, not a
        // vertical speed, which is why the V/S window stays blank.
        {38, {"VS DEC", "sim/autopilot/nose_down_pitch_mode"}},
        {39, {"VS INC", "sim/autopilot/nose_up_pitch_mode"}},

        // 27/29: FD CAPT/FO   - handled by handleSwitchChanged (byte 0x04, bits 0x08 / 0x20)
        // 31/32: AP DISC bar  - handled by handleSwitchChanged (byte 0x04 bit 0x80, byte 0x05 bit 0x01)
        // 33-37: Bank angle   - unmapped, the Baron has no bank limit selector
        // 40/41: A/T ARM      - unmapped, the Baron has no autothrottle
    };
    return buttons;
}

const std::vector<PAP3MCPEncoderDef> &B58PAP3MCPProfile::encoderDefs() const {
    static const std::vector<PAP3MCPEncoderDef> encoders = {
        {0, "CRS CAPT", "sim/radios/obs_HSI_up", "sim/radios/obs_HSI_down"},
        {2, "HDG", "sim/autopilot/heading_up", "sim/autopilot/heading_down"},
        {4, "V/S", "sim/autopilot/nose_up_pitch_mode", "sim/autopilot/nose_down_pitch_mode"},
        {5, "CRS FO", "sim/radios/obs2_up", "sim/radios/obs2_down"},
    };
    return encoders;
}

void B58PAP3MCPProfile::updateDisplayData(PAP3MCPDisplayData &data) {
    auto datarefManager = Dataref::getInstance();

    data.displayEnabled = datarefManager->getCached<bool>("sim/cockpit/electrical/battery_on");
    data.displayTest = false;

    // Only the two HSI-derived windows have a source in this cockpit.
    data.showCourse = true;
    data.headingVisible = true;
    data.speedVisible = false;
    data.altitudeVisible = false;
    data.verticalSpeedVisible = false;
    data.showDashesWhenInactive = false;
    data.showLabels = false;
    data.showLabelsWhenInactive = false;

    data.crsCapt = static_cast<int>(std::lround(datarefManager->getCached<float>(kCrsCapt)));
    data.crsFo = static_cast<int>(std::lround(datarefManager->getCached<float>(kCrsFo)));
    data.heading = static_cast<int>(std::lround(datarefManager->getCached<float>(kHeadingBug)));

    data.speed = 0.0f;
    data.altitude = 0;
    data.verticalSpeed = 0.0f;
    data.spdMach = false;
    data.digitA = false;
    data.digitB = false;
}

void B58PAP3MCPProfile::buttonPressed(const PAP3MCPButtonDef *button, XPLMCommandPhase phase) {
    if (!button || button->dataref.empty() || phase != xplm_CommandBegin) {
        return;
    }

    Dataref::getInstance()->executeCommand(button->dataref.c_str());
}

void B58PAP3MCPProfile::encoderRotated(const PAP3MCPEncoderDef *encoder, int8_t delta) {
    if (!encoder || delta == 0) {
        return;
    }

    const char *cmd = (delta > 0) ? encoder->incCmd.c_str() : encoder->decCmd.c_str();
    int steps = std::abs(static_cast<int>(delta));

    for (int i = 0; i < steps; i++) {
        Dataref::getInstance()->executeCommand(cmd);
    }
}

void B58PAP3MCPProfile::handleSwitchChanged(uint8_t byteOffset, uint8_t bitMask, bool state) {
    auto datarefManager = Dataref::getInstance();

    // FD CAPT (bit 0x08) and FD FO (bit 0x20) both drive the single Baron flight director, so
    // either switch moves it. Sync to the switch position instead of toggling blindly, otherwise
    // the hardware and the aircraft drift apart when the FD is switched off in the cockpit.
    if (byteOffset == 0x04 && (bitMask == 0x08 || bitMask == 0x20)) {
        bool fdOn = datarefManager->get<int>("sim/cockpit2/autopilot/flight_director_mode") > 0;
        if (fdOn != state) {
            datarefManager->executeCommand("sim/autopilot/fdir_toggle");
        }
        return;
    }

    // AP disconnect bar down drops the servos. Raising it only clears the bar: the KFC 150
    // re-engages from the AP key, not from the bar.
    if (byteOffset == 0x04 && bitMask == 0x80 && state) {
        datarefManager->executeCommand("sim/autopilot/servos_off_any");
    }
}
