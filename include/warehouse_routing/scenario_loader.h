#pragma once
#include <string>
#include "warehouse_routing/scenario.h"

namespace warehouse_routing {
    Scenario loadScenario(const std::string& filepath);
}