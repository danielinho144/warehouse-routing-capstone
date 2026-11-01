#pragma once
#include <string>
#include <cstddef>
#include <optional>
#include <vector>
#include "warehouse_routing/position.h"
#include "warehouse_routing/graph.h"
#include "warehouse_routing/order.h"
#include "warehouse_routing/agent.h"
#include "warehouse_routing/inventory.h"

namespace warehouse_routing {
    struct PickupTask {
        std::size_t inventoryIndex;
        std::string itemID;
        Position pickupPosition;
    };

    std::optional<PickupTask> findNextPickup(const Graph& graph, 
        const Agent& agent, 
        const Order& order, 
        const std::vector<InventoryCell>& inventory);
}