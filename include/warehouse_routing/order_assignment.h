#pragma once
#include "warehouse_routing/graph.h"
#include "warehouse_routing/agent.h"
#include "warehouse_routing/order.h"
#include <vector>

namespace warehouse_routing {
    bool assignNearestOrder(const Graph& graph, Agent& agent, std::vector<Order>& orders);
}