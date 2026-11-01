#include <gtest/gtest.h>
#include <stdexcept>
#include <vector>
#include <nlohmann/json.hpp>
#include "warehouse_routing/scenario_loader.h"

using namespace warehouse_routing;

TEST(ScenarioLoaderTest, AgentsCheck) {
    Scenario scenario = loadScenario(SCENARIO_DATA_DIR "/test_scenario_small.json");
    ASSERT_EQ(scenario.agents.size(), 2);
    EXPECT_EQ(scenario.agents.at(0).getId(), 0);
    EXPECT_EQ(scenario.agents.at(0).getPosition().row, 1);
    EXPECT_EQ(scenario.agents.at(0).getPosition().col, 1);
    EXPECT_EQ(scenario.agents.at(0).getCapacity(), 5);

    EXPECT_EQ(scenario.agents.at(1).getId(), 1);
    EXPECT_EQ(scenario.agents.at(1).getPosition().row, 1);
    EXPECT_EQ(scenario.agents.at(1).getPosition().col, 2);
    EXPECT_EQ(scenario.agents.at(1).getCapacity(), 5);

    ASSERT_EQ(scenario.orders.size(), 2);
    EXPECT_EQ(scenario.orders.at(0).getId(), 0);
    ASSERT_EQ(scenario.orders.at(0).getLines().size(), 2);
    EXPECT_EQ(scenario.orders.at(0).getLines().at(0).itemID, "SKU_A");
    EXPECT_EQ(scenario.orders.at(0).getLines().at(0).reqQuantity, 3);
    EXPECT_EQ(scenario.orders.at(0).getLines().at(0).delQuantity, 0);
    EXPECT_EQ(scenario.orders.at(0).getLines().at(1).itemID, "SKU_B");
    EXPECT_EQ(scenario.orders.at(0).getLines().at(1).reqQuantity, 2);
    EXPECT_EQ(scenario.orders.at(0).getLines().at(1).delQuantity, 0);
    EXPECT_EQ(scenario.orders.at(0).getDeliveryPosition().row, 1);
    EXPECT_EQ(scenario.orders.at(0).getDeliveryPosition().col, 1);
    EXPECT_EQ(scenario.orders.at(0).getStatus(), OrderStatus::Unclaimed);

    EXPECT_EQ(scenario.orders.at(1).getId(), 1);
    ASSERT_EQ(scenario.orders.at(1).getLines().size(), 1);
    EXPECT_EQ(scenario.orders.at(1).getLines().at(0).itemID, "SKU_A");
    EXPECT_EQ(scenario.orders.at(1).getLines().at(0).reqQuantity, 1);
    EXPECT_EQ(scenario.orders.at(1).getLines().at(0).delQuantity, 0);
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

TEST(ScenarioLoaderTest, BlockedDelivery) {
    EXPECT_THROW(loadScenario(SCENARIO_DATA_DIR "/blocked_delivery.json"), std::runtime_error);
}

TEST(ScenarioLoaderTest, InventoryFieldsAndExistingAgentsAndOrdersLoad) {
    const Scenario scenario = loadScenario(SCENARIO_DATA_DIR "/test_scenario_inventory.json");
    ASSERT_EQ(scenario.inventory.size(), 3);
    ASSERT_EQ(scenario.agents.size(), 2);
    ASSERT_EQ(scenario.orders.size(), 2);
    EXPECT_EQ(scenario.agents.at(0).getPosition().row, 1);
    EXPECT_EQ(scenario.agents.at(0).getPosition().col, 1);
    ASSERT_EQ(scenario.orders.at(0).getLines().size(), 2);
    EXPECT_EQ(scenario.orders.at(0).getLines().at(0).itemID, "SKU_A");
    EXPECT_EQ(scenario.orders.at(0).getLines().at(0).reqQuantity, 3);
    EXPECT_EQ(scenario.orders.at(0).getLines().at(0).delQuantity, 0);
    EXPECT_EQ(scenario.orders.at(0).getLines().at(1).itemID, "SKU_B");
    EXPECT_EQ(scenario.orders.at(0).getLines().at(1).reqQuantity, 2);
    EXPECT_EQ(scenario.orders.at(0).getLines().at(1).delQuantity, 0);

    const std::vector<std::string> itemIDs{"SKU_A", "SKU_A", "SKU_B"};
    const std::vector<int> quantities{10, 5, 8};
    const std::vector<int> shelfColumns{35, 57, 79};
    for (std::size_t i = 0; i < scenario.inventory.size(); ++i) {
        SCOPED_TRACE(i);
        const auto& cell = scenario.inventory.at(i);
        EXPECT_EQ(cell.getItemID(), itemIDs.at(i));
        EXPECT_EQ(cell.getQuantity(), quantities.at(i));
        EXPECT_EQ(cell.getShelfPosition().row, 5);
        EXPECT_EQ(cell.getShelfPosition().col, shelfColumns.at(i));
        EXPECT_EQ(cell.getPickupPosition().row, 5);
        EXPECT_EQ(cell.getPickupPosition().col, shelfColumns.at(i) + 1);
    }
}

TEST(ScenarioLoaderTest, OrdersCanLoadWithoutInventory) {
    const Scenario scenario = loadScenario(SCENARIO_DATA_DIR "/test_scenario_small.json");
    EXPECT_TRUE(scenario.inventory.empty());
    EXPECT_EQ(scenario.agents.size(), 2);
    EXPECT_EQ(scenario.orders.size(), 2);
}

TEST(ScenarioLoaderTest, EmptyInventoryArrayIsAllowed) {
    const Scenario scenario = loadScenario(SCENARIO_DATA_DIR "/inventory_cases/empty_inventory.json");
    EXPECT_TRUE(scenario.inventory.empty());
}

TEST(ScenarioLoaderTest, ZeroInventoryStockIsAllowed) {
    const Scenario scenario = loadScenario(SCENARIO_DATA_DIR "/inventory_cases/zero_stock.json");
    ASSERT_EQ(scenario.inventory.size(), 1);
    EXPECT_EQ(scenario.inventory.at(0).getQuantity(), 0);
}

TEST(ScenarioLoaderTest, SeparateShelvesCanShareItemAndPickupPosition) {
    const Scenario scenario = loadScenario(SCENARIO_DATA_DIR "/inventory_cases/shared_pickup.json");
    ASSERT_EQ(scenario.inventory.size(), 2);
    EXPECT_EQ(scenario.inventory.at(0).getItemID(), scenario.inventory.at(1).getItemID());
    for (const auto& cell : scenario.inventory) {
        EXPECT_EQ(cell.getPickupPosition().row, 1);
        EXPECT_EQ(cell.getPickupPosition().col, 1);
    }
}

class InvalidInventoryScenarioTest : public testing::TestWithParam<const char*> {};

TEST_P(InvalidInventoryScenarioTest, RejectsInvalidInventory) {
    const std::string path = std::string(SCENARIO_DATA_DIR) + "/inventory_cases/" + GetParam() + ".json";
    EXPECT_THROW(loadScenario(path), std::runtime_error);
}

INSTANTIATE_TEST_SUITE_P(InventoryValidation, InvalidInventoryScenarioTest,
    testing::Values("not_array", "free_shelf", "negative_shelf", "out_of_bounds_shelf",
                    "blocked_pickup", "out_of_bounds_pickup", "diagonal_pickup",
                    "distant_pickup", "duplicate_shelf"));

TEST(ScenarioLoaderTest, InventoryConstructorValidationIsAppliedDuringLoading) {
    EXPECT_THROW(loadScenario(SCENARIO_DATA_DIR "/inventory_cases/negative_quantity.json"),
                 std::invalid_argument);
    EXPECT_THROW(loadScenario(SCENARIO_DATA_DIR "/inventory_cases/empty_item_id.json"),
                 std::invalid_argument);
}

TEST(ScenarioLoaderTest, OrdersCanExceedStockAndRequestUnavailableItems) {
    const Scenario scenario = loadScenario(SCENARIO_DATA_DIR "/order_cases/short_stock.json");
    ASSERT_EQ(scenario.inventory.size(), 1);
    EXPECT_EQ(scenario.inventory.at(0).getQuantity(), 1);
    ASSERT_EQ(scenario.orders.size(), 1);
    const auto& order = scenario.orders.at(0);
    ASSERT_EQ(order.getLines().size(), 2);
    EXPECT_EQ(order.getLines().at(0).reqQuantity, 5);
    EXPECT_EQ(order.getLines().at(1).itemID, "MISSING_SKU");
    EXPECT_EQ(order.getLines().at(1).reqQuantity, 2);
    EXPECT_EQ(order.getStatus(), OrderStatus::Unclaimed);
    for (const auto& line : order.getLines()) {
        EXPECT_EQ(line.delQuantity, 0);
    }
}

class InvalidOrderScenarioTest : public testing::TestWithParam<const char*> {};

TEST_P(InvalidOrderScenarioTest, RejectsInvalidOrderLines) {
    const std::string path = std::string(SCENARIO_DATA_DIR) + "/order_cases/" + GetParam() + ".json";
    EXPECT_THROW(loadScenario(path), std::invalid_argument);
}

INSTANTIATE_TEST_SUITE_P(OrderValidation, InvalidOrderScenarioTest,
    testing::Values("empty_lines", "lines_not_array", "empty_item_id", "duplicate_item_id",
                    "zero_quantity", "negative_quantity"));

TEST(ScenarioLoaderTest, MissingOrderLineFieldsAreRejected) {
    for (const char* name : {"missing_lines", "missing_item_id", "missing_quantity"}) {
        SCOPED_TRACE(name);
        const std::string path = std::string(SCENARIO_DATA_DIR) + "/order_cases/" + name + ".json";
        EXPECT_THROW(loadScenario(path), nlohmann::json::out_of_range);
    }
}

TEST(ScenarioLoaderTest, DifferentAgentCapacitiesLoad) {
    Scenario scenario = loadScenario(SCENARIO_DATA_DIR "/agent_cases/different_capacities.json");

    ASSERT_EQ(scenario.agents.size(), 2u);
    EXPECT_EQ(scenario.agents.at(0).getCapacity(), 1);
    EXPECT_EQ(scenario.agents.at(1).getCapacity(), 7);
    for (const Agent& agent : scenario.agents) {
        EXPECT_EQ(agent.getCurrentLoad(), 0);
        EXPECT_EQ(agent.getRemainingCapacity(), agent.getCapacity());
        EXPECT_TRUE(agent.getCarriedItems().empty());
        EXPECT_EQ(agent.getStatus(), AgentStatus::Idle);
        EXPECT_FALSE(agent.getOrderId().has_value());
    }
}

TEST(ScenarioLoaderTest, MissingAgentCapacityIsRejected) {
    EXPECT_THROW(loadScenario(SCENARIO_DATA_DIR "/agent_cases/missing_capacity.json"),
                 nlohmann::json::out_of_range);
}

TEST(ScenarioLoaderTest, ZeroAgentCapacityIsRejected) {
    EXPECT_THROW(loadScenario(SCENARIO_DATA_DIR "/agent_cases/zero_capacity.json"),
                 std::invalid_argument);
}

TEST(ScenarioLoaderTest, NegativeAgentCapacityIsRejected) {
    EXPECT_THROW(loadScenario(SCENARIO_DATA_DIR "/agent_cases/negative_capacity.json"),
                 std::invalid_argument);
}
