#include <gtest/gtest.h>
#include <stdexcept>
#include <vector>
#include "warehouse_routing/scenario_loader.h"

using namespace warehouse_routing;

TEST(ScenarioLoaderTest, ExperimentScenariosShareWorkload) {
    const std::string directory = SCENARIO_DATA_DIR;
    const Scenario reference = loadScenario(directory + "/scenario_10_agents.json");
    ASSERT_EQ(reference.agents.size(), 10);
    ASSERT_EQ(reference.orders.size(), 20);

    for (int count : {2, 4, 6, 8, 10}) {
        SCOPED_TRACE(count);
        const Scenario scenario = loadScenario(
            directory + "/scenario_" + std::to_string(count) + "_agents.json");
        ASSERT_EQ(scenario.agents.size(), count);
        EXPECT_EQ(scenario.mapPath, reference.mapPath);
        ASSERT_EQ(scenario.orders.size(), reference.orders.size());
        for (int i = 0; i < count; ++i) {
            EXPECT_EQ(scenario.agents.at(i).getId(), reference.agents.at(i).getId());
            EXPECT_EQ(scenario.agents.at(i).getPosition().row, reference.agents.at(i).getPosition().row);
            EXPECT_EQ(scenario.agents.at(i).getPosition().col, reference.agents.at(i).getPosition().col);
        }
        for (std::size_t i = 0; i < reference.orders.size(); ++i) {
            const auto& actual = scenario.orders.at(i);
            const auto& expected = reference.orders.at(i);
            EXPECT_EQ(actual.getId(), expected.getId());
            EXPECT_EQ(actual.getPickupPosition().row, expected.getPickupPosition().row);
            EXPECT_EQ(actual.getPickupPosition().col, expected.getPickupPosition().col);
            EXPECT_EQ(actual.getDeliveryPosition().row, expected.getDeliveryPosition().row);
            EXPECT_EQ(actual.getDeliveryPosition().col, expected.getDeliveryPosition().col);
        }
    }
}

TEST(ScenarioLoaderTest, AgentsCheck) {
    Scenario scenario = loadScenario(SCENARIO_DATA_DIR "/test_scenario_small.json");
    ASSERT_EQ(scenario.agents.size(), 2);
    EXPECT_EQ(scenario.agents.at(0).getId(), 0);
    EXPECT_EQ(scenario.agents.at(0).getPosition().row, 1);
    EXPECT_EQ(scenario.agents.at(0).getPosition().col, 1);

    EXPECT_EQ(scenario.agents.at(1).getId(), 1);
    EXPECT_EQ(scenario.agents.at(1).getPosition().row, 1);
    EXPECT_EQ(scenario.agents.at(1).getPosition().col, 2);

    ASSERT_EQ(scenario.orders.size(), 2);
    EXPECT_EQ(scenario.orders.at(0).getId(), 0);
    EXPECT_EQ(scenario.orders.at(0).getPickupPosition().row, 2);
    EXPECT_EQ(scenario.orders.at(0).getPickupPosition().col, 3);
    EXPECT_EQ(scenario.orders.at(0).getDeliveryPosition().row, 1);
    EXPECT_EQ(scenario.orders.at(0).getDeliveryPosition().col, 1);
    EXPECT_EQ(scenario.orders.at(0).getStatus(), OrderStatus::Unclaimed);

    EXPECT_EQ(scenario.orders.at(1).getId(), 1);
    EXPECT_EQ(scenario.orders.at(1).getPickupPosition().row, 3);
    EXPECT_EQ(scenario.orders.at(1).getPickupPosition().col, 3);
    EXPECT_EQ(scenario.orders.at(1).getDeliveryPosition().row, 1);
    EXPECT_EQ(scenario.orders.at(1).getDeliveryPosition().col, 1);
    EXPECT_EQ(scenario.orders.at(1).getStatus(), OrderStatus::Unclaimed);
}

TEST(ScenarioLoaderTest, DupeAgentCheck) {
    EXPECT_THROW(loadScenario(SCENARIO_DATA_DIR "/duplicate_agent_id.json"), std::runtime_error);
}

TEST(ScenarioLoaderTest, DupeOrderCheck) {
    EXPECT_THROW(loadScenario(SCENARIO_DATA_DIR "/duplicate_order_id.json"), std::runtime_error);
}

TEST(ScenarioLoaderTest, DupeAgentStart) {
    EXPECT_THROW(loadScenario(SCENARIO_DATA_DIR "/duplicate_agent_start.json"), std::runtime_error);
}

TEST(ScenarioLoaderTest, BlockedAgentStart) {
    EXPECT_THROW(loadScenario(SCENARIO_DATA_DIR "/blocked_agent_start.json"), std::runtime_error);
}

TEST(ScenarioLoaderTest, OutofBoundsAgentStart) {
    EXPECT_THROW(loadScenario(SCENARIO_DATA_DIR "/out_of_bounds_agent_start.json"), std::runtime_error);
}

TEST(ScenarioLoaderTest, BlockedPickup) {
    EXPECT_THROW(loadScenario(SCENARIO_DATA_DIR "/blocked_pickup.json"), std::runtime_error);
}

TEST(ScenarioLoaderTest, BlockedDelivery) {
    EXPECT_THROW(loadScenario(SCENARIO_DATA_DIR "/blocked_delivery.json"), std::runtime_error);
}
