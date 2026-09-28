#pragma once
#include <vector>
#include <unordered_map>
#include "warehouse_routing/grid.h"

namespace warehouse_routing {

// A node is identified by a single integer using this Graph's own
// numbering: id = row * cols + col
// Use toNodeId / rowOf / colOf below instead of redoing the math

class Graph {
    public:
    //build graph from grid
    //explicit to stop compiler from secretely turning Grid into a Graph
    explicit Graph(const Grid& grid);

    //return neighboring node ID for given node
    const std::vector<int>& getNeighbors(int nodeId) const;

    //true if nodeId corresponds to Free cell that exists
    bool hasNode(int nodeId) const;

    //conversion between (row,col) and nodeID
    int toNodeId(int row, int col) const;
    int rowOf(int nodeId) const;
    int colOf(int nodeId) const;

    private:
    int rows_;
    int cols_;
    
    //maps each node ID to list of node IDs it's connected to
    //converts (row,col) to single int for convenience
    std::unordered_map<int, std::vector<int>> adjacency_;
};

}