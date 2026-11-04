#include <gtest/gtest.h>
#include <stdexcept>
#include <algorithm>
#include "warehouse_routing/cbs.h"
#include "warehouse_routing/conflict_detection.h"
#include "warehouse_routing/independent_planner.h"

using namespace warehouse_routing;

TEST(CBSTest, NoAgentsReturnsSuccessfulEmptyPlan) {
    Grid grid(3, 3);
    Graph graph(grid);
    std::vector<Agent> agents;
    std::unordered_map<int, int> goals;

    auto paths = planCBSPaths(graph, agents, goals, 0, 1);

    ASSERT_TRUE(paths.has_value());
    EXPECT_TRUE(paths->empty());
}

TEST(CBSTest, SingleAgentReachesGoal) {
    Grid grid(3, 3);
    Graph graph(grid);
    //std::vector<Agent> agents = {Agent(7, Position{0, 0})};
    std::vector<Agent> agents = {Agent(7, Position{0, 0}, 5)};
    std::unordered_map<int, int> goals = {{7, 8}};

    auto paths = planCBSPaths(graph, agents, goals, 2, 1);

    ASSERT_TRUE(paths.has_value());
    ASSERT_EQ(paths->size(), 1u);
    EXPECT_EQ(paths->at(7), (std::vector<int>{0, 4, 8}));
}

TEST(CBSTest, StartAtGoalAllowsZeroTicks) {
    Grid grid(3, 3);
    Graph graph(grid);
    //std::vector<Agent> agents = {Agent(7, Position{1, 1})};
    std::vector<Agent> agents = {Agent(7, Position{1, 1}, 5)};
    std::unordered_map<int, int> goals = {{7, 4}};

    auto paths = planCBSPaths(graph, agents, goals, 0, 1);

    ASSERT_TRUE(paths.has_value());
    EXPECT_EQ(paths->at(7), (std::vector<int>{4}));
}

TEST(CBSTest, SeparateRoutesNeedNoBranching) {
    Grid grid(3, 3);
    Graph graph(grid);
    std::vector<Agent> agents = {
        //Agent(2, Position{0, 0}), Agent(9, Position{2, 0})
        Agent(2, Position{0, 0}, 5), Agent(9, Position{2, 0}, 5)
    };
    std::unordered_map<int, int> goals = {{2, 2}, {9, 8}};

    auto paths = planCBSPaths(graph, agents, goals, 2, 1);

    ASSERT_TRUE(paths.has_value());
    ASSERT_EQ(paths->size(), 2u);
    EXPECT_EQ(paths->at(2), (std::vector<int>{0, 1, 2}));
    EXPECT_EQ(paths->at(9), (std::vector<int>{6, 4, 8}));
    EXPECT_FALSE(findFirstConflict(graph, paths.value()).has_value());
}

TEST(CBSTest, VertexConflictIsResolvedAtMinimumCombinedCost) {
    Grid grid(3, 3);
    Graph graph(grid);
    std::vector<Agent> agents = {
        //Agent(2, Position{1, 0}), Agent(9, Position{1, 2})
        Agent(2, Position{1, 0}, 5), Agent(9, Position{1, 2}, 5)
    };
    std::unordered_map<int, int> goals = {{2, 5}, {9, 3}};
    auto independent = planIndependentPaths(graph, agents, goals);
    auto conflict = findFirstConflict(graph, independent);
    ASSERT_TRUE(conflict.has_value());
    ASSERT_EQ(conflict->type, ConflictType::Vertex);

    auto paths = planCBSPaths(graph, agents, goals, 4, 100);

    ASSERT_TRUE(paths.has_value());
    EXPECT_FALSE(findFirstConflict(graph, paths.value()).has_value());
    EXPECT_EQ(paths->at(2).front(), 3);
    EXPECT_EQ(paths->at(2).back(), 5);
    EXPECT_EQ(paths->at(9).front(), 5);
    EXPECT_EQ(paths->at(9).back(), 3);
    EXPECT_EQ(paths->at(2).size() + paths->at(9).size() - 2, 4u);
    for (const Agent& agent : agents) {
        const std::vector<int>& path = paths->at(agent.getId());
        for (std::size_t i = 1; i < path.size(); i++) {
            const std::vector<int>& neighbors = graph.getNeighbors(path.at(i - 1));
            EXPECT_TRUE(path.at(i) == path.at(i - 1) ||
                std::find(neighbors.begin(), neighbors.end(), path.at(i)) != neighbors.end());
        }
    }
}

TEST(CBSTest, SwapRequiresFurtherBranchingAndHasMinimumCost) {
    Grid grid(2, 2);
    Graph graph(grid);
    std::vector<Agent> agents = {
        //Agent(2, Position{0, 0}), Agent(9, Position{0, 1})
        Agent(2, Position{0, 0}, 5), Agent(9, Position{0, 1}, 5)
    };
    std::unordered_map<int, int> goals = {{2, 1}, {9, 0}};
    auto independent = planIndependentPaths(graph, agents, goals);
    auto conflict = findFirstConflict(graph, independent);
    ASSERT_TRUE(conflict.has_value());
    ASSERT_EQ(conflict->type, ConflictType::Swap);

    auto paths = planCBSPaths(graph, agents, goals, 3, 100);

    ASSERT_TRUE(paths.has_value());
    EXPECT_FALSE(findFirstConflict(graph, paths.value()).has_value());
    EXPECT_EQ(paths->at(2).front(), 0);
    EXPECT_EQ(paths->at(2).back(), 1);
    EXPECT_EQ(paths->at(9).front(), 1);
    EXPECT_EQ(paths->at(9).back(), 0);
    EXPECT_EQ(paths->at(2).size() + paths->at(9).size() - 2, 3u);
}

TEST(CBSTest, DiagonalCrossingIsResolvedWithWait) {
    Grid grid(2, 2);
    Graph graph(grid);
    std::vector<Agent> agents = {
        //Agent(2, Position{0, 0}), Agent(9, Position{0, 1})
        Agent(2, Position{0, 0}, 5), Agent(9, Position{0, 1}, 5)
    };
    std::unordered_map<int, int> goals = {{2, 3}, {9, 2}};
    auto independent = planIndependentPaths(graph, agents, goals);
    auto conflict = findFirstConflict(graph, independent);
    ASSERT_TRUE(conflict.has_value());
    ASSERT_EQ(conflict->type, ConflictType::Diagonal);

    auto paths = planCBSPaths(graph, agents, goals, 2, 100);

    ASSERT_TRUE(paths.has_value());
    EXPECT_FALSE(findFirstConflict(graph, paths.value()).has_value());
    EXPECT_EQ(paths->at(2).back(), 3);
    EXPECT_EQ(paths->at(9).back(), 2);
    EXPECT_EQ(paths->at(2).size() + paths->at(9).size() - 2, 3u);
}

TEST(CBSTest, GoalOccupantIsRespectedAfterItsPathEnds) {
    Grid grid(2, 3);
    Graph graph(grid);
    std::vector<Agent> agents = {
        //Agent(2, Position{0, 1}), Agent(9, Position{0, 0})
        Agent(2, Position{0, 1}, 5), Agent(9, Position{0, 0}, 5)
    };
    std::unordered_map<int, int> goals = {{2, 1}, {9, 2}};

    auto paths = planCBSPaths(graph, agents, goals, 4, 100);

    ASSERT_TRUE(paths.has_value());
    EXPECT_FALSE(findFirstConflict(graph, paths.value()).has_value());
    EXPECT_EQ(paths->at(2).back(), 1);
    EXPECT_EQ(paths->at(9).back(), 2);
    EXPECT_EQ(paths->at(2).size() + paths->at(9).size() - 2, 2u);
}

TEST(CBSTest, RoutesDoNotCutBlockedCorners) {
    Grid grid(3, 3);
    grid.setCellType(0, 1, CellType::Shelf);
    Graph graph(grid);
    //std::vector<Agent> agents = {Agent(2, Position{0, 0})};
    std::vector<Agent> agents = {Agent(2, Position{0, 0}, 5)};
    std::unordered_map<int, int> goals = {{2, 4}};

    auto paths = planCBSPaths(graph, agents, goals, 3, 10);

    ASSERT_TRUE(paths.has_value());
    EXPECT_EQ(paths->at(2), (std::vector<int>{0, 3, 4}));
}

TEST(CBSTest, RepeatedCallsAndInputOrderProduceSameTieResult) {
    Grid grid(2, 2);
    Graph graph(grid);
    std::vector<Agent> agents = {
        //Agent(9, Position{0, 1}), Agent(2, Position{0, 0})
        Agent(9, Position{0, 1}, 5), Agent(2, Position{0, 0}, 5)
    };
    std::unordered_map<int, int> goals = {{9, 2}, {2, 3}};

    auto first = planCBSPaths(graph, agents, goals, 3, 100);
    auto second = planCBSPaths(graph, agents, goals, 3, 100);
    std::reverse(agents.begin(), agents.end());
    auto reversed = planCBSPaths(graph, agents, goals, 3, 100);

    ASSERT_TRUE(first.has_value());
    ASSERT_TRUE(second.has_value());
    ASSERT_TRUE(reversed.has_value());
    EXPECT_EQ(first.value(), second.value());
    EXPECT_EQ(first.value(), reversed.value());
    EXPECT_EQ(agents.at(0).getPosition().row, 0);
    EXPECT_EQ(agents.at(0).getPosition().col, 0);
    EXPECT_EQ(agents.at(0).getStatus(), AgentStatus::Idle);
    EXPECT_FALSE(agents.at(0).getOrderId().has_value());
}

TEST(CBSTest, UnreachableRootReturnsNoPlan) {
    Grid grid(3, 3);
    for (int col = 0; col < 3; col++) {
        grid.setCellType(1, col, CellType::Shelf);
    }
    Graph graph(grid);
    //std::vector<Agent> agents = {Agent(2, Position{0, 0})};
    std::vector<Agent> agents = {Agent(2, Position{0, 0}, 5)};
    std::unordered_map<int, int> goals = {{2, 8}};

    EXPECT_FALSE(planCBSPaths(graph, agents, goals, 5, 100).has_value());
}

TEST(CBSTest, ArrivalBeyondHorizonReturnsNoPlan) {
    Grid grid(3, 3);
    Graph graph(grid);
    //std::vector<Agent> agents = {Agent(2, Position{0, 0})};
    std::vector<Agent> agents = {Agent(2, Position{0, 0}, 5)};
    std::unordered_map<int, int> goals = {{2, 8}};

    EXPECT_FALSE(planCBSPaths(graph, agents, goals, 1, 100).has_value());
}

TEST(CBSTest, InfeasibleChildrenAreDiscarded) {
    Grid grid(1, 2);
    Graph graph(grid);
    std::vector<Agent> agents = {
        //Agent(2, Position{0, 0}), Agent(9, Position{0, 1})
        Agent(2, Position{0, 0}, 5), Agent(9, Position{0, 1}, 5)
    };
    std::unordered_map<int, int> goals = {{2, 1}, {9, 0}};

    EXPECT_FALSE(planCBSPaths(graph, agents, goals, 1, 100).has_value());
}

TEST(CBSTest, ExpansionLimitStopsSearchBeforeChildrenAreExamined) {
    Grid grid(2, 2);
    Graph graph(grid);
    std::vector<Agent> agents = {
        //Agent(2, Position{0, 0}), Agent(9, Position{0, 1})
        Agent(2, Position{0, 0}, 5), Agent(9, Position{0, 1}, 5)
    };
    std::unordered_map<int, int> goals = {{2, 3}, {9, 2}};

    EXPECT_FALSE(planCBSPaths(graph, agents, goals, 2, 1).has_value());
    EXPECT_TRUE(planCBSPaths(graph, agents, goals, 2, 2).has_value());
}

TEST(CBSTest, InitialSharedPositionHasNoValidBranch) {
    Grid grid(2, 2);
    Graph graph(grid);
    std::vector<Agent> agents = {
        //Agent(2, Position{0, 0}), Agent(9, Position{0, 0})
        Agent(2, Position{0, 0}, 5), Agent(9, Position{0, 0}, 5)
    };
    std::unordered_map<int, int> goals = {{2, 1}, {9, 2}};

    EXPECT_FALSE(planCBSPaths(graph, agents, goals, 3, 100).has_value());
}

TEST(CBSTest, SharedPermanentGoalCannotBeConflictFree) {
    Grid grid(2, 2);
    Graph graph(grid);
    std::vector<Agent> agents = {
        //Agent(2, Position{0, 0}), Agent(9, Position{0, 1})
        Agent(2, Position{0, 0}, 5), Agent(9, Position{0, 1}, 5)
    };
    std::unordered_map<int, int> goals = {{2, 3}, {9, 3}};

    EXPECT_FALSE(planCBSPaths(graph, agents, goals, 2, 100).has_value());
}

TEST(CBSTest, NegativeHorizonIsRejected) {
    Grid grid(2, 2);
    Graph graph(grid);
    std::vector<Agent> agents;
    std::unordered_map<int, int> goals;

    EXPECT_THROW(planCBSPaths(graph, agents, goals, -1, 10), std::invalid_argument);
}

TEST(CBSTest, NonpositiveExpansionLimitsAreRejected) {
    Grid grid(2, 2);
    Graph graph(grid);
    std::vector<Agent> agents;
    std::unordered_map<int, int> goals;

    EXPECT_THROW(planCBSPaths(graph, agents, goals, 2, 0), std::invalid_argument);
    EXPECT_THROW(planCBSPaths(graph, agents, goals, 2, -1), std::invalid_argument);
}

TEST(CBSTest, DuplicateAgentIDsAreRejected) {
    Grid grid(2, 2);
    Graph graph(grid);
    std::vector<Agent> agents = {
        //Agent(2, Position{0, 0}), Agent(2, Position{0, 1})
        Agent(2, Position{0, 0}, 5), Agent(2, Position{0, 1}, 5)
    };
    std::unordered_map<int, int> goals = {{2, 3}};

    EXPECT_THROW(planCBSPaths(graph, agents, goals, 3, 100), std::invalid_argument);
}

TEST(CBSTest, MissingGoalIsRejected) {
    Grid grid(2, 2);
    Graph graph(grid);
    //std::vector<Agent> agents = {Agent(2, Position{0, 0})};
    std::vector<Agent> agents = {Agent(2, Position{0, 0}, 5)};
    std::unordered_map<int, int> goals;

    EXPECT_THROW(planCBSPaths(graph, agents, goals, 3, 100), std::invalid_argument);
}

TEST(CBSTest, InvalidStartCoordinatesAreRejectedBeforeNodeConversion) {
    Grid grid(3, 3);
    Graph graph(grid);
    std::unordered_map<int, int> goals = {{2, 8}};
    // Column 3 would otherwise alias valid node 3; column -1 can alias node 2.
    for (Position position : {Position{-1, 0}, Position{3, 0}, Position{0, 3}, Position{1, -1}}) {
        //std::vector<Agent> agents = {Agent(2, position)};
        std::vector<Agent> agents = {Agent(2, position, 5)};
        EXPECT_THROW(planCBSPaths(graph, agents, goals, 3, 100), std::invalid_argument);
    }
}

TEST(CBSTest, BlockedStartAndBlockedOrInvalidGoalsAreRejected) {
    Grid grid(3, 3);
    grid.setCellType(1, 1, CellType::Shelf);
    Graph graph(grid);
    //std::vector<Agent> blocked = {Agent(2, Position{1, 1})};
    std::vector<Agent> blocked = {Agent(2, Position{1, 1}, 5)};
    std::unordered_map<int, int> validGoal = {{2, 8}};
    EXPECT_THROW(planCBSPaths(graph, blocked, validGoal, 3, 100), std::invalid_argument);

    //std::vector<Agent> agents = {Agent(2, Position{0, 0})};
    std::vector<Agent> agents = {Agent(2, Position{0, 0}, 5)};
    for (int node : {4, -1, 9}) {
        std::unordered_map<int, int> goals = {{2, node}};
        EXPECT_THROW(planCBSPaths(graph, agents, goals, 3, 100), std::invalid_argument);
    }
}
