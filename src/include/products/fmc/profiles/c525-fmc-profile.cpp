#include "c525-fmc-profile.h"

#include "dataref.h"
#include "product-fmc.h"
#include "uns1-decode.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <sstream>

// The TorqueSim CJ drives the stock Universal Avionics UNS-1 module, so the CDU
// display lives under "uns1/cdu1" and the FMS keys under "uns1/fms1".
static const std::string kCdu = "uns1/cdu1";
static const std::string kFms = "uns1/fms1";
static constexpr int kScreenLines = 11;

C525FMCProfile::C525FMCProfile(ProductFMC *product) : FMCAircraftProfile(product) {
    product->setAllLedsEnabled(false);

    Dataref::getInstance()->monitorExistingDataref<float>((kFms + "/screen_brightness").c_str(), [product](const float &brightness) {
        uint8_t target = static_cast<uint8_t>(std::clamp(brightness, 0.0f, 1.0f) * 255);
        product->setLedBrightness(FMCLed::SCREEN_BACKLIGHT, target);
    },
        this);

    auto font = Font::GlyphData("UNS1.xpwwf", product->identifierByte, product->hardwareType);
    if (!font.empty()) {
        for (auto &packet : font) {
            product->writeData(packet);
        }
    } else {
        product->setFont(FontVariant::Default);
    }
}

bool C525FMCProfile::IsEligible() {
    auto datarefManager = Dataref::getInstance();
    return datarefManager->exists("uns1/cdu1/text_line_0") && datarefManager->exists("afm/cj/autopilot/alt");
}

const std::vector<std::string> &C525FMCProfile::displayDatarefs() const {
    static std::vector<std::string> refs;
    if (refs.empty()) {
        for (int i = 0; i < kScreenLines; ++i) {
            refs.push_back(kCdu + "/text_line_" + std::to_string(i));
            refs.push_back(kCdu + "/style_line_" + std::to_string(i));
        }
    }
    return refs;
}

const std::vector<FMCButtonDef> &C525FMCProfile::buttonDefs() const {
    static const std::vector<FMCButtonDef> buttons = {
        {FMCKey::LSK1L, kFms + "/lsk_l1", FMCDatarefType::EXECUTE_CMD_ONCE},
        {FMCKey::LSK2L, kFms + "/lsk_l2", FMCDatarefType::EXECUTE_CMD_ONCE},
        {FMCKey::LSK3L, kFms + "/lsk_l3", FMCDatarefType::EXECUTE_CMD_ONCE},
        {FMCKey::LSK4L, kFms + "/lsk_l4", FMCDatarefType::EXECUTE_CMD_ONCE},
        {FMCKey::LSK5L, kFms + "/lsk_l5", FMCDatarefType::EXECUTE_CMD_ONCE},
        {FMCKey::LSK1R, kFms + "/lsk_r1", FMCDatarefType::EXECUTE_CMD_ONCE},
        {FMCKey::LSK2R, kFms + "/lsk_r2", FMCDatarefType::EXECUTE_CMD_ONCE},
        {FMCKey::LSK3R, kFms + "/lsk_r3", FMCDatarefType::EXECUTE_CMD_ONCE},
        {FMCKey::LSK4R, kFms + "/lsk_r4", FMCDatarefType::EXECUTE_CMD_ONCE},
        {FMCKey::LSK5R, kFms + "/lsk_r5", FMCDatarefType::EXECUTE_CMD_ONCE},

        // UNS-1 only has 5 LSKs per side — LSK6 mapped empty
        {FMCKey::LSK6L, ""},
        {FMCKey::LSK6R, ""},

        {std::vector<FMCKey>{FMCKey::MCDU_DIR, FMCKey::PFP_INIT_REF}, kFms + "/dto", FMCDatarefType::EXECUTE_CMD_ONCE},
        {std::vector<FMCKey>{FMCKey::MCDU_PERF, FMCKey::PFP3_N1_LIMIT}, kFms + "/perf", FMCDatarefType::EXECUTE_CMD_ONCE},
        {FMCKey::MCDU_DATA, kFms + "/data", FMCDatarefType::EXECUTE_CMD_ONCE},
        {FMCKey::MCDU_FPLN, kFms + "/fpl", FMCDatarefType::EXECUTE_CMD_ONCE},
        {FMCKey::PFP_LEGS, kFms + "/fpl", FMCDatarefType::EXECUTE_CMD_ONCE},
        {FMCKey::PFP_ROUTE, kFms + "/fpl", FMCDatarefType::EXECUTE_CMD_ONCE},
        {std::vector<FMCKey>{FMCKey::MCDU_RAD_NAV, FMCKey::PFP4_NAV_RAD, FMCKey::PFP7_NAV_RAD}, kFms + "/nav", FMCDatarefType::EXECUTE_CMD_ONCE},
        {FMCKey::MCDU_FUEL_PRED, kFms + "/fuel", FMCDatarefType::EXECUTE_CMD_ONCE},
        {std::vector<FMCKey>{FMCKey::MCDU_ATC_COMM, FMCKey::PFP4_ATC}, kFms + "/tune", FMCDatarefType::EXECUTE_CMD_ONCE},
        {std::vector<FMCKey>{FMCKey::MCDU_INIT, FMCKey::PFP4_VNAV, FMCKey::PFP7_VNAV}, kFms + "/vnav", FMCDatarefType::EXECUTE_CMD_ONCE},
        {std::vector<FMCKey>{FMCKey::MCDU_AIRPORT, FMCKey::PFP_DEP_ARR}, kFms + "/list", FMCDatarefType::EXECUTE_CMD_ONCE},
        {FMCKey::MENU, kFms + "/menu", FMCDatarefType::EXECUTE_CMD_ONCE},
        {FMCKey::PROG, kFms + "/msg", FMCDatarefType::EXECUTE_CMD_ONCE},
        {FMCKey::PFP_EXEC, kFms + "/key_enter", FMCDatarefType::EXECUTE_CMD_ONCE},
        {FMCKey::MCDU_OVERFLY, kFms + "/key_enter", FMCDatarefType::EXECUTE_CMD_ONCE},
        {FMCKey::MCDU_EMPTY_TOP_RIGHT, kFms + "/pwr", FMCDatarefType::EXECUTE_CMD_ONCE},
        {std::vector<FMCKey>{FMCKey::CLR, FMCKey::PFP_DEL}, kFms + "/key_back", FMCDatarefType::EXECUTE_CMD_ONCE},
        {FMCKey::KEY0, kFms + "/key_0", FMCDatarefType::EXECUTE_CMD_ONCE},
        {FMCKey::KEY1, kFms + "/key_1", FMCDatarefType::EXECUTE_CMD_ONCE},
        {FMCKey::KEY2, kFms + "/key_2", FMCDatarefType::EXECUTE_CMD_ONCE},
        {FMCKey::KEY3, kFms + "/key_3", FMCDatarefType::EXECUTE_CMD_ONCE},
        {FMCKey::KEY4, kFms + "/key_4", FMCDatarefType::EXECUTE_CMD_ONCE},
        {FMCKey::KEY5, kFms + "/key_5", FMCDatarefType::EXECUTE_CMD_ONCE},
        {FMCKey::KEY6, kFms + "/key_6", FMCDatarefType::EXECUTE_CMD_ONCE},
        {FMCKey::KEY7, kFms + "/key_7", FMCDatarefType::EXECUTE_CMD_ONCE},
        {FMCKey::KEY8, kFms + "/key_8", FMCDatarefType::EXECUTE_CMD_ONCE},
        {FMCKey::KEY9, kFms + "/key_9", FMCDatarefType::EXECUTE_CMD_ONCE},
        {FMCKey::KEYA, kFms + "/key_A", FMCDatarefType::EXECUTE_CMD_ONCE},
        {FMCKey::KEYB, kFms + "/key_B", FMCDatarefType::EXECUTE_CMD_ONCE},
        {FMCKey::KEYC, kFms + "/key_C", FMCDatarefType::EXECUTE_CMD_ONCE},
        {FMCKey::KEYD, kFms + "/key_D", FMCDatarefType::EXECUTE_CMD_ONCE},
        {FMCKey::KEYE, kFms + "/key_E", FMCDatarefType::EXECUTE_CMD_ONCE},
        {FMCKey::KEYF, kFms + "/key_F", FMCDatarefType::EXECUTE_CMD_ONCE},
        {FMCKey::KEYG, kFms + "/key_G", FMCDatarefType::EXECUTE_CMD_ONCE},
        {FMCKey::KEYH, kFms + "/key_H", FMCDatarefType::EXECUTE_CMD_ONCE},
        {FMCKey::KEYI, kFms + "/key_I", FMCDatarefType::EXECUTE_CMD_ONCE},
        {FMCKey::KEYJ, kFms + "/key_J", FMCDatarefType::EXECUTE_CMD_ONCE},
        {FMCKey::KEYK, kFms + "/key_K", FMCDatarefType::EXECUTE_CMD_ONCE},
        {FMCKey::KEYL, kFms + "/key_L", FMCDatarefType::EXECUTE_CMD_ONCE},
        {FMCKey::KEYM, kFms + "/key_M", FMCDatarefType::EXECUTE_CMD_ONCE},
        {FMCKey::KEYN, kFms + "/key_N", FMCDatarefType::EXECUTE_CMD_ONCE},
        {FMCKey::KEYO, kFms + "/key_O", FMCDatarefType::EXECUTE_CMD_ONCE},
        {FMCKey::KEYP, kFms + "/key_P", FMCDatarefType::EXECUTE_CMD_ONCE},
        {FMCKey::KEYQ, kFms + "/key_Q", FMCDatarefType::EXECUTE_CMD_ONCE},
        {FMCKey::KEYR, kFms + "/key_R", FMCDatarefType::EXECUTE_CMD_ONCE},
        {FMCKey::KEYS, kFms + "/key_S", FMCDatarefType::EXECUTE_CMD_ONCE},
        {FMCKey::KEYT, kFms + "/key_T", FMCDatarefType::EXECUTE_CMD_ONCE},
        {FMCKey::KEYU, kFms + "/key_U", FMCDatarefType::EXECUTE_CMD_ONCE},
        {FMCKey::KEYV, kFms + "/key_V", FMCDatarefType::EXECUTE_CMD_ONCE},
        {FMCKey::KEYW, kFms + "/key_W", FMCDatarefType::EXECUTE_CMD_ONCE},
        {FMCKey::KEYX, kFms + "/key_X", FMCDatarefType::EXECUTE_CMD_ONCE},
        {FMCKey::KEYY, kFms + "/key_Y", FMCDatarefType::EXECUTE_CMD_ONCE},
        {FMCKey::KEYZ, kFms + "/key_Z", FMCDatarefType::EXECUTE_CMD_ONCE},
        {FMCKey::PLUSMINUS, kFms + "/plus_minus", FMCDatarefType::EXECUTE_CMD_ONCE},

        {std::vector<FMCKey>{FMCKey::MCDU_PAGE_UP, FMCKey::PAGE_PREV}, kFms + "/prev", FMCDatarefType::EXECUTE_CMD_ONCE},
        {std::vector<FMCKey>{FMCKey::MCDU_PAGE_DOWN, FMCKey::PAGE_NEXT}, kFms + "/next", FMCDatarefType::EXECUTE_CMD_ONCE},

        // Remaining keys — no UNS-1 equivalent
        {FMCKey::BRIGHTNESS_UP, ""},
        {FMCKey::BRIGHTNESS_DOWN, ""},
        {FMCKey::MCDU_EMPTY_BOTTOM_LEFT, ""},
        {FMCKey::MCDU_SEC_FPLN, ""},
        {FMCKey::SLASH, ""},
        {FMCKey::PERIOD, ""},
        {FMCKey::SPACE, ""},
        {FMCKey::PFP_HOLD, ""},
        {FMCKey::PFP_FIX, ""},
        {FMCKey::PFP3_CLB, ""},
        {FMCKey::PFP3_CRZ, ""},
        {FMCKey::PFP3_DES, ""},
        {FMCKey::PFP4_FMC_COMM, ""},
        {FMCKey::PFP7_ALTN, ""},
        {FMCKey::PFP7_FMC_COMM, ""},
    };

    return buttons;
}

const std::unordered_map<FMCKey, const FMCButtonDef *> &C525FMCProfile::buttonKeyMap() const {
    static std::unordered_map<FMCKey, const FMCButtonDef *> map;
    if (map.empty()) {
        const auto &buttons = buttonDefs();
        for (const auto &button : buttons) {
            std::visit([&](auto &&k) {
                using T = std::decay_t<decltype(k)>;
                if constexpr (std::is_same_v<T, FMCKey>) {
                    map[k] = &button;
                } else {
                    for (const auto &key : k) {
                        map[key] = &button;
                    }
                }
            },
                button.key);
        }
    }
    return map;
}

const std::map<char, FMCTextColor> &C525FMCProfile::colorMap() const {
    return UNS1Decode::colorMap();
}

void C525FMCProfile::mapCharacter(std::vector<uint8_t> *buffer, uint8_t character, bool isFontSmall) {
    UNS1Decode::mapCharacter(buffer, character);
}

void C525FMCProfile::updatePage(std::vector<std::vector<char>> &page) {
    UNS1Decode::updatePage(product, page, kCdu, kScreenLines);
}

void C525FMCProfile::buttonPressed(const FMCButtonDef *button, XPLMCommandPhase phase) {
    if (!button || button->dataref.empty() || phase == xplm_CommandContinue) {
        return;
    }

    auto datarefManager = Dataref::getInstance();
    if (button->datarefType == FMCDatarefType::SET_VALUE || button->datarefType == FMCDatarefType::SET_VALUE_PHASED) {
        double value = std::fabs(button->value) < std::numeric_limits<double>::epsilon() ? 1.0 : button->value;
        if (button->datarefType == FMCDatarefType::SET_VALUE && phase != xplm_CommandBegin) {
            return;
        }

        datarefManager->set<double>(button->dataref.c_str(), phase == xplm_CommandBegin ? value : 0.0);
    } else if (phase == xplm_CommandBegin && button->datarefType == FMCDatarefType::ADJUST_VALUE) {
        double currentValue = datarefManager->get<double>(button->dataref.c_str());
        datarefManager->set<double>(button->dataref.c_str(), currentValue + button->value);
    } else if (phase == xplm_CommandBegin && button->datarefType == FMCDatarefType::EXECUTE_MULTIPLE_CMD_ONCE) {
        std::stringstream ss(button->dataref);
        std::string item;
        std::vector<std::string> commands;
        while (std::getline(ss, item, ',')) {
            commands.push_back(item);
        }

        for (const auto &cmd : commands) {
            datarefManager->executeCommand(cmd.c_str());
        }
    } else if (phase == xplm_CommandBegin && button->datarefType == FMCDatarefType::EXECUTE_CMD_ONCE) {
        datarefManager->executeCommand(button->dataref.c_str());
    } else {
        datarefManager->executeCommand(button->dataref.c_str(), phase);
    }
}
