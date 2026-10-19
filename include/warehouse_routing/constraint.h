#pragma once

namespace warehouse_routing {

    enum class ConstraintType {
        Vertex,
        Edge
    };

    struct constraintInfo {
        ConstraintType type;
        int agentID;
        int fromNode;
        int toNode;
        int tick;
    };
}