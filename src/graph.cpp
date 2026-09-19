#include "warehouse_routing/graph.h"

namespace warehouse_routing {
    Graph::Graph(const Grid& grid) {
        //nested loop visits every (row,col) in grid
        for (int row = 0; row < grid.getRows(); row++) {
            for (int col = 0; col < grid.getCols(); col++) {
                //only free cells become nodes
                //shelf cells are skipped
                if (grid.isFree(row,col)) {
                    //same as Grid; computed fresh
                    int nodeId = row * grid.getCols() + col;
                    //create entry in map for node
                    //empty neighbor list for now
                    adjacency_[nodeId] = vector<int>();
                }
            }
        }
        //need a second pass for every node to check its 4 neighbors
        //up, down, left, right and add edge if neighbor is also free
        for (int row = 0; row < grid.getRows(); row++) {
            for (int col = 0; col < grid.getCols(); col++) {
                if (!grid.isFree(row,col)) {
                    continue;
                }

                int nodeId = row * grid.getCols() + col;

                //each pair is offset of (row,col)
                //4 directions; left, right, up, down
                int rowOffsets[] = {-1, 1, 0 , 0};
                int colOffsets[] = {0, 0, -1, 1};

                for(int i = 0; i < 4; i++) {
                    int neighborRow = row + rowOffsets[i];
                    int neighborCol = col + colOffsets[i];

                    bool inBounds = neighborRow >= 0 && neighborRow < grid.getRows()
                    && neighborCol >= 0 && neighborCol < grid.getCols();

                    if (inBounds && grid.isFree(neighborRow, neighborCol)) {
                        //row * cols skips past every full row before this one
                        //+ col to move to the right column in that row
                        int neighborId = neighborRow * grid.getCols() + neighborCol;
                        adjacency_[nodeId].push_back(neighborId);
                    }
                }
            }
        }
    }
    //uses .at(nodeId) because safer than [nodeId]
    const vector<int>& Graph::getNeighbors(int nodeId) const {
        return adjacency_.at(nodeId);
    }
    //uses .find() to check if key exists in map
    //compares result to .end()
    bool Graph::hasNode(int nodeId) const {
        return adjacency_.find(nodeId) != adjacency_.end();
    }
}