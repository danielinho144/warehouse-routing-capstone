#pragma once
#include <vector>
#include <unordered_map>
#include "warehouse_routing/constraint.h"

namespace warehouse_routing {

    struct CBSNode {
        std::vector<constraintInfo> constraints;
        std::unordered_map<int, std::vector<int>> paths;
        int cost = 0;
    };
}