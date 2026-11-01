#pragma once
#include "warehouse_routing/graph.h"
#include "warehouse_routing/agent.h"
#include "warehouse_routing/order.h"
#include <vector>
#include "warehouse_routing/inventory.h"

namespace warehouse_routing {
    bool assignNearestOrder(const Graph& graph, Agent& agent, 
        std::vector<Order>& orders, 
        const std::vector<InventoryCell>& inventory);
}