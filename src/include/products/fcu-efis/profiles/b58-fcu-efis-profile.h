#ifndef B58_FCU_EFIS_PROFILE_H
#define B58_FCU_EFIS_PROFILE_H

#include "fcu-efis-aircraft-profile.h"

#include <string>
#include <unordered_map>

class B58FCUEfisProfile : public FCUEfisAircraftProfile {
    public:
        B58FCUEfisProfile(ProductFCUEfis *product);

        static bool IsEligible();

        const std::vector<std::string> &displayDatarefs() const override;
        const std::unordered_map<uint16_t, FCUEfisButtonDef> &buttonDefs() const override;
        void updateDisplayData(FCUDisplayData &data) override;

        bool hasEfisLeft() const override {
            return true;
        }

        // The Baron has a copilot altimeter with its own Kollsman setting.
        bool hasEfisRight() const override {
            return true;
        }

        void buttonPressed(const FCUEfisButtonDef *button, XPLMCommandPhase phase) override;
};

#endif
