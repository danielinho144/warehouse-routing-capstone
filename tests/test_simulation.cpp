#include <gtest/gtest.h>
#include "warehouse_routing/simulation.h"
#include "warehouse_routing/scenario_loader.h"
#include <stdexcept>
#include <algorithm>
#include <cstdlib>
#include "warehouse_routing/map_loader.h"
#include "warehouse_routing/conflict_detection.h"

using namespace warehouse_routing;

TEST(SimulationTest, SmallScenarioCompletesSharedDeliveries) {
    const std::string directory = SCENARIO_DATA_DIR;
    const Scenario reference = loadScenario(directory + "/test_scenario_small.json");
    Simulation sim(reference);

    sim.run(100);

    EXPECT_TRUE(sim.isFinished());
    EXPECT_EQ(sim.getCompletedOrderCount(), 2);
    EXPECT_EQ(sim.getCurrentTick(), 5);
    EXPECT_DOUBLE_EQ(sim.getThroughput(), 0.4);
    ASSERT_EQ(sim.getAgents().size(), 2);
    for (const Agent& agent : sim.getAgents()) {
        EXPECT_EQ(agent.getStatus(), AgentStatus::Idle);
        EXPECT_FALSE(agent.getOrderId().has_value());
    }
}

TEST(SimulationTest, SmallScenario) {
    const std::string directory = SCENARIO_DATA_DIR;
    const Scenario reference = loadScenario(directory + "/test_scenario_small.json");
    Simulation sim(reference);
    EXPECT_EQ(sim.getCurrentTick(), 0);
    ASSERT_EQ(sim.getAgents().size(), 2);
    ASSERT_EQ(sim.getOrders().size(), 2);

    EXPECT_EQ(sim.getAgents().at(0).getId(), 0);
    EXPECT_EQ(sim.getAgents().at(0).getPosition().col, 1);
    EXPECT_EQ(sim.getAgents().at(0).getPosition().row, 1);
    EXPECT_EQ(sim.getAgents().at(0).getStatus(), AgentStatus::Idle);
    EXPECT_FALSE(sim.getAgents().at(0).getOrderId().has_value());

    EXPECT_EQ(sim.getOrders().at(0).getId(), 0);
    EXPECT_EQ(sim.getOrders().at(0).getStatus(), OrderStatus::Unclaimed);
}

TEST(SimulationTest, StepTest) {
    const std::string directory = SCENARIO_DATA_DIR;
    const Scenario reference = loadScenario(directory + "/test_scenario_small.json");
    Simulation sim(reference);

    sim.step();

    for(const Agent& agent : sim.getAgents()) {
        EXPECT_EQ(agent.getStatus(), AgentStatus::PickupPending);
        ASSERT_TRUE(agent.getOrderId().has_value());
    }

    ASSERT_EQ(sim.getAgents().size(), 2);
    ASSERT_EQ(sim.getOrders().size(), 2);
    
    EXPECT_EQ(sim.getAgents().at(0).getPosition().row, 1);
    EXPECT_EQ(sim.getAgents().at(0).getPosition().col, 2);

    EXPECT_EQ(sim.getAgents().at(1).getPosition().row, 2);
    EXPECT_EQ(sim.getAgents().at(1).getPosition().col, 2);

    EXPECT_TRUE(sim.getAgents().at(0).getOrderId().value() != sim.getAgents().at(1).getOrderId().value());
    EXPECT_EQ(sim.getOrders().at(0).getStatus(), OrderStatus::Assigned);
    EXPECT_EQ(sim.getOrders().at(1).getStatus(), OrderStatus::Assigned);

    EXPECT_EQ(sim.getCurrentTick(), 1);
}

TEST(SimulationTest, VertexConflictGivesLowerIdPriority) {
    const std::string directory = SCENARIO_DATA_DIR;
    Scenario reference = loadScenario(directory + "/test_scenario_small.json");

    reference.agents = {
        Agent(20, Position{1, 1}),
        Agent(10, Position{1, 3})
    };
    reference.orders = {
        Order(0, Position{1, 2}, Position{1, 4}),
        Order(1, Position{1, 2}, Position{1, 4})
    };

    Simulation sim(reference);
    sim.step();

    ASSERT_EQ(sim.getAgents().size(), 2);
    ASSERT_EQ(sim.getOrders().size(), 2);

    EXPECT_EQ(sim.getAgents().at(0).getPosition().row, 1);
    EXPECT_EQ(sim.getAgents().at(0).getPosition().col, 1);
    EXPECT_EQ(sim.getAgents().at(1).getPosition().row, 1);
    EXPECT_EQ(sim.getAgents().at(1).getPosition().col, 2);

    for (const Agent& agent : sim.getAgents()) {
        ASSERT_TRUE(agent.getOrderId().has_value());
    }
    EXPECT_EQ(sim.getAgents().at(0).getStatus(), AgentStatus::PickupPending);
    EXPECT_EQ(sim.getAgents().at(1).getStatus(), AgentStatus::Delivering);
    EXPECT_NE(sim.getAgents().at(0).getOrderId().value(), sim.getAgents().at(1).getOrderId().value());
    EXPECT_EQ(sim.getOrders().at(0).getStatus(), OrderStatus::Assigned);
    EXPECT_EQ(sim.getOrders().at(1).getStatus(), OrderStatus::Assigned);
    EXPECT_EQ(sim.getCurrentTick(), 1);
}

TEST(SimulationTest, WaitingPropagatesToFollowingAgent) {
    Scenario reference = loadScenario(SCENARIO_DATA_DIR "/test_scenario_small.json");
    reference.agents = {
        Agent(10, Position{1, 1}),
        Agent(20, Position{1, 2}),
        Agent(30, Position{1, 3})
    };
    reference.orders = {
        Order(0, Position{1, 2}, Position{5, 5}),
        Order(1, Position{1, 3}, Position{5, 5})
    };
    Simulation sim(reference);

    sim.step();

    ASSERT_EQ(sim.getAgents().size(), 3);
    for (std::size_t i = 0; i < reference.agents.size(); i++) {
        EXPECT_EQ(sim.getAgents().at(i).getPosition().row, 1);
        EXPECT_EQ(sim.getAgents().at(i).getPosition().col,
                  reference.agents.at(i).getPosition().col);

    }
    EXPECT_EQ(sim.getAgents().at(0).getStatus(), AgentStatus::PickupPending);
    EXPECT_EQ(sim.getAgents().at(1).getStatus(), AgentStatus::PickupPending);
    EXPECT_EQ(sim.getAgents().at(2).getStatus(), AgentStatus::Idle);
    EXPECT_FALSE(sim.getAgents().at(2).getOrderId().has_value());
    EXPECT_EQ(sim.getCurrentTick(), 1);
}

TEST(SimulationTest, WaitingPropagatesThroughMultipleAgents) {
    Scenario reference = loadScenario(SCENARIO_DATA_DIR "/test_scenario_small.json");
    reference.agents = {
        Agent(10, Position{1, 1}), Agent(20, Position{1, 2}),
        Agent(30, Position{1, 3}), Agent(40, Position{1, 4})
    };
    reference.orders = {
        Order(0, Position{1, 2}, Position{5, 5}),
        Order(1, Position{1, 3}, Position{5, 5}),
        Order(2, Position{1, 4}, Position{5, 5})
    };
    Simulation sim(reference);

    for (int tick = 1; tick <= 3; tick++) {
        sim.step();
        ASSERT_EQ(sim.getAgents().size(), 4);
        for (std::size_t i = 0; i < reference.agents.size(); i++) {
            EXPECT_EQ(sim.getAgents().at(i).getPosition().row, 1);
            EXPECT_EQ(sim.getAgents().at(i).getPosition().col,
                      reference.agents.at(i).getPosition().col);
        }
        EXPECT_EQ(sim.getCurrentTick(), tick);
    }
}

TEST(SimulationTest, SwapConflictUsesSafeDetour) {
    Scenario reference = loadScenario(SCENARIO_DATA_DIR "/test_scenario_small.json");
    reference.agents = {
        Agent(10, Position{1, 4}), Agent(20, Position{1, 5})
    };

    reference.orders = {
        Order(0, Position{1, 6}, Position{5, 5}),
        Order(1, Position{1, 1}, Position{5, 5})
    };

    Simulation sim(reference);
    sim.step();

    ASSERT_EQ(sim.getAgents().size(), 2);
    EXPECT_EQ(sim.getAgents().at(0).getOrderId(), 0);
    EXPECT_EQ(sim.getAgents().at(1).getOrderId(), 1);
    Grid grid = loadMap(reference.mapPath);
    Graph graph(grid);
    Position positionA = sim.getAgents().at(0).getPosition();
    Position positionB = sim.getAgents().at(1).getPosition();
    int currentA = graph.toNodeId(1, 4);
    int currentB = graph.toNodeId(1, 5);
    int nextA = graph.toNodeId(positionA.row, positionA.col);
    int nextB = graph.toNodeId(positionB.row, positionB.col);

    EXPECT_TRUE(nextA != currentA || nextB != currentB);
    EXPECT_FALSE(hasVertexConflict(nextA, nextB));
    EXPECT_FALSE(hasSwapConflict(currentA, nextA, currentB, nextB));
    EXPECT_FALSE(hasDiagonalConflict(graph, currentA, nextA, currentB, nextB));
    for (std::size_t i = 0; i < sim.getAgents().size(); i++) {
        Position start = reference.agents.at(i).getPosition();
        Position end = sim.getAgents().at(i).getPosition();
        int startID = graph.toNodeId(start.row, start.col);
        int endID = graph.toNodeId(end.row, end.col);
        const std::vector<int>& neighbors = graph.getNeighbors(startID);
        EXPECT_TRUE(startID == endID ||
                    std::find(neighbors.begin(), neighbors.end(), endID) != neighbors.end());
    }
    EXPECT_EQ(sim.getCurrentTick(), 1);
}

TEST(SimulationTest, DiagonalCrossingMakesAgentsWait) {
    Scenario reference = loadScenario(SCENARIO_DATA_DIR "/test_scenario_small.json");
    reference.agents = {
        Agent(10, Position{2, 4}), Agent(20, Position{2, 5})
    };
    reference.orders = {
        Order(0, Position{3, 5}, Position{5, 5}),
        Order(1, Position{4, 3}, Position{5, 5})
    };
    Simulation sim(reference);
    sim.step();

    ASSERT_EQ(sim.getAgents().size(), 2);
    EXPECT_EQ(sim.getAgents().at(0).getOrderId(), 0);
    EXPECT_EQ(sim.getAgents().at(1).getOrderId(), 1);
    EXPECT_EQ(sim.getAgents().at(0).getPosition().row, 2);
    EXPECT_EQ(sim.getAgents().at(0).getPosition().col, 4);
    EXPECT_EQ(sim.getAgents().at(1).getPosition().row, 2);
    EXPECT_EQ(sim.getAgents().at(1).getPosition().col, 5);
    EXPECT_EQ(sim.getCurrentTick(), 1);
}

TEST(SimulationTest, PickupStartsDeliveryRoute) {
    const std::string directory = SCENARIO_DATA_DIR;
    Scenario reference = loadScenario(directory + "/test_scenario_small.json");
    reference.agents = {
        Agent(0, Position{1, 1})
    };
    reference.orders = {
        Order(0, Position{1, 2}, Position{1, 3})
    };
    Simulation sim(reference);

    EXPECT_EQ(sim.getCompletedOrderCount(), 0);
    EXPECT_DOUBLE_EQ(sim.getThroughput(), 0.0);

    sim.step();

    ASSERT_EQ(sim.getAgents().size(), 1);
    ASSERT_EQ(sim.getOrders().size(), 1);
    EXPECT_EQ(sim.getAgents().at(0).getPosition().row, 1);
    EXPECT_EQ(sim.getAgents().at(0).getPosition().col, 2);
    EXPECT_EQ(sim.getAgents().at(0).getStatus(), AgentStatus::Delivering);
    ASSERT_TRUE(sim.getAgents().at(0).getOrderId().has_value());
    EXPECT_EQ(sim.getAgents().at(0).getOrderId().value(), 0);
    EXPECT_EQ(sim.getOrders().at(0).getStatus(), OrderStatus::Assigned);
    EXPECT_EQ(sim.getCurrentTick(), 1);
    EXPECT_EQ(sim.getCompletedOrderCount(), 0);
    EXPECT_DOUBLE_EQ(sim.getThroughput(), 0.0);

    sim.step();

    EXPECT_EQ(sim.getAgents().at(0).getPosition().row, 1);
    EXPECT_EQ(sim.getAgents().at(0).getPosition().col, 3);
    EXPECT_EQ(sim.getAgents().at(0).getStatus(), AgentStatus::Idle);
    EXPECT_FALSE(sim.getAgents().at(0).getOrderId().has_value());
    EXPECT_EQ(sim.getOrders().at(0).getStatus(), OrderStatus::Completed);
    EXPECT_EQ(sim.getCurrentTick(), 2);
    EXPECT_EQ(sim.getCompletedOrderCount(), 1);
    EXPECT_DOUBLE_EQ(sim.getThroughput(), 0.5);
}

TEST(SimulationTest, AgentReceivesAnotherOrderAfterDelivery) {
    const std::string directory = SCENARIO_DATA_DIR;
    Scenario reference = loadScenario(directory + "/test_scenario_small.json");
    reference.agents = {
        Agent(0, Position{1, 1})
    };
    reference.orders = {
        Order(0, Position{1, 2}, Position{1, 3}),
        Order(1, Position{1, 4}, Position{1, 5})
    };
    Simulation sim(reference);

    sim.step();

    ASSERT_EQ(sim.getAgents().size(), 1);
    ASSERT_EQ(sim.getOrders().size(), 2);
    EXPECT_EQ(sim.getAgents().at(0).getPosition().row, 1);
    EXPECT_EQ(sim.getAgents().at(0).getPosition().col, 2);
    EXPECT_EQ(sim.getAgents().at(0).getStatus(), AgentStatus::Delivering);
    ASSERT_TRUE(sim.getAgents().at(0).getOrderId().has_value());
    EXPECT_EQ(sim.getAgents().at(0).getOrderId().value(), 0);
    EXPECT_EQ(sim.getOrders().at(1).getStatus(), OrderStatus::Unclaimed);
    EXPECT_EQ(sim.getCurrentTick(), 1);

    sim.step();

    EXPECT_EQ(sim.getAgents().at(0).getPosition().row, 1);
    EXPECT_EQ(sim.getAgents().at(0).getPosition().col, 3);
    EXPECT_EQ(sim.getAgents().at(0).getStatus(), AgentStatus::Idle);
    EXPECT_FALSE(sim.getAgents().at(0).getOrderId().has_value());
    EXPECT_EQ(sim.getOrders().at(0).getStatus(), OrderStatus::Completed);
    EXPECT_EQ(sim.getOrders().at(1).getStatus(), OrderStatus::Unclaimed);
    EXPECT_EQ(sim.getCurrentTick(), 2);

    sim.step();

    EXPECT_EQ(sim.getAgents().at(0).getPosition().row, 1);
    EXPECT_EQ(sim.getAgents().at(0).getPosition().col, 4);
    EXPECT_EQ(sim.getAgents().at(0).getStatus(), AgentStatus::Delivering);
    ASSERT_TRUE(sim.getAgents().at(0).getOrderId().has_value());
    EXPECT_EQ(sim.getAgents().at(0).getOrderId().value(), 1);
    EXPECT_EQ(sim.getOrders().at(1).getStatus(), OrderStatus::Assigned);
    EXPECT_EQ(sim.getCurrentTick(), 3);

    sim.step();

    EXPECT_EQ(sim.getAgents().at(0).getPosition().row, 1);
    EXPECT_EQ(sim.getAgents().at(0).getPosition().col, 5);
    EXPECT_EQ(sim.getAgents().at(0).getStatus(), AgentStatus::Idle);
    EXPECT_FALSE(sim.getAgents().at(0).getOrderId().has_value());
    EXPECT_EQ(sim.getOrders().at(0).getStatus(), OrderStatus::Completed);
    EXPECT_EQ(sim.getOrders().at(1).getStatus(), OrderStatus::Completed);
    EXPECT_EQ(sim.getCurrentTick(), 4);
}

TEST(SimulationTest, AgentStartsOnPickupCell) {
    const std::string directory = SCENARIO_DATA_DIR;
    Scenario reference = loadScenario(directory + "/test_scenario_small.json");
    reference.agents = {
        Agent(0, Position{1, 1})
    };
    reference.orders = {
        Order(0, Position{1, 1}, Position{1, 2})
    };
    Simulation sim(reference);

    sim.step();

    ASSERT_EQ(sim.getAgents().size(), 1);
    ASSERT_EQ(sim.getOrders().size(), 1);
    EXPECT_EQ(sim.getAgents().at(0).getPosition().row, 1);
    EXPECT_EQ(sim.getAgents().at(0).getPosition().col, 1);
    EXPECT_EQ(sim.getAgents().at(0).getStatus(), AgentStatus::Delivering);
    ASSERT_TRUE(sim.getAgents().at(0).getOrderId().has_value());
    EXPECT_EQ(sim.getAgents().at(0).getOrderId().value(), 0);
    EXPECT_EQ(sim.getOrders().at(0).getStatus(), OrderStatus::Assigned);
    EXPECT_EQ(sim.getCurrentTick(), 1);

    sim.step();

    EXPECT_EQ(sim.getAgents().at(0).getPosition().row, 1);
    EXPECT_EQ(sim.getAgents().at(0).getPosition().col, 2);
    EXPECT_EQ(sim.getAgents().at(0).getStatus(), AgentStatus::Idle);
    EXPECT_FALSE(sim.getAgents().at(0).getOrderId().has_value());
    EXPECT_EQ(sim.getOrders().at(0).getStatus(), OrderStatus::Completed);
    EXPECT_EQ(sim.getCurrentTick(), 2);
}

TEST(SimulationTest, PickupAndDeliveryAreSameCell) {
    const std::string directory = SCENARIO_DATA_DIR;
    Scenario reference = loadScenario(directory + "/test_scenario_small.json");
    reference.agents = {
        Agent(0, Position{1, 1})
    };
    reference.orders = {
        Order(0, Position{1, 2}, Position{1, 2})
    };
    Simulation sim(reference);

    sim.step();

    ASSERT_EQ(sim.getAgents().size(), 1);
    ASSERT_EQ(sim.getOrders().size(), 1);
    EXPECT_EQ(sim.getAgents().at(0).getPosition().row, 1);
    EXPECT_EQ(sim.getAgents().at(0).getPosition().col, 2);
    EXPECT_EQ(sim.getAgents().at(0).getStatus(), AgentStatus::Idle);
    EXPECT_FALSE(sim.getAgents().at(0).getOrderId().has_value());
    EXPECT_EQ(sim.getOrders().at(0).getStatus(), OrderStatus::Completed);
    EXPECT_EQ(sim.getCurrentTick(), 1);

    sim.step();

    EXPECT_EQ(sim.getAgents().at(0).getPosition().row, 2);
    EXPECT_EQ(sim.getAgents().at(0).getPosition().col, 2);
    EXPECT_EQ(sim.getAgents().at(0).getStatus(), AgentStatus::Idle);
    EXPECT_FALSE(sim.getAgents().at(0).getOrderId().has_value());
    EXPECT_EQ(sim.getOrders().at(0).getStatus(), OrderStatus::Completed);
    EXPECT_EQ(sim.getCurrentTick(), 2);
}

TEST(SimulationTest, UnreachableDeliveryThrows) {
    const std::string directory = MAP_TEST_DATA_DIR;
    Scenario reference;
    reference.mapPath = directory + "/unreachable_delivery.map";
    reference.agents = {
        Agent(0, Position{0, 0})
    };
    reference.orders = {
        Order(0, Position{0, 1}, Position{2, 1})
    };
    Simulation sim(reference);

    EXPECT_THROW(sim.step(), std::runtime_error);

    ASSERT_EQ(sim.getAgents().size(), 1);
    ASSERT_EQ(sim.getOrders().size(), 1);
    EXPECT_EQ(sim.getAgents().at(0).getPosition().row, 0);
    EXPECT_EQ(sim.getAgents().at(0).getPosition().col, 1);
    EXPECT_EQ(sim.getAgents().at(0).getStatus(), AgentStatus::PickupPending);
    ASSERT_TRUE(sim.getAgents().at(0).getOrderId().has_value());
    EXPECT_EQ(sim.getAgents().at(0).getOrderId().value(), 0);
    EXPECT_EQ(sim.getOrders().at(0).getStatus(), OrderStatus::Assigned);
}

TEST(SimulationTest, NoOrdersIsAlreadyFinished) {
    const std::string directory = SCENARIO_DATA_DIR;
    Scenario reference = loadScenario(directory + "/test_scenario_small.json");
    reference.orders = {};
    Simulation sim(reference);

    EXPECT_TRUE(sim.isFinished());
    EXPECT_EQ(sim.getCurrentTick(), 0);

    sim.run(10);

    EXPECT_TRUE(sim.isFinished());
    EXPECT_TRUE(sim.getOrders().empty());
    EXPECT_EQ(sim.getCurrentTick(), 0);
}

TEST(SimulationTest, RunStopsWhenAllOrdersCompleted) {
    const std::string directory = SCENARIO_DATA_DIR;
    Scenario reference = loadScenario(directory + "/test_scenario_small.json");
    reference.agents = {
        Agent(0, Position{1, 1})
    };
    reference.orders = {
        Order(0, Position{1, 2}, Position{1, 3})
    };
    Simulation sim(reference);

    EXPECT_FALSE(sim.isFinished());
    sim.run(10);

    ASSERT_EQ(sim.getAgents().size(), 1);
    ASSERT_EQ(sim.getOrders().size(), 1);
    EXPECT_TRUE(sim.isFinished());
    EXPECT_EQ(sim.getCurrentTick(), 2);
    EXPECT_EQ(sim.getAgents().at(0).getPosition().row, 1);
    EXPECT_EQ(sim.getAgents().at(0).getPosition().col, 3);
    EXPECT_EQ(sim.getAgents().at(0).getStatus(), AgentStatus::Idle);
    EXPECT_FALSE(sim.getAgents().at(0).getOrderId().has_value());
    EXPECT_EQ(sim.getOrders().at(0).getStatus(), OrderStatus::Completed);

    sim.run(10);

    EXPECT_TRUE(sim.isFinished());
    EXPECT_EQ(sim.getCurrentTick(), 2);
}

TEST(SimulationTest, RunStopsAtTickLimit) {
    const std::string directory = MAP_TEST_DATA_DIR;
    Scenario reference;
    reference.mapPath = directory + "/swap_corridor.map";
    reference.agents = {
        Agent(0, Position{0, 4}),
        Agent(1, Position{0, 5})
    };
    reference.orders = {
        Order(0, Position{0, 6}, Position{0, 0}),
        Order(1, Position{0, 1}, Position{0, 0})
    };
    Simulation sim(reference);

    sim.run(5);

    ASSERT_EQ(sim.getAgents().size(), 2);
    ASSERT_EQ(sim.getOrders().size(), 2);
    EXPECT_FALSE(sim.isFinished());
    EXPECT_EQ(sim.getCurrentTick(), 5);
    EXPECT_EQ(sim.getAgents().at(0).getPosition().row, 0);
    EXPECT_EQ(sim.getAgents().at(0).getPosition().col, 4);
    EXPECT_EQ(sim.getAgents().at(1).getPosition().row, 0);
    EXPECT_EQ(sim.getAgents().at(1).getPosition().col, 5);
    EXPECT_EQ(sim.getAgents().at(0).getStatus(), AgentStatus::PickupPending);
    EXPECT_EQ(sim.getAgents().at(1).getStatus(), AgentStatus::PickupPending);
    EXPECT_EQ(sim.getOrders().at(0).getStatus(), OrderStatus::Assigned);
    EXPECT_EQ(sim.getOrders().at(1).getStatus(), OrderStatus::Assigned);
}

TEST(SimulationTest, RunUsesTotalTickLimit) {
    const std::string directory = MAP_TEST_DATA_DIR;
    Scenario reference;
    reference.mapPath = directory + "/swap_corridor.map";
    reference.agents = {
        Agent(0, Position{0, 4}),
        Agent(1, Position{0, 5})
    };
    reference.orders = {
        Order(0, Position{0, 6}, Position{0, 0}),
        Order(1, Position{0, 1}, Position{0, 0})
    };
    Simulation sim(reference);

    sim.step();
    EXPECT_EQ(sim.getCurrentTick(), 1);

    sim.run(3);

    EXPECT_FALSE(sim.isFinished());
    EXPECT_EQ(sim.getCurrentTick(), 3);

    sim.run(3);

    EXPECT_FALSE(sim.isFinished());
    EXPECT_EQ(sim.getCurrentTick(), 3);
}

TEST(SimulationTest, ClearingDeliverySkipsOccupiedNeighbor) {
    const std::string directory = SCENARIO_DATA_DIR;
    Scenario reference = loadScenario(directory + "/test_scenario_small.json");
    reference.agents = {
        Agent(0, Position{1, 2}),
        Agent(1, Position{2, 2})
    };
    reference.orders = {
        Order(0, Position{1, 4}, Position{1, 2})
    };
    reference.orders.at(0).markAssigned();
    reference.orders.at(0).markCompleted();
    Simulation sim(reference);

    Grid grid = loadMap(reference.mapPath);
    Graph graph(grid);

    for (int tick = 1; tick <= 4; tick++) {
        std::vector<Agent> previous = sim.getAgents();
        sim.step();
        ASSERT_EQ(sim.getAgents().size(), 2);
        for (std::size_t i = 0; i < sim.getAgents().size(); i++) {
            Position start = previous.at(i).getPosition();
            Position end = sim.getAgents().at(i).getPosition();
            int startID = graph.toNodeId(start.row, start.col);
            int endID = graph.toNodeId(end.row, end.col);
            const std::vector<int>& neighbors = graph.getNeighbors(startID);
            EXPECT_TRUE(startID == endID ||
                        std::find(neighbors.begin(), neighbors.end(), endID) != neighbors.end());
            Position otherStart = previous.at(1 - i).getPosition();
            EXPECT_FALSE(end.row == otherStart.row && end.col == otherStart.col);
        }
        Position startA = previous.at(0).getPosition();
        Position startB = previous.at(1).getPosition();
        Position endA = sim.getAgents().at(0).getPosition();
        Position endB = sim.getAgents().at(1).getPosition();
        int currentA = graph.toNodeId(startA.row, startA.col);
        int currentB = graph.toNodeId(startB.row, startB.col);
        int nextA = graph.toNodeId(endA.row, endA.col);
        int nextB = graph.toNodeId(endB.row, endB.col);
        EXPECT_FALSE(hasVertexConflict(nextA, nextB));
        EXPECT_FALSE(hasSwapConflict(currentA, nextA, currentB, nextB));
        EXPECT_FALSE(hasDiagonalConflict(graph, currentA, nextA, currentB, nextB));
        EXPECT_EQ(sim.getCurrentTick(), tick);
    }
    for (const Agent& agent : sim.getAgents()) {
        Position position = agent.getPosition();
        EXPECT_TRUE(std::abs(position.row - 1) > 1 || std::abs(position.col - 2) > 1);
        EXPECT_FALSE(position.row == 1 && position.col == 4);
        EXPECT_EQ(agent.getStatus(), AgentStatus::Idle);
        EXPECT_FALSE(agent.getOrderId().has_value());
    }
    EXPECT_EQ(sim.getOrders().at(0).getStatus(), OrderStatus::Completed);
}

TEST(SimulationTest, ClearingDeliverySkipsPickupAndDeliveryCells) {
    const std::string directory = SCENARIO_DATA_DIR;
    Scenario reference = loadScenario(directory + "/test_scenario_small.json");
    reference.agents = {
        Agent(0, Position{1, 2})
    };
    reference.orders = {
        Order(0, Position{2, 2}, Position{1, 2}),
        Order(1, Position{1, 5}, Position{1, 1})
    };
    for (Order& order : reference.orders) {
        order.markAssigned();
        order.markCompleted();
    }
    Simulation sim(reference);

    Grid grid = loadMap(reference.mapPath);
    Graph graph(grid);

    for (int tick = 1; tick <= 4; tick++) {
        Position start = sim.getAgents().at(0).getPosition();
        sim.step();
        ASSERT_EQ(sim.getAgents().size(), 1);
        Position end = sim.getAgents().at(0).getPosition();
        int startID = graph.toNodeId(start.row, start.col);
        int endID = graph.toNodeId(end.row, end.col);
        const std::vector<int>& neighbors = graph.getNeighbors(startID);
        EXPECT_TRUE(startID == endID ||
                    std::find(neighbors.begin(), neighbors.end(), endID) != neighbors.end());
        EXPECT_EQ(sim.getCurrentTick(), tick);
    }
    Position parked = sim.getAgents().at(0).getPosition();
    for (const Order& order : sim.getOrders()) {
        Position pickup = order.getPickupPosition();
        Position delivery = order.getDeliveryPosition();
        EXPECT_FALSE(parked.row == pickup.row && parked.col == pickup.col);
        EXPECT_TRUE(std::abs(parked.row - delivery.row) > 1 ||
                    std::abs(parked.col - delivery.col) > 1);
        EXPECT_EQ(order.getStatus(), OrderStatus::Completed);
    }
    EXPECT_EQ(sim.getAgents().at(0).getStatus(), AgentStatus::Idle);
    EXPECT_FALSE(sim.getAgents().at(0).getOrderId().has_value());

    sim.step();

    EXPECT_EQ(sim.getAgents().at(0).getPosition().row, parked.row);
    EXPECT_EQ(sim.getAgents().at(0).getPosition().col, parked.col);
    EXPECT_EQ(sim.getCurrentTick(), 5);
}

TEST(SimulationTest, ClearingDeliveryWaitsWhenNoSuitableNeighbor) {
    const std::string directory = MAP_TEST_DATA_DIR;
    Scenario reference;
    reference.mapPath = directory + "/no_delivery_parking.map";
    reference.agents = {
        Agent(0, Position{0, 0})
    };
    reference.orders = {
        Order(0, Position{0, 1}, Position{0, 0})
    };
    reference.orders.at(0).markAssigned();
    reference.orders.at(0).markCompleted();
    Simulation sim(reference);

    sim.step();

    ASSERT_EQ(sim.getAgents().size(), 1);
    EXPECT_EQ(sim.getAgents().at(0).getPosition().row, 0);
    EXPECT_EQ(sim.getAgents().at(0).getPosition().col, 0);
    EXPECT_EQ(sim.getAgents().at(0).getStatus(), AgentStatus::Idle);
    EXPECT_FALSE(sim.getAgents().at(0).getOrderId().has_value());
    EXPECT_EQ(sim.getCurrentTick(), 1);
}

TEST(SimulationTest, ClearingDeliveryAllowsAnotherAgentToDeliver) {
    const std::string directory = SCENARIO_DATA_DIR;
    Scenario reference = loadScenario(directory + "/test_scenario_small.json");
    reference.agents = {
        Agent(0, Position{1, 1}),
        Agent(1, Position{1, 6})
    };
    reference.orders = {
        Order(0, Position{1, 2}, Position{1, 3}),
        Order(1, Position{1, 6}, Position{1, 3})
    };
    Simulation sim(reference);

    sim.run(2);

    ASSERT_EQ(sim.getAgents().size(), 2);
    ASSERT_EQ(sim.getOrders().size(), 2);
    EXPECT_EQ(sim.getAgents().at(0).getPosition().row, 1);
    EXPECT_EQ(sim.getAgents().at(0).getPosition().col, 3);
    EXPECT_EQ(sim.getAgents().at(0).getStatus(), AgentStatus::Idle);
    EXPECT_EQ(sim.getOrders().at(0).getStatus(), OrderStatus::Completed);
    EXPECT_EQ(sim.getOrders().at(1).getStatus(), OrderStatus::Assigned);
    EXPECT_FALSE(sim.isFinished());
    EXPECT_EQ(sim.getCurrentTick(), 2);

    sim.step();

    EXPECT_EQ(sim.getAgents().at(0).getPosition().row, 2);
    EXPECT_EQ(sim.getAgents().at(0).getPosition().col, 3);
    EXPECT_EQ(sim.getAgents().at(1).getPosition().row, 1);
    EXPECT_EQ(sim.getAgents().at(1).getPosition().col, 4);
    EXPECT_EQ(sim.getCurrentTick(), 3);

    sim.run(10);

    EXPECT_TRUE(sim.isFinished());
    EXPECT_EQ(sim.getCurrentTick(), 4);
    EXPECT_EQ(sim.getAgents().at(1).getPosition().row, 1);
    EXPECT_EQ(sim.getAgents().at(1).getPosition().col, 3);
    for (const Agent& agent : sim.getAgents()) {
        EXPECT_EQ(agent.getStatus(), AgentStatus::Idle);
        EXPECT_FALSE(agent.getOrderId().has_value());
    }
    for (const Order& order : sim.getOrders()) {
        EXPECT_EQ(order.getStatus(), OrderStatus::Completed);
    }
}

TEST(SimulationTest, AgentDetoursAroundIdleAgent) {
    Scenario reference = loadScenario(SCENARIO_DATA_DIR "/test_scenario_small.json");
    reference.agents = {
        Agent(10, Position{1, 1}), Agent(20, Position{1, 2})
    };
    reference.orders = {Order(0, Position{1, 3}, Position{5, 5})};
    Simulation sim(reference);
    sim.step();

    ASSERT_EQ(sim.getAgents().size(), 2);
    Grid grid = loadMap(reference.mapPath);
    Graph graph(grid);
    Position position = sim.getAgents().at(0).getPosition();
    int startID = graph.toNodeId(1, 1);
    int nextID = graph.toNodeId(position.row, position.col);
    const std::vector<int>& neighbors = graph.getNeighbors(startID);
    EXPECT_NE(nextID, startID);
    EXPECT_NE(nextID, graph.toNodeId(1, 2));
    EXPECT_TRUE(std::find(neighbors.begin(), neighbors.end(), nextID) != neighbors.end());
    EXPECT_EQ(sim.getAgents().at(0).getOrderId(), 0);
    EXPECT_EQ(sim.getAgents().at(0).getStatus(), AgentStatus::PickupPending);
    EXPECT_EQ(sim.getAgents().at(1).getPosition().row, 1);
    EXPECT_EQ(sim.getAgents().at(1).getPosition().col, 2);
    EXPECT_EQ(sim.getAgents().at(1).getStatus(), AgentStatus::Idle);
    EXPECT_FALSE(sim.getAgents().at(1).getOrderId().has_value());
    EXPECT_EQ(sim.getCurrentTick(), 1);
}

TEST(SimulationTest, UnaffectedAgentMovesWhileOthersWait) {
    Scenario reference = loadScenario(SCENARIO_DATA_DIR "/test_scenario_small.json");
    reference.agents = {
        Agent(10, Position{1, 1}), Agent(20, Position{1, 3}),
        Agent(30, Position{5, 1})
    };
    reference.orders = {
        Order(0, Position{1, 2}, Position{6, 6}),
        Order(1, Position{1, 2}, Position{6, 6}),
        Order(2, Position{5, 3}, Position{6, 6})
    };
    Simulation sim(reference);
    sim.step();

    ASSERT_EQ(sim.getAgents().size(), 3);
    EXPECT_EQ(sim.getAgents().at(0).getPosition().row, 1);
    EXPECT_EQ(sim.getAgents().at(0).getPosition().col, 2);
    EXPECT_EQ(sim.getAgents().at(1).getPosition().row, 1);
    EXPECT_EQ(sim.getAgents().at(1).getPosition().col, 3);
    EXPECT_EQ(sim.getAgents().at(2).getPosition().row, 4);
    EXPECT_EQ(sim.getAgents().at(2).getPosition().col, 2);
    EXPECT_EQ(sim.getAgents().at(2).getOrderId(), 2);
    EXPECT_EQ(sim.getCurrentTick(), 1);
}
