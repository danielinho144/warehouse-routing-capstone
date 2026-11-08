#include <gtest/gtest.h>
#include "warehouse_routing/simulation.h"
#include <stdexcept>

using namespace warehouse_routing;

TEST(SimulationFulfillmentTest, NoInventoryMarksUnclaimedOrderUnfinishedAndStops) {
    Scenario scenario;
    scenario.mapPath = MAP_TEST_DATA_DIR "/fulfillment.map";
    scenario.agents = {Agent(0, Position{0, 0}, 2)};
    scenario.orders = {Order(0, {{"SKU_A", 3}}, Position{4, 0})};
    Simulation sim(scenario);
    EXPECT_FALSE(sim.isFinished());

    sim.run(100);

    EXPECT_TRUE(sim.isFinished());
    EXPECT_EQ(sim.getCurrentTick(), 1);
    EXPECT_EQ(sim.getOrders().at(0).getStatus(), OrderStatus::Unfinished);
    EXPECT_EQ(sim.getOrders().at(0).getLines().at(0).delQuantity, 0);
    EXPECT_EQ(sim.getCompletedOrderCount(), 0);
    EXPECT_DOUBLE_EQ(sim.getThroughput(), 0.0);
    EXPECT_EQ(sim.getAgents().at(0).getStatus(), AgentStatus::Idle);
    EXPECT_FALSE(sim.getAgents().at(0).getOrderId().has_value());
}

TEST(SimulationFulfillmentTest, ZeroStockMarksOrderUnfinished) {
    Scenario scenario;
    scenario.mapPath = MAP_TEST_DATA_DIR "/fulfillment.map";
    scenario.agents = {Agent(0, Position{0, 0}, 2)};
    scenario.orders = {Order(0, {{"SKU_A", 3}}, Position{4, 0})};
    scenario.inventory = {InventoryCell("SKU_A", 0, Position{1, 2}, Position{0, 2})};
    Simulation sim(scenario);

    sim.run(100);

    EXPECT_TRUE(sim.isFinished());
    EXPECT_EQ(sim.getCurrentTick(), 1);
    EXPECT_EQ(sim.getOrders().at(0).getStatus(), OrderStatus::Unfinished);
    EXPECT_EQ(sim.getCompletedOrderCount(), 0);
}

TEST(SimulationFulfillmentTest, UnrelatedStockDoesNotKeepOrderOpen) {
    Scenario scenario;
    scenario.mapPath = MAP_TEST_DATA_DIR "/fulfillment.map";
    scenario.agents = {Agent(0, Position{0, 0}, 2)};
    scenario.orders = {Order(0, {{"SKU_A", 3}}, Position{4, 0})};
    scenario.inventory = {InventoryCell("SKU_B", 10, Position{1, 2}, Position{0, 2})};
    Simulation sim(scenario);

    sim.run(100);

    EXPECT_TRUE(sim.isFinished());
    EXPECT_EQ(sim.getOrders().at(0).getStatus(), OrderStatus::Unfinished);
    EXPECT_EQ(sim.getOrders().at(0).getLines().at(0).delQuantity, 0);
}

TEST(SimulationFulfillmentTest, AvailableLaterLineIsDeliveredBeforeOrderBecomesUnfinished) {
    Scenario scenario;
    scenario.mapPath = MAP_TEST_DATA_DIR "/fulfillment.map";
    scenario.agents = {Agent(0, Position{0, 0}, 2)};
    scenario.orders = {Order(0, {{"MISSING", 3}, {"SKU_B", 1}}, Position{4, 0})};
    scenario.inventory = {InventoryCell("SKU_B", 1, Position{1, 5}, Position{0, 5})};
    Simulation sim(scenario);

    sim.run(100);

    EXPECT_TRUE(sim.isFinished());
    EXPECT_GT(sim.getCurrentTick(), 1);
    EXPECT_LT(sim.getCurrentTick(), 100);
    EXPECT_EQ(sim.getOrders().at(0).getStatus(), OrderStatus::Unfinished);
    EXPECT_EQ(sim.getOrders().at(0).getLines().at(0).delQuantity, 0);
    EXPECT_EQ(sim.getOrders().at(0).getLines().at(1).delQuantity, 1);
    EXPECT_EQ(sim.getAgents().at(0).getCurrentLoad(), 0);
    EXPECT_FALSE(sim.getAgents().at(0).getOrderId().has_value());
    EXPECT_EQ(sim.getCompletedOrderCount(), 0);
}

TEST(SimulationFulfillmentTest, StockShortageAcrossTripsPreservesDeliveredQuantities) {
    Scenario scenario;
    scenario.mapPath = MAP_TEST_DATA_DIR "/fulfillment.map";
    scenario.agents = {Agent(0, Position{0, 0}, 2)};
    scenario.orders = {Order(0, {{"SKU_A", 5}, {"SKU_B", 2}}, Position{4, 0})};
    scenario.inventory = {
        InventoryCell("SKU_A", 3, Position{1, 2}, Position{0, 2}),
        InventoryCell("SKU_B", 1, Position{1, 5}, Position{0, 5})
    };
    Simulation sim(scenario);

    sim.run(200);

    EXPECT_TRUE(sim.isFinished());
    EXPECT_LT(sim.getCurrentTick(), 200);
    EXPECT_EQ(sim.getOrders().at(0).getStatus(), OrderStatus::Unfinished);
    EXPECT_EQ(sim.getOrders().at(0).getLines().at(0).delQuantity, 3);
    EXPECT_EQ(sim.getOrders().at(0).getLines().at(1).delQuantity, 1);
    EXPECT_EQ(sim.getAgents().at(0).getStatus(), AgentStatus::Idle);
    EXPECT_EQ(sim.getAgents().at(0).getCurrentLoad(), 0);
    EXPECT_FALSE(sim.getAgents().at(0).getOrderId().has_value());
}

TEST(SimulationFulfillmentTest, SufficientStockCompletesOrderAcrossTrips) {
    Scenario scenario;
    scenario.mapPath = MAP_TEST_DATA_DIR "/fulfillment.map";
    scenario.agents = {Agent(0, Position{0, 0}, 2)};
    scenario.orders = {Order(0, {{"SKU_A", 5}, {"SKU_B", 2}}, Position{4, 0})};
    scenario.inventory = {
        InventoryCell("SKU_A", 5, Position{1, 2}, Position{0, 2}),
        InventoryCell("SKU_B", 2, Position{1, 5}, Position{0, 5})
    };
    Simulation sim(scenario);

    sim.run(200);

    EXPECT_TRUE(sim.isFinished());
    EXPECT_LT(sim.getCurrentTick(), 200);
    EXPECT_EQ(sim.getOrders().at(0).getStatus(), OrderStatus::Completed);
    EXPECT_EQ(sim.getOrders().at(0).getLines().at(0).delQuantity, 5);
    EXPECT_EQ(sim.getOrders().at(0).getLines().at(1).delQuantity, 2);
    EXPECT_EQ(sim.getCompletedOrderCount(), 1);
    EXPECT_EQ(sim.getAgents().at(0).getCurrentLoad(), 0);
    EXPECT_FALSE(sim.getAgents().at(0).getOrderId().has_value());
}

TEST(SimulationFulfillmentTest, CompletedAndUnfinishedOrdersBothAllowStopping) {
    Scenario scenario;
    scenario.mapPath = MAP_TEST_DATA_DIR "/fulfillment.map";
    scenario.agents = {Agent(0, Position{0, 0}, 2)};
    scenario.orders = {
        Order(0, {{"MISSING", 3}}, Position{4, 0}),
        Order(1, {{"SKU_A", 1}}, Position{4, 0})
    };
    scenario.inventory = {InventoryCell("SKU_A", 1, Position{1, 2}, Position{0, 2})};
    Simulation sim(scenario);

    sim.run(100);

    EXPECT_TRUE(sim.isFinished());
    EXPECT_EQ(sim.getOrders().at(0).getStatus(), OrderStatus::Unfinished);
    EXPECT_EQ(sim.getOrders().at(1).getStatus(), OrderStatus::Completed);
    EXPECT_EQ(sim.getCompletedOrderCount(), 1);
    EXPECT_DOUBLE_EQ(sim.getThroughput(), 1.0 / sim.getCurrentTick());
    int finishedTick = sim.getCurrentTick();
    sim.run(200);
    EXPECT_EQ(sim.getCurrentTick(), finishedTick);
}

TEST(SimulationFulfillmentTest, UnreachableStockIsNotMarkedAsExhausted) {
    Scenario scenario;
    scenario.mapPath = MAP_TEST_DATA_DIR "/fulfillment_unreachable.map";
    scenario.agents = {Agent(0, Position{0, 0}, 2)};
    scenario.orders = {Order(0, {{"SKU_A", 1}}, Position{0, 0})};
    scenario.inventory = {InventoryCell("SKU_A", 1, Position{3, 2}, Position{4, 2})};
    Simulation sim(scenario);

    sim.run(5);

    EXPECT_FALSE(sim.isFinished());
    EXPECT_EQ(sim.getCurrentTick(), 5);
    EXPECT_EQ(sim.getOrders().at(0).getStatus(), OrderStatus::Unclaimed);
    EXPECT_EQ(sim.getOrders().at(0).getLines().at(0).delQuantity, 0);
}

TEST(SimulationFulfillmentTest, UnreachableRemainingStockRaisesRoutingErrorAfterDelivery) {
    Scenario scenario;
    scenario.mapPath = MAP_TEST_DATA_DIR "/fulfillment_unreachable.map";
    scenario.agents = {Agent(0, Position{0, 0}, 2)};
    scenario.orders = {Order(0, {{"SKU_A", 1}, {"SKU_B", 1}}, Position{0, 0})};
    scenario.inventory = {
        InventoryCell("SKU_A", 1, Position{1, 2}, Position{0, 2}),
        InventoryCell("SKU_B", 1, Position{3, 2}, Position{4, 2})
    };
    Simulation sim(scenario);

    EXPECT_THROW(sim.run(100), std::runtime_error);
    EXPECT_FALSE(sim.isFinished());
    EXPECT_EQ(sim.getOrders().at(0).getStatus(), OrderStatus::Assigned);
    EXPECT_EQ(sim.getOrders().at(0).getLines().at(0).delQuantity, 1);
    EXPECT_EQ(sim.getOrders().at(0).getLines().at(1).delQuantity, 0);
}

TEST(SimulationFulfillmentTest, NoOrdersAndIdleEmptyAgentsAreFinishedAtTickZero) {
    Scenario scenario;
    scenario.mapPath = MAP_TEST_DATA_DIR "/fulfillment.map";
    scenario.agents = {Agent(0, Position{0, 0}, 2)};
    Simulation sim(scenario);

    EXPECT_TRUE(sim.isFinished());
    sim.run(100);
    EXPECT_EQ(sim.getCurrentTick(), 0);
}

TEST(SimulationFulfillmentTest, TerminalOrderDoesNotHideOutstandingWorkInAnotherOrder) {
    Scenario scenario;
    scenario.mapPath = MAP_TEST_DATA_DIR "/fulfillment.map";
    scenario.orders = {
        Order(0, {{"SKU_A", 1}}, Position{4, 0}),
        Order(1, {{"SKU_B", 1}}, Position{4, 0})
    };
    scenario.orders.at(0).markUnfinished();
    Simulation unclaimed(scenario);
    EXPECT_FALSE(unclaimed.isFinished());
    scenario.orders.at(1).markAssigned();
    Simulation assigned(scenario);
    EXPECT_FALSE(assigned.isFinished());
    scenario.orders.at(1).recordDelivery("SKU_B", 1);
    scenario.orders.at(1).markCompleted();
    Simulation terminal(scenario);
    EXPECT_TRUE(terminal.isFinished());
}

TEST(SimulationFulfillmentTest, IdleAgentWithLoadPreventsStopping) {
    Scenario scenario;
    scenario.mapPath = MAP_TEST_DATA_DIR "/fulfillment.map";
    scenario.agents = {Agent(0, Position{0, 0}, 2)};
    scenario.agents.at(0).addCarriedItems("SKU_A", 1);
    Simulation sim(scenario);
    EXPECT_FALSE(sim.isFinished());
}

TEST(SimulationFulfillmentTest, AssignedAgentPreventsStoppingEvenWithTerminalOrder) {
    Scenario scenario;
    scenario.mapPath = MAP_TEST_DATA_DIR "/fulfillment.map";
    scenario.orders = {Order(0, {{"SKU_A", 1}}, Position{4, 0})};
    scenario.orders.at(0).markUnfinished();
    scenario.agents = {Agent(0, Position{0, 0}, 2)};
    scenario.agents.at(0).assignOrder(0);
    Simulation sim(scenario);
    EXPECT_FALSE(sim.isFinished());
}

TEST(SimulationFulfillmentTest, DeliveringAgentPreventsStoppingAfterUnload) {
    Scenario scenario;
    scenario.mapPath = MAP_TEST_DATA_DIR "/fulfillment.map";
    scenario.agents = {Agent(0, Position{0, 0}, 2)};
    scenario.agents.at(0).assignOrder(0);
    scenario.agents.at(0).addCarriedItems("SKU_A", 1);
    scenario.agents.at(0).startDelivery();
    scenario.agents.at(0).removeCarriedItems("SKU_A", 1);
    Simulation sim(scenario);
    EXPECT_FALSE(sim.isFinished());
}

TEST(SimulationFulfillmentTest, TwoAgentsCompeteForLastUnitAndEmptyAgentIsReleased) {
    Scenario scenario;
    scenario.mapPath = MAP_TEST_DATA_DIR "/fulfillment.map";
    scenario.agents = {
        Agent(0, Position{0, 1}, 2),
        Agent(1, Position{0, 4}, 2)
    };
    scenario.orders = {
        Order(0, {{"SKU_A", 1}}, Position{4, 0}),
        Order(1, {{"SKU_A", 1}}, Position{4, 0})
    };
    scenario.inventory = {
        InventoryCell("SKU_A", 1, Position{1, 2}, Position{0, 2})
    };
    Simulation sim(scenario);

    sim.step();

    EXPECT_EQ(sim.getOrders().at(0).getStatus(), OrderStatus::Assigned);
    EXPECT_EQ(sim.getOrders().at(1).getStatus(), OrderStatus::Assigned);
    EXPECT_EQ(sim.getAgents().at(0).getCurrentLoad(), 1);
    EXPECT_EQ(sim.getAgents().at(1).getStatus(), AgentStatus::PickupPending);
    EXPECT_EQ(sim.getAgents().at(1).getOrderId(), 1);
    EXPECT_EQ(sim.getAgents().at(1).getCurrentLoad(), 0);

    EXPECT_FALSE(sim.isFinished());
    while (sim.getOrders().at(1).getStatus() == OrderStatus::Assigned && sim.getCurrentTick() < 100) {
        sim.step();
    }

    EXPECT_EQ(sim.getOrders().at(1).getStatus(), OrderStatus::Unfinished);
    EXPECT_EQ(sim.getAgents().at(1).getStatus(), AgentStatus::Idle);
    EXPECT_FALSE(sim.getAgents().at(1).getOrderId().has_value());
    EXPECT_EQ(sim.getAgents().at(1).getCurrentLoad(), 0);

    EXPECT_NO_THROW(sim.run(100));

    EXPECT_TRUE(sim.isFinished());
    EXPECT_LT(sim.getCurrentTick(), 100);
    EXPECT_EQ(sim.getCompletedOrderCount(), 1);
    EXPECT_EQ(sim.getOrders().at(0).getStatus(), OrderStatus::Completed);
    EXPECT_EQ(sim.getOrders().at(0).getLines().at(0).delQuantity, 1);
    EXPECT_EQ(sim.getOrders().at(1).getLines().at(0).delQuantity, 0);
    for (const Agent& agent : sim.getAgents()) {
        EXPECT_EQ(agent.getStatus(), AgentStatus::Idle);
        EXPECT_FALSE(agent.getOrderId().has_value());
        EXPECT_TRUE(agent.getCarriedItems().empty());
    }
    EXPECT_EQ(scenario.inventory.at(0).getQuantity(), 1);
}

TEST(SimulationFulfillmentTest, AgentLosingLastStockDeliversOtherCarriedItemsBeforeClosing) {
    Scenario scenario;
    scenario.mapPath = MAP_TEST_DATA_DIR "/fulfillment.map";
    scenario.agents = {
        Agent(0, Position{0, 0}, 2),
        Agent(1, Position{0, 4}, 2)
    };
    scenario.orders = {
        Order(0, {{"SKU_A", 1}}, Position{4, 0}),
        Order(1, {{"SKU_A", 1}, {"SKU_B", 1}}, Position{4, 0})
    };
    scenario.inventory = {
        InventoryCell("SKU_A", 1, Position{1, 2}, Position{0, 2}),
        InventoryCell("SKU_B", 1, Position{1, 5}, Position{0, 5})
    };
    Simulation sim(scenario);

    sim.step();

    EXPECT_EQ(sim.getAgents().at(1).getCurrentLoad(), 1);
    EXPECT_EQ(sim.getAgents().at(1).getStatus(), AgentStatus::PickupPending);
    EXPECT_EQ(sim.getOrders().at(1).getLines().at(1).delQuantity, 0);

    EXPECT_NO_THROW(sim.run(100));

    EXPECT_TRUE(sim.isFinished());
    EXPECT_LT(sim.getCurrentTick(), 100);
    EXPECT_EQ(sim.getCompletedOrderCount(), 1);
    EXPECT_EQ(sim.getOrders().at(0).getStatus(), OrderStatus::Completed);
    EXPECT_EQ(sim.getOrders().at(1).getStatus(), OrderStatus::Unfinished);
    EXPECT_EQ(sim.getOrders().at(0).getLines().at(0).delQuantity, 1);
    EXPECT_EQ(sim.getOrders().at(1).getLines().at(0).delQuantity, 0);
    EXPECT_EQ(sim.getOrders().at(1).getLines().at(1).delQuantity, 1);
    for (const Agent& agent : sim.getAgents()) {
        EXPECT_EQ(agent.getStatus(), AgentStatus::Idle);
        EXPECT_FALSE(agent.getOrderId().has_value());
        EXPECT_EQ(agent.getCurrentLoad(), 0);
    }
}
