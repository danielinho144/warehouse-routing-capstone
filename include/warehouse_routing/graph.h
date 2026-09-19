#pragma once
#include <vector>
#include <unordered_map>
#include "warehouse_routing/grid.h"

namespace warehouse_routing {

// A node is identified by a single integer, computed the same way
// Grid indexes its cells internally: row * cols + col. 
// To go from a node ID back to (row, col) when needed:
//   row = id / cols
//   col = id % cols

class Graph {
    public:
    //build graph from grid
    //explicit to stop compiler from secretely turning Grid into a Graph
    Graph(const Grid& grid);

    //return neighboring node ID for given node
    const vector<int>& getNeighbors(int nodeId) const;

    //true if nodeId corresponds to Free cell that exists
    bool hasNode(int nodeId) const;

    private:
    //maps each node ID to list of node IDs it's connected to
    //converts (row,col) to single int for convenience
    unordered_map<int, vector<int>> adjacency_;
};

}