#pragma once
#include "warehouse_routing/graph.h"

namespace warehouse_routing {
    bool hasVertexConflict(int nextNodeA, int nextNodeB);
    bool hasSwapConflict(int currentNodeA, int nextNodeA, int currentNodeB, int nextNodeB);
    bool hasDiagonalConflict(const Graph& graph, int currentNodeA, int nextNodeA, int currentNodeB, int nextNodeB);
}