#ifndef NWS_AIRCRAFT_PROFILE_H
#define NWS_AIRCRAFT_PROFILE_H

#include "profile-cleanup.h"

class ProductNWS;

class NWSAircraftProfile {
    protected:
        ProductNWS *product = nullptr;

    public:
        NWSAircraftProfile(ProductNWS *product) : product(product) {};

        virtual ~NWSAircraftProfile() {
            cleanupProfile(this);
        }
};

#endif
