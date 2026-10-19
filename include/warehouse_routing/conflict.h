#pragma once

namespace warehouse_routing {

    enum class ConflictType {
        Vertex,
        Swap,
        Diagonal
    };

    struct Conflict {
        ConflictType type;
        int agentA;
        int agentB;
        int startNodeA;
        int endNodeA;
        int startNodeB;
        int endNodeB;
        int tick;
    };
}