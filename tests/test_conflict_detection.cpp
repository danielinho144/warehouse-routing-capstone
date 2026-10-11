#include <gtest/gtest.h>
#include "warehouse_routing/conflict_detection.h"
#include "warehouse_routing/grid.h"

using namespace warehouse_routing;

TEST(ConflictDetectionTest, SameDestinationIsVertexConflict) {
    EXPECT_TRUE(hasVertexConflict(4, 4));
}

TEST(ConflictDetectionTest, DifferentDestinationsAreNotVertexConflict) {
    EXPECT_FALSE(hasVertexConflict(4, 5));
}

TEST(ConflictDetectionTest, ExchangingCellsIsSwapConflict) {
    EXPECT_TRUE(hasSwapConflict(3, 4, 4, 3));
    EXPECT_TRUE(hasSwapConflict(4, 3, 3, 4));
}

TEST(ConflictDetectionTest, FollowingAgentIsNotSwapConflict) {
    EXPECT_FALSE(hasSwapConflict(0, 1, 1, 2));
}

TEST(ConflictDetectionTest, WaitingIsNotSwapConflict) {
    EXPECT_FALSE(hasSwapConflict(0, 0, 1, 1));
    EXPECT_FALSE(hasSwapConflict(0, 1, 1, 1));
    EXPECT_FALSE(hasSwapConflict(1, 1, 1, 1));
}

TEST(ConflictDetectionTest, CrossingDiagonalsAreConflict) {
    Grid grid(3, 3);
    Graph graph(grid);

    EXPECT_TRUE(hasDiagonalConflict(graph, 0, 4, 1, 3));
    EXPECT_TRUE(hasDiagonalConflict(graph, 1, 3, 0, 4));
    EXPECT_TRUE(hasDiagonalConflict(graph, 4, 0, 3, 1));
    EXPECT_TRUE(hasDiagonalConflict(graph, 0, 4, 3, 1));
}

TEST(ConflictDetectionTest, SeparateDiagonalsDoNotConflict) {
    Grid grid(3, 3);
    Graph graph(grid);

    EXPECT_FALSE(hasDiagonalConflict(graph, 0, 4, 1, 5));
}

TEST(ConflictDetectionTest, SameDiagonalIsNotCrossingConflict) {
    Grid grid(3, 3);
    Graph graph(grid);

    EXPECT_FALSE(hasDiagonalConflict(graph, 0, 4, 0, 4));
    EXPECT_FALSE(hasDiagonalConflict(graph, 0, 4, 4, 0));
}

TEST(ConflictDetectionTest, StraightMovesAreNotDiagonalConflict) {
    Grid grid(3, 3);
    Graph graph(grid);

    EXPECT_FALSE(hasDiagonalConflict(graph, 0, 1, 1, 0));
    EXPECT_FALSE(hasDiagonalConflict(graph, 0, 4, 1, 4));
    EXPECT_FALSE(hasDiagonalConflict(graph, 1, 4, 0, 4));
}

TEST(ConflictDetectionTest, WaitingIsNotDiagonalConflict) {
    Grid grid(3, 3);
    Graph graph(grid);

    EXPECT_FALSE(hasDiagonalConflict(graph, 0, 0, 1, 3));
    EXPECT_FALSE(hasDiagonalConflict(graph, 1, 3, 0, 0));
    EXPECT_FALSE(hasDiagonalConflict(graph, 0, 0, 1, 1));
}
