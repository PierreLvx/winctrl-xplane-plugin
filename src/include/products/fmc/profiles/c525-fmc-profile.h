#ifndef C525_FMC_PROFILE_H
#define C525_FMC_PROFILE_H

#include "fmc-aircraft-profile.h"

// TorqueSim Citation CJ (C525): a single-pilot aircraft with one CDU/FMS on the
// stock Universal Avionics UNS-1 "uns1/cdu1" / "uns1/fms1" dataref namespace. The
// UNS-1 screen decoding (symbol tables, page decode, glyph and colour mapping) is
// shared via UNS1Decode; the button map, eligibility and backlight wiring are
// specific to this airframe.
class C525FMCProfile : public FMCAircraftProfile {
    public:
        C525FMCProfile(ProductFMC *product);

        static bool IsEligible();

        const std::vector<std::string> &displayDatarefs() const override;
        const std::vector<FMCButtonDef> &buttonDefs() const override;
        const std::unordered_map<FMCKey, const FMCButtonDef *> &buttonKeyMap() const override;
        const std::map<char, FMCTextColor> &colorMap() const override;
        void mapCharacter(std::vector<uint8_t> *buffer, uint8_t character, bool isFontSmall) override;
        void updatePage(std::vector<std::vector<char>> &page) override;
        void buttonPressed(const FMCButtonDef *button, XPLMCommandPhase phase) override;
};

#endif // C525_FMC_PROFILE_H
