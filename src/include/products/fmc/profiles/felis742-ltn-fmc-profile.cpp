#include "felis742-ltn-fmc-profile.h"

#include "dataref.h"
#include "product-fmc.h"
#include "logger.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

// Felis 747-200 LTN-92. Display: five 16-char ASCII strings per unit under
// B742/LTN{u}/display_text_line1..5, with one global colour selector
// (B742/LTN/color_option: 0 = green, 1 = amber). Keys: B742/LTN{u}/but_letters
// (int array[28], momentary) plus but_A_N, but_CLR, but_ENT (int, momentary). The
// aircraft actuates them via ATTR_manip_push with no commands, so every key is
// driven by writing 1 on press and 0 on release.
//
// but_letters indices are the alphabetical faceplate order (== physical reading
// order): 0..25 = A..Z, 26 = "0", 27 = "( )/HLD". Digits and directions share the
// alpha keys: 1=E[4] 2=F[5] 3=G[6] 4=L[11] 5=M[12] 6=N[13] 7=S[18] 8=T[19]
// 9=U[20] 0=[26]; slew up = R[17], slew down = X[23], EXP = Z[25], HLD = [27].
static const std::string kNs = "B742/LTN";

static int unitNumberForVariant(FMCDeviceVariant variant) {
    switch (variant) {
        case FMCDeviceVariant::VARIANT_FIRSTOFFICER:
            return 2;
        case FMCDeviceVariant::VARIANT_OBSERVER:
            return 3;
        case FMCDeviceVariant::VARIANT_CAPTAIN:
        default:
            return 1;
    }
}

Felis742LTNFMCProfile::Felis742LTNFMCProfile(ProductFMC *product) : FMCAircraftProfile(product) {
    int unitNumber = unitNumberForVariant(product->deviceVariant);
    unitIndex = unitNumber - 1;
    unitPrefix = kNs + std::to_string(unitNumber); // "B742/LTN1"

    // The five screen lines plus the global colour selector (so a colour change
    // also forces a redraw).
    displayRefs = {
        unitPrefix + "/display_text_line1",
        unitPrefix + "/display_text_line2",
        unitPrefix + "/display_text_line3",
        unitPrefix + "/display_text_line4",
        unitPrefix + "/display_text_line5",
        kNs + "/color_option",
    };

    buildButtons();

    product->setAllLedsEnabled(false);

    // Screen backlight follows the LTN's own brightness (float[3], per unit).
    int idx = unitIndex;
    Dataref::getInstance()->monitorExistingDataref<std::vector<float>>((kNs + "_INT/brightness").c_str(), [product, idx](const std::vector<float> &brightness) {
        if (idx >= (int) brightness.size()) {
            return;
        }
        uint8_t target = static_cast<uint8_t>(std::clamp(brightness[idx], 0.0f, 1.0f) * 255);
        product->setLedBrightness(FMCLed::SCREEN_BACKLIGHT, target);
    },
        this);

    // Key-legend backlight follows the panel brightness rheostat, gated on power
    Dataref::getInstance()->monitorExistingDataref<std::vector<float>>("sim/cockpit2/electrical/instrument_brightness_ratio", [this, product](const std::vector<float> &brightness) {
        bool hasPower = Dataref::getInstance()->get<bool>("sim/cockpit/electrical/battery_on");

        float panel = 0.0f;
        for (size_t i = 0; i < brightness.size() && i < 4; i++) {
            panel = std::max(panel, brightness[i]);
        }

        uint8_t backlight = hasPower ? static_cast<uint8_t>(panel * 255) : 0;
        if (backlight == lastBacklightSent) {
            return;
        }
        lastBacklightSent = backlight;
        product->setLedBrightness(FMCLed::BACKLIGHT, backlight);
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<bool>("sim/cockpit/electrical/battery_on", [](bool) {
        Dataref::getInstance()->executeChangedCallbacksForDataref("sim/cockpit2/electrical/instrument_brightness_ratio");
    },
        this);

    // Annunciator lamps (float, per unit): OFS -> offset LED, WRN -> FAIL, ALR -> MSG.
    Dataref::getInstance()->monitorExistingDataref<float>((unitPrefix + "/OFS_lamp").c_str(), [product](float lit) {
        product->setLedBrightness(FMCLed::PFP_OFST, lit > 0.5f ? 1 : 0);
    },
        this);
    Dataref::getInstance()->monitorExistingDataref<float>((unitPrefix + "/WRN_lamp").c_str(), [product](float lit) {
        product->setLedBrightness(FMCLed::PFP_FAIL, lit > 0.5f ? 1 : 0);
        product->setLedBrightness(FMCLed::MCDU_FAIL, lit > 0.5f ? 1 : 0);
    },
        this);
    Dataref::getInstance()->monitorExistingDataref<float>((unitPrefix + "/ALR_lamp").c_str(), [product](float lit) {
        product->setLedBrightness(FMCLed::PFP_MSG, lit > 0.5f ? 1 : 0);
    },
        this);

    // The LTN-92 dot-matrix screen font should be placed as LTN.xpwwf; fall back to
    // the built-in 747 font if it has not been deployed yet.
    auto font = Font::GlyphData("LTN.xpwwf", product->identifierByte, product->hardwareType);
    if (!font.empty()) {
        for (auto &packet : font) {
            product->writeData(packet);
        }
    } else {
        product->setFont(FontVariant::Font744);
    }

    Logger::getInstance()->info("FMC: Felis 747-200 LTN-92 profile active (unit LTN%i)\n", unitNumber);
    Dataref::getInstance()->executeChangedCallbacksForDataref("sim/cockpit2/electrical/instrument_brightness_ratio");
}

void Felis742LTNFMCProfile::buildButtons() {
    auto L = [this](int index) { return unitPrefix + "/but_letters[" + std::to_string(index) + "]"; };
    auto S = [this](const std::string &name) { return unitPrefix + "/" + name; };
    using DT = FMCDatarefType;

    // Alpha keys A-Z -> but_letters[0..25]; digit, slew and brightness aliases are
    // folded into whichever alpha key the LTN prints them on. Page functions (POS,
    // CRS, ...) live on those same alpha keys, so mapping the letters covers them.
    buttons = {
        {FMCKey::KEYA, L(0), DT::SET_VALUE_PHASED},                                                              // A / POS
        {FMCKey::KEYB, L(1), DT::SET_VALUE_PHASED},                                                              // B / CRS
        {std::vector<FMCKey>{FMCKey::KEYC, FMCKey::PFP_LEGS}, L(2), DT::SET_VALUE_PHASED},                       // C / LEG
        {std::vector<FMCKey>{FMCKey::KEYD, FMCKey::BRIGHTNESS_UP}, L(3), DT::SET_VALUE_PHASED},                  // D / BRT
        {std::vector<FMCKey>{FMCKey::KEYE, FMCKey::KEY1}, L(4), DT::SET_VALUE_PHASED},                           // E / 1
        {std::vector<FMCKey>{FMCKey::KEYF, FMCKey::KEY2}, L(5), DT::SET_VALUE_PHASED},                           // F / 2 (N)
        {std::vector<FMCKey>{FMCKey::KEYG, FMCKey::KEY3}, L(6), DT::SET_VALUE_PHASED},                           // G / 3
        {FMCKey::KEYH, L(7), DT::SET_VALUE_PHASED},                                                              // H / WPT
        {std::vector<FMCKey>{FMCKey::KEYI, FMCKey::MCDU_FPLN, FMCKey::PFP_ROUTE}, L(8), DT::SET_VALUE_PHASED},   // I / FPL
        {std::vector<FMCKey>{FMCKey::KEYJ, FMCKey::MCDU_DIR, FMCKey::PFP_INIT_REF}, L(9), DT::SET_VALUE_PHASED}, // J / DIR
        {std::vector<FMCKey>{FMCKey::KEYK, FMCKey::BRIGHTNESS_DOWN}, L(10), DT::SET_VALUE_PHASED},               // K / DIM
        {std::vector<FMCKey>{FMCKey::KEYL, FMCKey::KEY4}, L(11), DT::SET_VALUE_PHASED},                          // L / 4 (W)
        {std::vector<FMCKey>{FMCKey::KEYM, FMCKey::KEY5}, L(12), DT::SET_VALUE_PHASED},                          // M / 5
        {std::vector<FMCKey>{FMCKey::KEYN, FMCKey::KEY6}, L(13), DT::SET_VALUE_PHASED},                          // N / 6 (E)
        {std::vector<FMCKey>{FMCKey::KEYO, FMCKey::MCDU_DATA}, L(14), DT::SET_VALUE_PHASED},                     // O / DATA
        {FMCKey::KEYP, L(15), DT::SET_VALUE_PHASED},                                                             // P / CAT
        {FMCKey::KEYQ, L(16), DT::SET_VALUE_PHASED},                                                             // Q / RMT
        {std::vector<FMCKey>{FMCKey::KEYR, FMCKey::PAGE_PREV, FMCKey::MCDU_PAGE_UP}, L(17), DT::SET_VALUE_PHASED},     // R / slew up
        {std::vector<FMCKey>{FMCKey::KEYS, FMCKey::KEY7}, L(18), DT::SET_VALUE_PHASED},                          // S / 7 (L)
        {std::vector<FMCKey>{FMCKey::KEYT, FMCKey::KEY8}, L(19), DT::SET_VALUE_PHASED},                          // T / 8 (S)
        {std::vector<FMCKey>{FMCKey::KEYU, FMCKey::KEY9}, L(20), DT::SET_VALUE_PHASED},                          // U / 9 (R)
        {std::vector<FMCKey>{FMCKey::KEYV, FMCKey::PROG}, L(21), DT::SET_VALUE_PHASED},                          // V / STS
        {FMCKey::KEYW, L(22), DT::SET_VALUE_PHASED},                                                             // W
        {FMCKey::KEYX, L(23), DT::SET_VALUE_PHASED},                                                             // X
        {std::vector<FMCKey>{FMCKey::KEYY, FMCKey::PAGE_NEXT, FMCKey::MCDU_PAGE_DOWN}, L(24), DT::SET_VALUE_PHASED},   // Y / slew down
        {FMCKey::KEYZ, L(25), DT::SET_VALUE_PHASED},                                                             // Z / EXP
        {FMCKey::KEY0, L(26), DT::SET_VALUE_PHASED},                                                             // 0
        {FMCKey::PFP_HOLD, L(27), DT::SET_VALUE_PHASED},                                                         // ( ) / HLD

        // Dedicated keys (scalar int datarefs).
        {FMCKey::PLUSMINUS, S("but_A_N"), DT::SET_VALUE_PHASED},                                                 // A/N shift
        {std::vector<FMCKey>{FMCKey::CLR, FMCKey::PFP_DEL}, S("but_CLR"), DT::SET_VALUE_PHASED},                 // CLR
        {std::vector<FMCKey>{FMCKey::PFP_EXEC, FMCKey::MCDU_OVERFLY}, S("but_ENT"), DT::SET_VALUE_PHASED},       // ENT
    };

    keyMap.clear();
    for (const auto &button : buttons) {
        std::visit([&](auto &&k) {
            using T = std::decay_t<decltype(k)>;
            if constexpr (std::is_same_v<T, FMCKey>) {
                keyMap[k] = &button;
            } else {
                for (const auto &key : k) {
                    keyMap[key] = &button;
                }
            }
        },
            button.key);
    }
}

bool Felis742LTNFMCProfile::IsEligible() {
    auto datarefManager = Dataref::getInstance();
    std::string icao = datarefManager->get<std::string>("sim/aircraft/view/acf_ICAO");
    return icao == "B742" && datarefManager->exists("B742/LTN1/display_text_line1");
}

const std::vector<std::string> &Felis742LTNFMCProfile::displayDatarefs() const {
    return displayRefs;
}

const std::vector<FMCButtonDef> &Felis742LTNFMCProfile::buttonDefs() const {
    return buttons;
}

const std::unordered_map<FMCKey, const FMCButtonDef *> &Felis742LTNFMCProfile::buttonKeyMap() const {
    return keyMap;
}

const std::map<char, FMCTextColor> &Felis742LTNFMCProfile::colorMap() const {
    static const std::map<char, FMCTextColor> colMap = {
        {'g', FMCTextColor::COLOR_GREEN},
        {'a', FMCTextColor::COLOR_AMBER},
    };
    return colMap;
}

void Felis742LTNFMCProfile::mapCharacter(std::vector<uint8_t> *buffer, uint8_t character, bool isFontSmall) {
    buffer->push_back(character);
}

void Felis742LTNFMCProfile::updatePage(std::vector<std::vector<char>> &page) {
    page = std::vector<std::vector<char>>(ProductFMC::PageLines, std::vector<char>(ProductFMC::PageCharsPerLine * ProductFMC::PageBytesPerChar, ' '));

    auto datarefManager = Dataref::getInstance();
    int colorOption = datarefManager->getCached<int>((kNs + "/color_option").c_str());
    char color = colorOption == 1 ? 'a' : 'g';

    static constexpr int kScreenLines = 5;
    static constexpr int kScreenCols = 16;
    // Centre the 16-wide screen in the 24-wide grid and lay the five lines on the
    // LSK-aligned content rows (2,4,6,8,10) for even, readable spacing.
    static constexpr int startCol = (ProductFMC::PageCharsPerLine - kScreenCols) / 2; // 4

    // Read all five lines first (NUL-truncated, clamped to the 16-char width) and
    // note whether any of them carries visible content. When the LTN-92 is switched
    // off the sim empties every display_text_line, so an all-blank screen means the
    // unit is off: leave the page all-spaces and draw nothing, not even the decorative
    // dots below, so the physical display goes fully dark.
    std::array<std::string, kScreenLines> lines;
    bool hasContent = false;
    for (int i = 0; i < kScreenLines; ++i) {
        std::string ref = unitPrefix + "/display_text_line" + std::to_string(i + 1);
        std::string text = datarefManager->getCached<std::string>(ref.c_str());

        auto nul = text.find('\0');
        if (nul != std::string::npos) {
            text.resize(nul);
        }
        if ((int) text.size() > kScreenCols) {
            text.resize(kScreenCols);
        }

        if (text.find_first_not_of(' ') != std::string::npos) {
            hasContent = true;
        }
        lines[i] = std::move(text);
    }

    if (!hasContent) {
        return;
    }

    for (int i = 0; i < kScreenLines; ++i) {
        const std::string &text = lines[i];
        int row = 2 + i * 2;
        for (int c = 0; c < (int) text.size(); ++c) {
            char ch = static_cast<char>(std::toupper((unsigned char) text[c]));
            if (ch == ' ') {
                continue;
            }
            product->writeLineToPage(page, row, startCol + c, std::string(1, ch), color, false);
        }
    }

    // Two amber annunciator dots flank the vertical-centre content row (the "third"
    // screen line) of the real LTN-92 bezel. Draw them in the otherwise-empty grid
    // margins on every page as a fixed decorative feature, always amber ('a')
    // regardless of the green/amber screen colour.
    static constexpr int kDotRow = 6;      // content line 3 -> row 2 + 2*2
    static constexpr char kDotGlyph = '*'; // closest dot-like mark in the LTN font
    product->writeLineToPage(page, kDotRow, 1, std::string(1, kDotGlyph), 'a', false);
    product->writeLineToPage(page, kDotRow, ProductFMC::PageCharsPerLine - 2, std::string(1, kDotGlyph), 'a', false);
}

void Felis742LTNFMCProfile::buttonPressed(const FMCButtonDef *button, XPLMCommandPhase phase) {
    if (!button || button->dataref.empty() || phase == xplm_CommandContinue) {
        return;
    }

    auto datarefManager = Dataref::getInstance();
    const std::string &ref = button->dataref;
    int pressed = (phase == xplm_CommandBegin) ? 1 : 0;

    // but_letters is an int array; XPLMFindDataRef cannot resolve a "name[N]"
    // subscript, so actuate one element with a read-modify-write of the whole array.
    auto bracket = ref.find('[');
    if (bracket != std::string::npos && ref.back() == ']') {
        std::string base = ref.substr(0, bracket);
        int index = std::stoi(ref.substr(bracket + 1, ref.size() - bracket - 2));
        std::vector<int> values = datarefManager->get<std::vector<int>>(base.c_str());
        if (index >= 0 && index < (int) values.size()) {
            values[index] = pressed;
            datarefManager->set<std::vector<int>>(base.c_str(), values);
        }
        return;
    }

    // Scalar momentary keys (but_A_N / but_CLR / but_ENT).
    datarefManager->set<double>(ref.c_str(), pressed ? 1.0 : 0.0);
}
