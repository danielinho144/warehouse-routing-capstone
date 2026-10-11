#include "warehouse_routing/conflict_detection.h"
#include <cstdlib>

namespace warehouse_routing {
    bool hasVertexConflict(int nextNodeA, int nextNodeB) {
        return nextNodeA == nextNodeB;
    }

    bool hasSwapConflict(int currentNodeA, int nextNodeA, int currentNodeB, int nextNodeB) {
        return nextNodeA == currentNodeB && nextNodeB == currentNodeA && currentNodeA != nextNodeA;
    }

    bool hasDiagonalConflict(const Graph& graph, int currentNodeA, int nextNodeA, int currentNodeB, int nextNodeB) {
        int currentRowA = graph.rowOf(currentNodeA);
        int currentColA = graph.colOf(currentNodeA);
        int nextRowA = graph.rowOf(nextNodeA);
        int nextColA = graph.colOf(nextNodeA);

        int currentRowB = graph.rowOf(currentNodeB);
        int currentColB = graph.colOf(currentNodeB);
        int nextRowB = graph.rowOf(nextNodeB);
        int nextColB = graph.colOf(nextNodeB);

        bool diagonalA = std::abs(nextRowA - currentRowA) == 1 && std::abs(nextColA - currentColA) == 1;
        bool diagonalB = std::abs(nextRowB - currentRowB) == 1 && std::abs(nextColB - currentColB) == 1;

        if (!diagonalA || !diagonalB) {return false;}
        bool sameMidpoint = currentRowA + nextRowA == currentRowB + nextRowB && currentColA + nextColA == currentColB + nextColB;

        return sameMidpoint && currentNodeA != currentNodeB && currentNodeA != nextNodeB;
    }
}