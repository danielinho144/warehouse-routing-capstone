#pragma once
#include <string>
#include <vector>
#include "warehouse_routing/agent.h"
#include "warehouse_routing/order.h"
#include "warehouse_routing/inventory.h"

namespace warehouse_routing {
    struct Scenario {
        std::string mapPath;
        std::vector<Agent> agents;
        std::vector<Order> orders;
        std::vector<InventoryCell> inventory;
    };
}