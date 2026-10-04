#include <gtest/gtest.h>
#include <stdexcept>
#include "warehouse_routing/independent_planner.h"
#include "warehouse_routing/scenario.h"
#include "warehouse_routing/scenario_loader.h"
#include "warehouse_routing/map_loader.h"

using namespace warehouse_routing;

TEST(IndependentPlannerTest, ValidGoals) {
    Grid grid(3, 3);
    Graph graph(grid);

    Agent agent1(3, Position{0, 0});
    Agent agent2(7, Position{2, 2});

    std::vector<Agent> agents{agent1, agent2};

    int goalID1 = 8;
    int goalID2 = 0;
    std::unordered_map<int, int> goals {
        {3, goalID1},
        {7, goalID2}
    };
    
    auto paths = planIndependentPaths(graph, agents, goals);

    ASSERT_EQ(paths.size(), 2);
    ASSERT_TRUE(paths.contains(3));
    ASSERT_TRUE(paths.contains(7));

    EXPECT_EQ(paths.at(3), (std::vector<int>{0, 4, 8}));
    EXPECT_EQ(paths.at(7), (std::vector<int>{8, 4, 0}));
}

TEST(IndependentPlannerTest, MissingGoal) {
    Grid grid(3, 3);
    Graph graph(grid);
    std::unordered_map<int, int> goals;
    Agent agent(3, Position{0, 0});
    std::vector<Agent> agents{agent};
    EXPECT_THROW(planIndependentPaths(graph, agents, goals), std::runtime_error);
}

TEST(IndependentPlannerTest, UnreachableGoal) {
    Grid grid(3, 3);
    grid.setCellType(1, 0, CellType::Shelf);
    grid.setCellType(1, 1, CellType::Shelf);
    grid.setCellType(1, 2, CellType::Shelf);
    Graph graph(grid);
    Agent agent1(3, Position{0, 0});
    Agent agent2(7, Position{2, 0});
    std::vector<Agent> agents{agent1, agent2};
    int goalID1 = 8;
    int goalID2 = 8;
    std::unordered_map<int, int> goals {
    {3, goalID1},
    {7, goalID2}
    };
    auto paths = planIndependentPaths(graph, agents, goals);
    ASSERT_TRUE(paths.contains(3));
    ASSERT_TRUE(paths.contains(7));
    EXPECT_TRUE(paths.at(3).empty());
    EXPECT_EQ(paths.at(7), (std::vector<int>{6, 7, 8}));
}

TEST(IndependentPlannerTest, NoAgents) {
    Grid grid(3, 3);
    Graph graph(grid);
    std::vector<Agent> agents{};
    std::unordered_map<int, int> goals{};
    auto paths = planIndependentPaths(graph, agents, goals);
    EXPECT_TRUE(paths.empty());
}

TEST(IndependentPlannerTest, PlansWarehouseScenario) {
    Scenario scenario = loadScenario(SCENARIO_DATA_DIR "/scenario_2_agents.json");
    Grid grid = loadMap(scenario.mapPath);
    Graph graph(grid);
    ASSERT_EQ(scenario.agents.size(), 2);
    ASSERT_GE(scenario.orders.size(), 2);
    std::unordered_map<int, int> goals;

    for (std::size_t i = 0; i < scenario.agents.size(); i++) {
        int agentID = scenario.agents.at(i).getId();
        Position pickup = scenario.orders.at(i).getPickupPosition();
        goals[agentID] = graph.toNodeId(pickup.row, pickup.col);
    }
    auto paths = planIndependentPaths(graph, scenario.agents, goals);
    ASSERT_EQ(paths.size(), 2);

    for (const Agent& agent : scenario.agents) {
        int agentID = agent.getId();
        ASSERT_TRUE(paths.contains(agentID));

        const std::vector<int>& path = paths.at(agentID);
        ASSERT_FALSE(path.empty());

        Position start = agent.getPosition();
        EXPECT_EQ(path.front(), graph.toNodeId(start.row, start.col));
        EXPECT_EQ(path.back(), goals.at(agentID));

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
}