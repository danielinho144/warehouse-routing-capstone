#pragma once
#include <vector>
#include "warehouse_routing/graph.h"

namespace warehouse_routing {
    std::vector<int> aStar(const Graph& graph, int startID, int goalID);
}