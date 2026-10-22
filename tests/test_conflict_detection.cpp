#include <gtest/gtest.h>
#include <stdexcept>
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

TEST(PathConflictTest, EmptyMapHasNoConflict) {
    Grid grid(3, 3);
    Graph graph(grid);

    EXPECT_FALSE(findFirstConflict(graph, {}).has_value());
}

TEST(PathConflictTest, SingleAgentHasNoConflict) {
    Grid grid(3, 3);
    Graph graph(grid);

    EXPECT_FALSE(findFirstConflict(graph, {{7, {0, 1, 4}}}).has_value());
}

TEST(PathConflictTest, EmptyPathIsRejected) {
    Grid grid(3, 3);
    Graph graph(grid);

    EXPECT_THROW(findFirstConflict(graph, {{7, {}}}), std::runtime_error);
}

TEST(PathConflictTest, SeparateRoutesHaveNoConflict) {
    Grid grid(3, 3);
    Graph graph(grid);

    EXPECT_FALSE(findFirstConflict(graph,
        {{2, {0, 1, 2}}, {5, {6, 7, 8}}}).has_value());
}

TEST(PathConflictTest, FollowingAnotherAgentIsAllowed) {
    Grid grid(3, 3);
    Graph graph(grid);

    EXPECT_FALSE(findFirstConflict(graph,
        {{2, {0, 1}}, {5, {1, 2}}}).has_value());
}

TEST(PathConflictTest, DetectsInitialVertexConflictWithSingleNodePaths) {
    Grid grid(3, 3);
    Graph graph(grid);

    auto conflict = findFirstConflict(graph, {{5, {4}}, {2, {4}}});
    ASSERT_TRUE(conflict.has_value());
    EXPECT_EQ(conflict->type, ConflictType::Vertex);
    EXPECT_EQ(conflict->agentA, 2);
    EXPECT_EQ(conflict->agentB, 5);
    EXPECT_EQ(conflict->startNodeA, 4);
    EXPECT_EQ(conflict->endNodeA, 4);
    EXPECT_EQ(conflict->startNodeB, 4);
    EXPECT_EQ(conflict->endNodeB, 4);
    EXPECT_EQ(conflict->tick, 0);
}

TEST(PathConflictTest, DetectsVertexConflictAtFinalTick) {
    Grid grid(3, 3);
    Graph graph(grid);

    auto conflict = findFirstConflict(graph, {{2, {0, 1}}, {5, {2, 1}}});
    ASSERT_TRUE(conflict.has_value());
    EXPECT_EQ(conflict->type, ConflictType::Vertex);
    EXPECT_EQ(conflict->agentA, 2);
    EXPECT_EQ(conflict->agentB, 5);
    EXPECT_EQ(conflict->startNodeA, 1);
    EXPECT_EQ(conflict->endNodeA, 1);
    EXPECT_EQ(conflict->startNodeB, 1);
    EXPECT_EQ(conflict->endNodeB, 1);
    EXPECT_EQ(conflict->tick, 1);
}

TEST(PathConflictTest, SwapUsesDepartureTickAndDirectedMoves) {
    Grid grid(3, 3);
    Graph graph(grid);

    auto conflict = findFirstConflict(graph, {{2, {0, 1, 2}}, {5, {5, 2, 1}}});
    ASSERT_TRUE(conflict.has_value());
    EXPECT_EQ(conflict->type, ConflictType::Swap);
    EXPECT_EQ(conflict->agentA, 2);
    EXPECT_EQ(conflict->agentB, 5);
    EXPECT_EQ(conflict->startNodeA, 1);
    EXPECT_EQ(conflict->endNodeA, 2);
    EXPECT_EQ(conflict->startNodeB, 2);
    EXPECT_EQ(conflict->endNodeB, 1);
    EXPECT_EQ(conflict->tick, 1);
}

TEST(PathConflictTest, DiagonalUsesDepartureTickAndDirectedMoves) {
    Grid grid(3, 3);
    Graph graph(grid);

    auto conflict = findFirstConflict(graph, {{2, {0, 4}}, {5, {1, 3}}});
    ASSERT_TRUE(conflict.has_value());
    EXPECT_EQ(conflict->type, ConflictType::Diagonal);
    EXPECT_EQ(conflict->agentA, 2);
    EXPECT_EQ(conflict->agentB, 5);
    EXPECT_EQ(conflict->startNodeA, 0);
    EXPECT_EQ(conflict->endNodeA, 4);
    EXPECT_EQ(conflict->startNodeB, 1);
    EXPECT_EQ(conflict->endNodeB, 3);
    EXPECT_EQ(conflict->tick, 0);
}

TEST(PathConflictTest, AgentRemainsAtGoalAfterPathEnds) {
    Grid grid(3, 3);
    Graph graph(grid);

    auto conflict = findFirstConflict(graph, {{2, {0, 1}}, {5, {8, 5, 2, 1}}});
    ASSERT_TRUE(conflict.has_value());
    EXPECT_EQ(conflict->type, ConflictType::Vertex);
    EXPECT_EQ(conflict->agentA, 2);
    EXPECT_EQ(conflict->agentB, 5);
    EXPECT_EQ(conflict->startNodeA, 1);
    EXPECT_EQ(conflict->endNodeA, 1);
    EXPECT_EQ(conflict->startNodeB, 1);
    EXPECT_EQ(conflict->endNodeB, 1);
    EXPECT_EQ(conflict->tick, 3);
}

TEST(PathConflictTest, UnequalLengthSafePathsHaveNoConflict) {
    Grid grid(3, 3);
    Graph graph(grid);

    EXPECT_FALSE(findFirstConflict(graph,
        {{2, {0}}, {5, {6, 7, 8, 5}}}).has_value());
}

TEST(PathConflictTest, EarlierConflictWinsOverLowerIDPair) {
    Grid grid(3, 3);
    Graph graph(grid);

    // Pair (1, 2) meets at tick 2; pair (2, 9) meets at tick 1.
    auto conflict = findFirstConflict(graph, {{1, {0, 1, 2}}, {2, {8, 5, 2}}, {9, {4, 5, 8}}});
    ASSERT_TRUE(conflict.has_value());
    EXPECT_EQ(conflict->type, ConflictType::Vertex);
    EXPECT_EQ(conflict->agentA, 2);
    EXPECT_EQ(conflict->agentB, 9);
    EXPECT_EQ(conflict->startNodeA, 5);
    EXPECT_EQ(conflict->endNodeA, 5);
    EXPECT_EQ(conflict->startNodeB, 5);
    EXPECT_EQ(conflict->endNodeB, 5);
    EXPECT_EQ(conflict->tick, 1);
}

TEST(PathConflictTest, VertexPrecedesMovementAtSameTick) {
    Grid grid(3, 3);
    Graph graph(grid);

    // Pair (1, 2) swaps at departure tick 0; pair (8, 9) starts together.
    auto conflict = findFirstConflict(graph, {{1, {0, 1}}, {2, {1, 0}}, {8, {8, 5}}, {9, {8, 7}}});
    ASSERT_TRUE(conflict.has_value());
    EXPECT_EQ(conflict->type, ConflictType::Vertex);
    EXPECT_EQ(conflict->agentA, 8);
    EXPECT_EQ(conflict->agentB, 9);
    EXPECT_EQ(conflict->startNodeA, 8);
    EXPECT_EQ(conflict->endNodeA, 8);
    EXPECT_EQ(conflict->startNodeB, 8);
    EXPECT_EQ(conflict->endNodeB, 8);
    EXPECT_EQ(conflict->tick, 0);
}

TEST(PathConflictTest, VertexTiesUseAscendingAgentIDs) {
    Grid grid(3, 3);
    Graph graph(grid);

    auto conflict = findFirstConflict(graph, {{9, {4}}, {2, {4}}, {5, {4}}});
    ASSERT_TRUE(conflict.has_value());
    EXPECT_EQ(conflict->type, ConflictType::Vertex);
    EXPECT_EQ(conflict->agentA, 2);
    EXPECT_EQ(conflict->agentB, 5);
    EXPECT_EQ(conflict->startNodeA, 4);
    EXPECT_EQ(conflict->endNodeA, 4);
    EXPECT_EQ(conflict->startNodeB, 4);
    EXPECT_EQ(conflict->endNodeB, 4);
    EXPECT_EQ(conflict->tick, 0);
}

TEST(PathConflictTest, MovementTiesUseAscendingAgentIDsAcrossConflictTypes) {
    Grid grid(3, 3);
    Graph graph(grid);

    // Pair (1, 9) crosses diagonally; pair (2, 5) swaps.
    auto conflict = findFirstConflict(graph, {{9, {1, 3}}, {5, {8, 7}}, {2, {7, 8}}, {1, {0, 4}}});
    ASSERT_TRUE(conflict.has_value());
    EXPECT_EQ(conflict->type, ConflictType::Diagonal);
    EXPECT_EQ(conflict->agentA, 1);
    EXPECT_EQ(conflict->agentB, 9);
    EXPECT_EQ(conflict->startNodeA, 0);
    EXPECT_EQ(conflict->endNodeA, 4);
    EXPECT_EQ(conflict->startNodeB, 1);
    EXPECT_EQ(conflict->endNodeB, 3);
    EXPECT_EQ(conflict->tick, 0);
}
