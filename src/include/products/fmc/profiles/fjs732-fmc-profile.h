#ifndef FJS732_FMC_PROFILE_H
#define FJS732_FMC_PROFILE_H

#include "fmc-aircraft-profile.h"

// FlyJSim 737-200: a single CDU/FMS (no captain/FO split) on the Universal
// Avionics UNS-1 "uns1/cdu1" / "uns1/fms1" dataref namespace. The UNS-1 screen
// decoding (symbol tables, page decode, glyph mapping) is shared via UNS1Decode,
// but the style-byte colour table is this aircraft's own (its nibble->colour
// encoding differs from the CJ525 and Q4XP). The button map, eligibility and
// backlight wiring are specific to this airframe.
class FJS732FMCProfile : public FMCAircraftProfile {
    public:
        FJS732FMCProfile(ProductFMC *product);

        static bool IsEligible();

        const std::vector<std::string> &displayDatarefs() const override;
        const std::vector<FMCButtonDef> &buttonDefs() const override;
        const std::unordered_map<FMCKey, const FMCButtonDef *> &buttonKeyMap() const override;
        const std::map<char, FMCTextColor> &colorMap() const override;
        void mapCharacter(std::vector<uint8_t> *buffer, uint8_t character, bool isFontSmall) override;
        void updatePage(std::vector<std::vector<char>> &page) override;
        void buttonPressed(const FMCButtonDef *button, XPLMCommandPhase phase) override;
};

#endif // FJS732_FMC_PROFILE_H
