#include "warehouse_routing/conflict_detection.h"
#include <cstdlib>

//identify moves that would collide

namespace warehouse_routing {

    //check whther two agents propose the same destination
    bool hasVertexConflict(int nextNodeA, int nextNodeB) {
        return nextNodeA == nextNodeB;
    }

    //check whether two agents exchange positions during the same tick
    bool hasSwapConflict(int currentNodeA, int nextNodeA, int currentNodeB, int nextNodeB) {
        return nextNodeA == currentNodeB && nextNodeB == currentNodeA && currentNodeA != nextNodeA;
    }

    //check whether two diagonal moves cross through the center of the same square
    bool hasDiagonalConflict(const Graph& graph, int currentNodeA, int nextNodeA, int currentNodeB, int nextNodeB) {
        //convert node IDs into row/column coords
        int currentRowA = graph.rowOf(currentNodeA);
        int currentColA = graph.colOf(currentNodeA);
        int nextRowA = graph.rowOf(nextNodeA);
        int nextColA = graph.colOf(nextNodeA);

        int currentRowB = graph.rowOf(currentNodeB);
        int currentColB = graph.colOf(currentNodeB);
        int nextRowB = graph.rowOf(nextNodeB);
        int nextColB = graph.colOf(nextNodeB);

        //check both moves change row and column by 1
        bool diagonalA = std::abs(nextRowA - currentRowA) == 1 && std::abs(nextColA - currentColA) == 1;
        bool diagonalB = std::abs(nextRowB - currentRowB) == 1 && std::abs(nextColB - currentColB) == 1;

        //exclude movement along same diagonal; other conflict checks handle
        if (!diagonalA || !diagonalB) {return false;}
        bool sameMidpoint = currentRowA + nextRowA == currentRowB + nextRowB && currentColA + nextColA == currentColB + nextColB;

        return sameMidpoint && currentNodeA != currentNodeB && currentNodeA != nextNodeB;
    }
}