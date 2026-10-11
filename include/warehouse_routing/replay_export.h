#pragma once
#include <string>
#include <nlohmann/json.hpp>
#include "warehouse_routing/simulation.h"

namespace warehouse_routing {
    void recordSnapshot(const Simulation& simulation, nlohmann::json& replay);
    void saveReplay(const nlohmann::json& replay, const std::string& filePath);
}