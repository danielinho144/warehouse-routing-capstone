#include <gtest/gtest.h>
#include "warehouse_routing/constrained_astar.h"

using namespace warehouse_routing;

TEST(ConstrainedAStarTest, NoConstraints) {
    Grid grid(3, 3);
    Graph graph(grid);

    std::vector<constraintInfo> constraint;

    std::vector<int> path = constrainedAStar(graph, 0, 0, 8, constraint, 10);
    EXPECT_EQ(path, (std::vector<int>{0, 4, 8}));
}

TEST(ConstrainedAStarTest, VertexConstraintForcesWait) {
    Grid grid(1, 3);
    Graph graph(grid);

    constraintInfo vertex;
    vertex.type = ConstraintType::Vertex;
    vertex.agentID = 0;
    vertex.fromNode = -1;
    vertex.toNode = 1;
    vertex.tick = 1;
    std::vector<constraintInfo> constraint = {vertex};
    
    std::vector<int> path = constrainedAStar(graph, 0, 0, 2, constraint, 10);
    EXPECT_EQ(path, (std::vector<int>{0, 0, 1, 2}));
}

TEST(ConstrainedAStarTest, EdgeConstraintForcesWait) {
    Grid grid(1, 3);
    Graph graph(grid);

    constraintInfo edge;
    edge.type = ConstraintType::Edge;
    edge.agentID = 0;
    edge.fromNode = 0;
    edge.toNode = 1;
    edge.tick = 0;

    std::vector<constraintInfo> constraint = {edge};

    std::vector<int> path = constrainedAStar(graph, 0, 0, 2, constraint, 10);
    EXPECT_EQ(path, (std::vector<int>{0, 0, 1, 2}));
}

TEST(ConstrainedAStarTest, InvalidStartOrGoal) {
    Grid grid(3, 3);
    grid.setCellType(1, 1, CellType::Shelf);
    Graph graph(grid);
    std::vector<constraintInfo> constraint;

    EXPECT_TRUE(constrainedAStar(graph, 0, -1, 8, constraint, 10).empty());
    EXPECT_TRUE(constrainedAStar(graph, 0, 0, 9, constraint, 10).empty());
    EXPECT_TRUE(constrainedAStar(graph, 0, 4, 8, constraint, 10).empty());
    EXPECT_TRUE(constrainedAStar(graph, 0, 0, 4, constraint, 10).empty());
}

TEST(ConstrainedAStarTest, NegativeMaxTick) {
    Grid grid(1, 3);
    Graph graph(grid);
    std::vector<constraintInfo> constraint;

    EXPECT_TRUE(constrainedAStar(graph, 0, 0, 2, constraint, -1).empty());
}

TEST(ConstrainedAStarTest, StartEqualsGoal) {
    Grid grid(3, 3);
    Graph graph(grid);
    std::vector<constraintInfo> constraint;

    EXPECT_EQ(constrainedAStar(graph, 0, 4, 4, constraint, 0), (std::vector<int>{4}));
}

TEST(ConstrainedAStarTest, StartForbiddenAtTickZero) {
    Grid grid(1, 3);
    Graph graph(grid);

    constraintInfo vertex;
    vertex.type = ConstraintType::Vertex;
    vertex.agentID = 0;
    vertex.fromNode = -1;
    vertex.toNode = 0;
    vertex.tick = 0;
    std::vector<constraintInfo> constraint = {vertex};

    EXPECT_TRUE(constrainedAStar(graph, 0, 0, 2, constraint, 10).empty());
    EXPECT_TRUE(constrainedAStar(graph, 0, 0, 0, constraint, 10).empty());
}

TEST(ConstrainedAStarTest, TickLimit) {
    Grid grid(1, 3);
    Graph graph(grid);
    std::vector<constraintInfo> constraint;

    EXPECT_TRUE(constrainedAStar(graph, 0, 0, 2, constraint, 0).empty());
    EXPECT_TRUE(constrainedAStar(graph, 0, 0, 2, constraint, 1).empty());
    EXPECT_EQ(constrainedAStar(graph, 0, 0, 2, constraint, 2), (std::vector<int>{0, 1, 2}));
}

TEST(ConstrainedAStarTest, UnreachableGoal) {
    Grid grid(3, 3);
    grid.setCellType(1, 0, CellType::Shelf);
    grid.setCellType(1, 1, CellType::Shelf);
    grid.setCellType(1, 2, CellType::Shelf);
    Graph graph(grid);
    std::vector<constraintInfo> constraint;

    EXPECT_TRUE(constrainedAStar(graph, 0, 0, 8, constraint, 10).empty());
}

TEST(ConstrainedAStarTest, IgnoresOtherAgentsConstraints) {
    Grid grid(1, 3);
    Graph graph(grid);

    constraintInfo vertex;
    vertex.type = ConstraintType::Vertex;
    vertex.agentID = 7;
    vertex.fromNode = -1;
    vertex.toNode = 1;
    vertex.tick = 1;

    constraintInfo edge;
    edge.type = ConstraintType::Edge;
    edge.agentID = 7;
    edge.fromNode = 0;
    edge.toNode = 1;
    edge.tick = 0;

    constraintInfo goal;
    goal.type = ConstraintType::Vertex;
    goal.agentID = 7;
    goal.fromNode = -1;
    goal.toNode = 2;
    goal.tick = 3;
    std::vector<constraintInfo> constraint = {vertex, edge, goal};

    EXPECT_EQ(constrainedAStar(graph, 0, 0, 2, constraint, 10), (std::vector<int>{0, 1, 2}));
}

TEST(ConstrainedAStarTest, FutureGoalConstraints) {
    Grid grid(1, 2);
    Graph graph(grid);

    constraintInfo vertex;
    vertex.type = ConstraintType::Vertex;
    vertex.agentID = 0;
    vertex.fromNode = -1;
    vertex.toNode = 0;
    vertex.tick = 1;
    std::vector<constraintInfo> constraint = {vertex};

    EXPECT_TRUE(constrainedAStar(graph, 0, 0, 0, constraint, 1).empty());
    EXPECT_EQ(constrainedAStar(graph, 0, 0, 0, constraint, 2), (std::vector<int>{0, 1, 0}));

    constraintInfo edge;
    edge.type = ConstraintType::Edge;
    edge.agentID = 0;
    edge.fromNode = 0;
    edge.toNode = 0;
    edge.tick = 1;
    constraint = {edge};

    EXPECT_TRUE(constrainedAStar(graph, 0, 0, 0, constraint, 1).empty());
    EXPECT_EQ(constrainedAStar(graph, 0, 0, 0, constraint, 2), (std::vector<int>{0, 1, 0}));
}
