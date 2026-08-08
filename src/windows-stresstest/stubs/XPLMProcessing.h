#pragma once
// Stub XPLMProcessing.h for standalone Windows stress test build.
// logger.hpp registers a flight-loop callback to flush log lines queued off the
// main thread. There is no flight loop here, but Logger only queues once it has
// been initialized; uninitialized it emits directly, so no-ops are enough.

typedef float (*XPLMFlightLoop_f)(float inElapsedSinceLastCall, float inElapsedTimeSinceLastFlightLoop, int inCounter, void *inRefcon);

#ifdef __cplusplus
extern "C" {
#endif

static inline void XPLMRegisterFlightLoopCallback(XPLMFlightLoop_f inFlightLoop, float inInterval, void *inRefcon) {
    (void) inFlightLoop;
    (void) inInterval;
    (void) inRefcon;
}

static inline void XPLMUnregisterFlightLoopCallback(XPLMFlightLoop_f inFlightLoop, void *inRefcon) {
    (void) inFlightLoop;
    (void) inRefcon;
}

static inline float XPLMGetElapsedTime(void) {
    return 0.0f;
}

static inline int XPLMGetCycleNumber(void) {
    return 0;
}

#ifdef __cplusplus
}
#endif
