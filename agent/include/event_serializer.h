#pragma once

#include "event.h"
#include <string>

class EventSerializer { 
public: 
    static std::string to_json(const MonitorEvent& event);
};