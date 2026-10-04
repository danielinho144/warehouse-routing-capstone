#include <gtest/gtest.h>
#include <stdexcept>
#include <vector>
#include "warehouse_routing/astar.h"

/*
create a free 3×3 Grid
build its Graph
call aStar(graph, 0, 8)
expect the path {0, 4, 8}
*/

using namespace warehouse_routing;

TEST(aStarTest, VerifyPath) {
    Grid grid(3, 3);
    Graph graph(grid);
    EXPECT_EQ(aStar(graph, 0, 8), (std::vector<int>{0, 4, 8}));
}

TEST(aStarTest, StartEqualsGoal) {
    Grid grid(3, 3);
    Graph graph(grid);
    EXPECT_EQ(aStar(graph, 4, 4), (std::vector<int>{4}));
}

TEST(aStarTest, InvalidStartNode) {
    Grid grid(3, 3);
    Graph graph(grid);
    EXPECT_TRUE(aStar(graph, -1, 8).empty());
}

TEST(aStarTest, InvalidGoal) {
    Grid grid(3, 3);
    Graph graph(grid);
    EXPECT_TRUE(aStar(graph, 0, 9).empty());
}

TEST(aStarTest, UnreachableGoal) {
    Grid grid(3, 3);
    grid.setCellType(1, 0, CellType::Shelf);
    grid.setCellType(1, 1, CellType::Shelf);
    grid.setCellType(1, 2, CellType::Shelf);
    Graph graph(grid);
    EXPECT_EQ(aStar(graph, 0, 8), (std::vector<int>{}));
}

TEST(aStarTest, ObstacleDetour) {
    Grid grid(3, 3);
    grid.setCellType(1, 1, CellType::Shelf);
    Graph graph(grid);
    std::vector<int> path = aStar(graph, 0, 8);
    ASSERT_EQ(path.size(), 5);
    EXPECT_EQ(path.front(), 0);
    EXPECT_EQ(path.back(), 8);

    for (std::size_t i = 1; i < path.size(); i++) {
        int previous = path[i - 1];
        int next = path[i];

        bool found = false;
        for (int neighbor : graph.getNeighbors(previous)) {
            if (neighbor == next) {
                found = true;
                break;
            }
        }
        EXPECT_TRUE(found);
    }
}