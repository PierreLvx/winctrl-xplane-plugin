#include "a220-fmc-profile.h"

#include "dataref.h"
#include "product-fmc.h"

#include <algorithm>
#include <cmath>
#include <cstring>

// Deanarica A220-300 (SASL). The CDU is a thin wrapper around the stock X-Plane FMS: the
// a220/fmc/key/* commands animate the 3D keys and forward to sim/FMS/*, and the display
// comes from sim/cockpit2/radios/indicators/fms_cdu1_*. Keys are routed through the
// aircraft commands where they exist so the virtual CDU stays in sync.

A220FMCProfile::A220FMCProfile(ProductFMC *product) : FMCAircraftProfile(product) {
    product->setAllLedsEnabled(false);
    product->setFont(FontVariant::Default);

    // The aircraft has no CDU dimming, so brightness simply follows avionics power.
    Dataref::getInstance()->monitorExistingDataref<int>("a220/electrical/power_status", [product](int powered) {
        uint8_t target = powered ? 255 : 0;
        product->setLedBrightness(FMCLed::BACKLIGHT, target);
        product->setLedBrightness(FMCLed::SCREEN_BACKLIGHT, target);
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<bool>("sim/cockpit2/radios/indicators/fms_exec_light_pilot", [product](bool lit) {
        product->setLedBrightness(FMCLed::PFP_EXEC, lit ? 1 : 0);
    },
        this);

    Dataref::getInstance()->executeChangedCallbacksForDataref("a220/electrical/power_status");
}

bool A220FMCProfile::IsEligible() {
    return Dataref::getInstance()->exists("a220/fmc/key/button_EXEC");
}

const std::vector<std::string> &A220FMCProfile::displayDatarefs() const {
    static std::unordered_map<FMCDeviceVariant, std::vector<std::string>> cache;

    return cache.try_emplace(FMCDeviceVariant::VARIANT_CAPTAIN,
                    std::vector<std::string>{
                        "sim/cockpit2/radios/indicators/fms_cdu1_text_line0",
                        "sim/cockpit2/radios/indicators/fms_cdu1_text_line1",
                        "sim/cockpit2/radios/indicators/fms_cdu1_text_line2",
                        "sim/cockpit2/radios/indicators/fms_cdu1_text_line3",
                        "sim/cockpit2/radios/indicators/fms_cdu1_text_line4",
                        "sim/cockpit2/radios/indicators/fms_cdu1_text_line5",
                        "sim/cockpit2/radios/indicators/fms_cdu1_text_line6",
                        "sim/cockpit2/radios/indicators/fms_cdu1_text_line7",
                        "sim/cockpit2/radios/indicators/fms_cdu1_text_line8",
                        "sim/cockpit2/radios/indicators/fms_cdu1_text_line9",
                        "sim/cockpit2/radios/indicators/fms_cdu1_text_line10",
                        "sim/cockpit2/radios/indicators/fms_cdu1_text_line11",
                        "sim/cockpit2/radios/indicators/fms_cdu1_text_line12",
                        "sim/cockpit2/radios/indicators/fms_cdu1_text_line13",
                        "sim/cockpit2/radios/indicators/fms_cdu1_text_line14",
                        "sim/cockpit2/radios/indicators/fms_cdu1_text_line15"})
        .first->second;
}

const std::vector<FMCButtonDef> &A220FMCProfile::buttonDefs() const {
    static std::unordered_map<FMCDeviceVariant, std::vector<FMCButtonDef>> cache;

    return cache.try_emplace(FMCDeviceVariant::VARIANT_CAPTAIN,
                    std::vector<FMCButtonDef>{
                        // The A220 CDU drives line selects with a cursor plus ENTER and has no
                        // physical LSKs, but the stock FMS underneath accepts them directly.
                        {FMCKey::LSK1L, "sim/FMS/ls_1l"},
                        {FMCKey::LSK2L, "sim/FMS/ls_2l"},
                        {FMCKey::LSK3L, "sim/FMS/ls_3l"},
                        {FMCKey::LSK4L, "sim/FMS/ls_4l"},
                        {FMCKey::LSK5L, "sim/FMS/ls_5l"},
                        {FMCKey::LSK6L, "sim/FMS/ls_6l"},
                        {FMCKey::LSK1R, "sim/FMS/ls_1r"},
                        {FMCKey::LSK2R, "sim/FMS/ls_2r"},
                        {FMCKey::LSK3R, "sim/FMS/ls_3r"},
                        {FMCKey::LSK4R, "sim/FMS/ls_4r"},
                        {FMCKey::LSK5R, "sim/FMS/ls_5r"},
                        {FMCKey::LSK6R, "sim/FMS/ls_6r"},

                        // Page keys, named after the A220 legend with the stock page in brackets.
                        {std::vector<FMCKey>{FMCKey::MCDU_INIT, FMCKey::PFP_INIT_REF}, "a220/fmc/key/MSG"},   // INDEX
                        {std::vector<FMCKey>{FMCKey::MCDU_FPLN, FMCKey::PFP_ROUTE}, "a220/fmc/key/ROUTE"},    // FPLN
                        {std::vector<FMCKey>{FMCKey::MCDU_PERF, FMCKey::PFP3_CLB}, "a220/fmc/key/DIR"},       // CLB
                        {FMCKey::PFP3_DES, "a220/fmc/key/DEPAPPR"},                                           // DES
                        {FMCKey::MCDU_DIR, "a220/fmc/key/MAP"},                                               // DIR INTC
                        {FMCKey::PFP_LEGS, "a220/fmc/key/FMS"},                                               // LEGS
                        {std::vector<FMCKey>{FMCKey::MCDU_AIRPORT, FMCKey::PFP_DEP_ARR}, "a220/fmc/key/CMS"}, // DEP ARR
                        {FMCKey::PFP_HOLD, "a220/fmc/key/CHKL"},                                              // HOLD
                        {std::vector<FMCKey>{FMCKey::PROG, FMCKey::PFP3_CRZ}, "a220/fmc/key/SYN"},            // PROG
                        {std::vector<FMCKey>{FMCKey::MCDU_DATA, FMCKey::PFP_FIX}, "a220/fmc/key/DATA"},       // FIX
                        {std::vector<FMCKey>{FMCKey::PFP_EXEC, FMCKey::MCDU_EMPTY_TOP_RIGHT}, "a220/fmc/key/EXEC"},
                        {std::vector<FMCKey>{FMCKey::MCDU_OVERFLY, FMCKey::PFP_DEL}, "a220/fmc/key/CNCL"}, // DELETE

                        // Paging. PREV/NEXT change page, the arrow keys scroll within one.
                        {FMCKey::PAGE_PREV, "a220/fmc/key/PREV"},
                        {FMCKey::PAGE_NEXT, "a220/fmc/key/NEXT"},
                        {FMCKey::MCDU_PAGE_UP, "sim/FMS/up"},
                        {FMCKey::MCDU_PAGE_DOWN, "sim/FMS/down"},

                        {FMCKey::KEY0, "a220/fmc/key/0"},
                        {FMCKey::KEY1, "a220/fmc/key/1"},
                        {FMCKey::KEY2, "a220/fmc/key/2"},
                        {FMCKey::KEY3, "a220/fmc/key/3"},
                        {FMCKey::KEY4, "a220/fmc/key/4"},
                        {FMCKey::KEY5, "a220/fmc/key/5"},
                        {FMCKey::KEY6, "a220/fmc/key/6"},
                        {FMCKey::KEY7, "a220/fmc/key/7"},
                        {FMCKey::KEY8, "a220/fmc/key/8"},
                        {FMCKey::KEY9, "a220/fmc/key/9"},
                        {FMCKey::KEYA, "a220/fmc/key/a"},
                        {FMCKey::KEYB, "a220/fmc/key/b"},
                        {FMCKey::KEYC, "a220/fmc/key/c"},
                        {FMCKey::KEYD, "a220/fmc/key/d"},
                        {FMCKey::KEYE, "a220/fmc/key/e"},
                        {FMCKey::KEYF, "a220/fmc/key/f"},
                        {FMCKey::KEYG, "a220/fmc/key/g"},
                        {FMCKey::KEYH, "a220/fmc/key/h"},
                        {FMCKey::KEYI, "a220/fmc/key/i"},
                        {FMCKey::KEYJ, "a220/fmc/key/j"},
                        {FMCKey::KEYK, "a220/fmc/key/k"},
                        {FMCKey::KEYL, "a220/fmc/key/l"},
                        {FMCKey::KEYM, "a220/fmc/key/m"},
                        {FMCKey::KEYN, "a220/fmc/key/n"},
                        {FMCKey::KEYO, "a220/fmc/key/o"},
                        {FMCKey::KEYP, "a220/fmc/key/p"},
                        {FMCKey::KEYQ, "a220/fmc/key/q"},
                        {FMCKey::KEYR, "a220/fmc/key/r"},
                        {FMCKey::KEYS, "a220/fmc/key/s"},
                        {FMCKey::KEYT, "a220/fmc/key/t"},
                        {FMCKey::KEYU, "a220/fmc/key/u"},
                        {FMCKey::KEYV, "a220/fmc/key/v"},
                        {FMCKey::KEYW, "a220/fmc/key/w"},
                        {FMCKey::KEYX, "a220/fmc/key/x"},
                        {FMCKey::KEYY, "a220/fmc/key/y"},
                        {FMCKey::KEYZ, "a220/fmc/key/z"},
                        {FMCKey::PERIOD, "a220/fmc/key/PERIOD"},
                        {FMCKey::PLUSMINUS, "a220/fmc/key/PLUSMINUS"},
                        {FMCKey::SLASH, "a220/fmc/key/SLASH"},
                        {FMCKey::CLR, "a220/fmc/key/CLRDEL"},

                        // The A220 CDU model has no SP key, so this goes straight to the FMS.
                        {FMCKey::SPACE, "sim/FMS/key_space"},
                    })
        .first->second;
}

const std::unordered_map<FMCKey, const FMCButtonDef *> &A220FMCProfile::buttonKeyMap() const {
    static std::unordered_map<FMCDeviceVariant, std::unordered_map<FMCKey, const FMCButtonDef *>> cache;

    auto it = cache.find(product->deviceVariant);
    if (it == cache.end()) {
        std::unordered_map<FMCKey, const FMCButtonDef *> map;
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
        it = cache.emplace(product->deviceVariant, std::move(map)).first;
    }
    return it->second;
}

const std::map<char, FMCTextColor> &A220FMCProfile::colorMap() const {
    static const std::map<char, FMCTextColor> colMap = {
        {0x00, FMCTextColor::COLOR_WHITE},
        {0x01, FMCTextColor::COLOR_CYAN},
        {0x03, FMCTextColor::COLOR_YELLOW},
        {0x04, FMCTextColor::COLOR_GREEN},
        {0x05, FMCTextColor::COLOR_MAGENTA},
        {0x06, FMCTextColor::COLOR_AMBER},
    };
    return colMap;
}

void A220FMCProfile::mapCharacter(std::vector<uint8_t> *buffer, uint8_t character, bool isFontSmall) {
    switch (character) {
        case '#':
            buffer->insert(buffer->end(), FMCSpecialCharacter::OUTLINED_SQUARE.begin(), FMCSpecialCharacter::OUTLINED_SQUARE.end());
            break;

        case '<':
            if (isFontSmall) {
                buffer->insert(buffer->end(), FMCSpecialCharacter::ARROW_LEFT.begin(), FMCSpecialCharacter::ARROW_LEFT.end());
            } else {
                buffer->push_back(character);
            }
            break;

        case '>':
            if (isFontSmall) {
                buffer->insert(buffer->end(), FMCSpecialCharacter::ARROW_RIGHT.begin(), FMCSpecialCharacter::ARROW_RIGHT.end());
            } else {
                buffer->push_back(character);
            }
            break;

        case 30: // Up arrow
            if (isFontSmall) {
                buffer->insert(buffer->end(), FMCSpecialCharacter::ARROW_UP.begin(), FMCSpecialCharacter::ARROW_UP.end());
            }
            break;

        case 31: // Down arrow
            if (isFontSmall) {
                buffer->insert(buffer->end(), FMCSpecialCharacter::ARROW_DOWN.begin(), FMCSpecialCharacter::ARROW_DOWN.end());
            } else {
                buffer->push_back(character);
            }
            break;

        case '`':
            buffer->insert(buffer->end(), FMCSpecialCharacter::DEGREES.begin(), FMCSpecialCharacter::DEGREES.end());
            break;

        default:
            buffer->push_back(character);
            break;
    }
}

void A220FMCProfile::updatePage(std::vector<std::vector<char>> &page) {
    page = std::vector<std::vector<char>>(ProductFMC::PageLines, std::vector<char>(ProductFMC::PageCharsPerLine * ProductFMC::PageBytesPerChar, ' '));

    auto datarefManager = Dataref::getInstance();
    for (int lineNum = 0; lineNum < std::min(ProductFMC::PageLines, (unsigned int) 16); ++lineNum) {
        std::string textDataref = "sim/cockpit2/radios/indicators/fms_cdu1_text_line" + std::to_string(lineNum);
        std::string styleDataref = "sim/cockpit2/radios/indicators/fms_cdu1_style_line" + std::to_string(lineNum);

        std::string text = datarefManager->getCached<std::string>(textDataref.c_str());
        if (text.empty()) {
            continue;
        }

        std::vector<unsigned char> styleBytes = datarefManager->getCached<std::vector<unsigned char>>(styleDataref.c_str());

        // Replace all special characters with placeholders
        const std::vector<std::pair<std::string, unsigned char>> symbols = {
            {"\u2190", '<'},
            {"\u2192", '>'},
            {"\u2191", 30},
            {"\u2193", 31},
            {"\u2610", '#'},
            {"\u00B0", '`'}};

        for (const auto &symbol : symbols) {
            size_t pos = 0;
            while ((pos = text.find(symbol.first, pos)) != std::string::npos) {
                text.replace(pos, symbol.first.length(), std::string(1, static_cast<char>(symbol.second)));
                pos += 1;
            }
        }

        for (size_t i = 0; i < text.size(); ++i) {
            if (static_cast<unsigned char>(text[i]) > 127) {
                text[i] = '?';
            }
        }

        for (int i = 0; i < text.size() && i < ProductFMC::PageCharsPerLine; ++i) {
            char c = text[i];
            if (c == 0x00) {
                continue;
            }

            // The aircraft reads the same style byte as colour = byte & 0x0F (FMC_update.lua).
            unsigned char styleByte = (i < styleBytes.size()) ? styleBytes[i] : 0x00;
            bool fontSmall = (styleByte & 0xF0) == 0x00;

            int displayLine = lineNum;
            if (displayLine >= ProductFMC::PageLines) {
                break;
            }

            product->writeLineToPage(page, displayLine, i, std::string(1, c), styleByte & 0x0F, fontSmall);
        }
    }
}

void A220FMCProfile::buttonPressed(const FMCButtonDef *button, XPLMCommandPhase phase) {
    if (!button || button->dataref.empty() || phase == xplm_CommandContinue) {
        return;
    }

    Dataref::getInstance()->executeCommand(button->dataref.c_str(), phase);
}
