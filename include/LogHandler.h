#ifndef LogHandler_h
#define LogHandler_h

#include <Arduino.h>

class LogHandler {
public:
    LogHandler();
    void logMessage(const String& message);
};

#endif /* LogHandler_h */
