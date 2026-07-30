#ifndef A220_FCU_EFIS_PROFILE_H
#define A220_FCU_EFIS_PROFILE_H

#include "fcu-efis-aircraft-profile.h"

#include <map>
#include <string>
#include <unordered_map>

class A220FCUEfisProfile : public FCUEfisAircraftProfile {
    private:
        // alt_step_ft only mirrors the aircraft's internal step once it has been cycled at
        // least once, so the first selector use always cycles rather than trusting the value.
        bool altStepSynced = false;

    public:
        A220FCUEfisProfile(ProductFCUEfis *product);

        static bool IsEligible();

        const std::vector<std::string> &displayDatarefs() const override;
        const std::unordered_map<uint16_t, FCUEfisButtonDef> &buttonDefs() const override;
        void updateDisplayData(FCUDisplayData &data) override;

        bool hasEfisLeft() const override {
            return true;
        }

        bool hasEfisRight() const override {
            return true;
        }

        void buttonPressed(const FCUEfisButtonDef *button, XPLMCommandPhase phase) override;
};

#endif // A220_FCU_EFIS_PROFILE_H
