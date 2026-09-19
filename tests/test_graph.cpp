#include <gtest/gtest.h>
#include "warehouse_routing/grid.h"
#include "warehouse_routing/graph.h"

using namespace warehouse_routing;

TEST(GraphTest, BuildsCorrectNodesAndEdges) {
    //3x3 grid, all free except one shelf
    // (0,0) (0,1) (0,2)
    // (1,0) SHELF (1,2)
    // (2,0) (2,1) (2,2)

    Grid grid(3,3);
    grid.setCellType(1,1, CellType::Shelf);

    Graph graph(grid);

    //(1,1) is shelf;
    //should not exist as node at all
    int shelfId = 1 * grid.getCols() + 1;
    EXPECT_FALSE(graph.hasNode(shelfId));

    //(0,0) is free
    //should exist as node
    int cornerId = 0 * grid.getCols() + 0;
    EXPECT_TRUE(graph.hasNode(cornerId));

    // (0,0) only 4 connected free neighbors
    // (0,1) (1,0)
    // (1,1) is shelf, should not appear as neighbor
    int rightId = 0 * grid.getCols() + 1;
    int downId = 1 * grid.getCols() + 0;

    const vector<int>& neighbors = graph.getNeighbors(cornerId);
    EXPECT_EQ(neighbors.size(), 2);
    EXPECT_NE(find(neighbors.begin(), neighbors.end(), rightId), neighbors.end());
    EXPECT_NE(find(neighbors.begin(), neighbors.end(), downId), neighbors.end());

}