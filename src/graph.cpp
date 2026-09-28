#include "warehouse_routing/graph.h"

namespace warehouse_routing {

    //build graph from grid: every Free cell becomes a node,
    //connected to its Free neighbors in 8 directions, no corner cutting
    Graph::Graph(const Grid& grid) {
        rows_ = grid.getRows();
        cols_ = grid.getCols();

        //order is fixed on purpose so results repeat run to run
        //up, down, left, right, up-left, up-right, down-left, down-right
        const int rowOffsets[] = {-1, 1, 0, 0, -1, -1, 1, 1};
        const int colOffsets[] = {0, 0, -1, 1, -1, 1, -1, 1};

        for (int row = 0; row < rows_; row++) {
            for (int col = 0; col < cols_; col++) {
                //only free cells become nodes
                if (!grid.isFree(row, col)) {
                    continue;
                }

                //creates the entry (empty list) even if no neighbors get added,
                //so a walled-in free cell is still a node
                std::vector<int>& neighbors = adjacency_[toNodeId(row, col)];

                for (int i = 0; i < 8; i++) {
                    int neighborRow = row + rowOffsets[i];
                    int neighborCol = col + colOffsets[i];

                    //every move requires a free destination
                    if (!grid.isFree(neighborRow, neighborCol))
                        continue;

                    if (i >= 4)
                        if (!grid.isFree(neighborRow, col) || !grid.isFree(row, neighborCol))
                            continue;
                    neighbors.push_back(toNodeId(neighborRow, neighborCol));
                }
            }
        }
    }

    //uses .at(nodeId) because it throws std::out_of_range on a missing key
    //[nodeId] would silently insert an empty entry instead
    const std::vector<int>& Graph::getNeighbors(int nodeId) const {
        return adjacency_.at(nodeId);
    }

    //uses .find() to check if key exists in map
    //compares result to .end()
    bool Graph::hasNode(int nodeId) const {
        return adjacency_.find(nodeId) != adjacency_.end();
    }

    //row * cols skips past every full row before this one
    //+ col to move to the right column in that row
    int Graph::toNodeId(int row, int col) const {
        return row * cols_ + col;
    }

    int Graph::rowOf(int nodeId) const {
        return nodeId / cols_;
    }

    int Graph::colOf(int nodeId) const {
        return nodeId % cols_;
    }
}