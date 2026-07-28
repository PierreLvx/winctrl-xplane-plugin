#ifndef FELIS742_LTN_FMC_PROFILE_H
#define FELIS742_LTN_FMC_PROFILE_H

#include "fmc-aircraft-profile.h"

#include <string>

// Felis Boeing 747-200F LTN-92 Litton inertial navigation CDU.
// The screen is read as five ASCII strings; the 28-key alpha/numeric
// keypad (but_letters, an int array) plus A/N, CLR and ENT are momentary
// write-datarefs (the aircraft uses ATTR_manip_push, so there are no commands).
class Felis742LTNFMCProfile : public FMCAircraftProfile {
    private:
        std::string unitPrefix;
        int unitIndex = 0;
        std::vector<std::string> displayRefs;
        std::vector<FMCButtonDef> buttons;
        std::unordered_map<FMCKey, const FMCButtonDef *> keyMap;
        int lastBacklightSent = -1;

        void buildButtons();

    public:
        Felis742LTNFMCProfile(ProductFMC *product);

        static bool IsEligible();

        const std::vector<std::string> &displayDatarefs() const override;
        const std::vector<FMCButtonDef> &buttonDefs() const override;
        const std::unordered_map<FMCKey, const FMCButtonDef *> &buttonKeyMap() const override;
        const std::map<char, FMCTextColor> &colorMap() const override;
        void mapCharacter(std::vector<uint8_t> *buffer, uint8_t character, bool isFontSmall) override;
        void updatePage(std::vector<std::vector<char>> &page) override;
        void buttonPressed(const FMCButtonDef *button, XPLMCommandPhase phase) override;
};

#endif // FELIS742_LTN_FMC_PROFILE_H
