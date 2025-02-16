#ifndef ROUTE_CONSTANTS_H
#define ROUTE_CONSTANTS_H

namespace Routes {
    // Versão base da API
    constexpr const char* API_VERSION = "/api/v1";
    
    namespace Temperature {
        constexpr const char* BASE = "/api/v1/temperature";
        constexpr const char* CONFIG = "/api/v1/temperature/config";
    }
    
    namespace MQTT {
        constexpr const char* BASE = "/api/v1/mqtt";
        constexpr const char* CONFIG = "/api/v1/mqtt/config";
    }
    
    namespace System {
        constexpr const char* BASE = "/api/v1/system";
        constexpr const char* RESET = "/api/v1/system/reset";
        constexpr const char* CURE = "/api/v1/system/activateCure";
        constexpr const char* UPDATE = "/api/v1/system/updateFirmware";
    }
    
    namespace Monitor {
        constexpr const char* BASE = "/api/v1/monitor";
    }
    
    namespace Energy {
        constexpr const char* BASE = "/api/v1/energy";
        constexpr const char* COST = "/api/v1/energy/cost";
    }
    
    namespace AI {
        constexpr const char* BASE = "/api/v1/ai";
        constexpr const char* CONFIG = "/api/v1/ai/config";
    }
}

#endif 