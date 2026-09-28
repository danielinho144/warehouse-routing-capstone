#include <gtest/gtest.h>
#include <stdexcept>
#include <vector>
#include "warehouse_routing/grid.h"
#include "warehouse_routing/graph.h"

using namespace warehouse_routing;

// Node IDs on a 3-column grid are row * 3 + col:
//   0 1 2
//   3 4 5
//   6 7 8

TEST(GraphTest, ShelfIsNotANode) {
    Grid grid(3, 3);
    grid.setCellType(1, 1, CellType::Shelf);   // id 4
    Graph graph(grid);

    EXPECT_FALSE(graph.hasNode(4));
    EXPECT_TRUE(graph.hasNode(0));
}

TEST(GraphTest, CornerCellHasThreeNeighbors) {
    Grid grid(3, 3);
    Graph graph(grid);

    // (0,0): up and left are off the grid
    // down = (1,0) = 3, right = (0,1) = 1
    EXPECT_EQ(graph.getNeighbors(0), (std::vector<int>{3, 1, 4}));
}

TEST(GraphTest, EdgeCellHasFiveNeighbors) {
    Grid grid(3, 3);
    Graph graph(grid);

    // (0,1): up is off the grid
    // down = (1,1) = 4, left = (0,0) = 0, right = (0,2) = 2
    EXPECT_EQ(graph.getNeighbors(1), (std::vector<int>{4, 0, 2, 3, 5}));
}

TEST(GraphTest, InteriorCellHasEightNeighbors) {
    Grid grid(3, 3);
    Graph graph(grid);

    // (1,1): up = 1, down = 7, left = 3, right = 5
    EXPECT_EQ(graph.getNeighbors(4), (std::vector<int>{1, 7, 3, 5, 0, 2, 6, 8}));
}

TEST(GraphTest, RightShelfBlocksTheEdgeToIt) {
    Grid grid(3, 3);
    grid.setCellType(1, 2, CellType::Shelf);   // id 5
    Graph graph(grid);
    // From center node 4, shelf 5 blocks right and diagonals 2 and 8.
    // Remaining: up, down, left, up-left, down-left.
    EXPECT_EQ(graph.getNeighbors(4), (std::vector<int>{1, 7, 3, 0, 6}));
}

TEST(GraphTest, GetNeighborsThrowsForShelf) {
    Grid grid(3, 3);
    grid.setCellType(1, 1, CellType::Shelf);
    Graph graph(grid);

    EXPECT_THROW(graph.getNeighbors(4), std::out_of_range);
}

TEST(GraphTest, ShelfBlocksTheEdgeToIt) {
    Grid grid(3, 3);
    grid.setCellType(1, 1, CellType::Shelf); // ID 4
    Graph graph(grid);

    // From node 1, the shelf blocks down and both downward diagonals.
    // Only left (0) and right (2) remain.
    EXPECT_EQ(graph.getNeighbors(1), (std::vector<int>{0, 2}));
}

TEST(GraphTest, WalledInFreeCellIsANodeWithNoNeighbors) {
    Grid grid(3, 3);
    grid.setCellType(0, 1, CellType::Shelf);   // id 1, above the center
    grid.setCellType(1, 0, CellType::Shelf);   // id 3, left of the center
    grid.setCellType(1, 2, CellType::Shelf);   // id 5, right of the center
    grid.setCellType(2, 1, CellType::Shelf);   // id 7, below the center
    Graph graph(grid);

    // (1,1) = id 4 is still free, so it is a node, just with nowhere to go
    EXPECT_TRUE(graph.hasNode(4));
    EXPECT_TRUE(graph.getNeighbors(4).empty());
}

TEST(GraphTest, IdRoundTrip) {
    Grid grid(3, 5);
    Graph graph(grid);

    EXPECT_EQ(graph.toNodeId(0, 0), 0);
    EXPECT_EQ(graph.rowOf(0), 0);
    EXPECT_EQ(graph.colOf(0), 0);

    EXPECT_EQ(graph.toNodeId(1, 2), 7);
    EXPECT_EQ(graph.rowOf(7), 1);
    EXPECT_EQ(graph.colOf(7), 2);

    EXPECT_EQ(graph.toNodeId(2, 4), 14);
    EXPECT_EQ(graph.rowOf(14), 2);
    EXPECT_EQ(graph.colOf(14), 4);
}