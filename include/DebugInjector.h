#ifndef DEBUG_INJECTOR_H
#define DEBUG_INJECTOR_H

#include <Arduino.h>

struct DebugInjectorState {
    bool active = false;
    float bbqTemp = 0;
    float proteinTemp = 0;
    unsigned long endTimeMs = 0;
};

extern DebugInjectorState debugInjector;

// Returns true while injection is active; auto-expires when time runs out
inline bool debugInjectorIsActive() {
    if (debugInjector.active && millis() >= debugInjector.endTimeMs) {
        debugInjector.active = false;
    }
    return debugInjector.active;
}

#endif // DEBUG_INJECTOR_H
