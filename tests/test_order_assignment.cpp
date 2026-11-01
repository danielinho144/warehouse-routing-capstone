#include <gtest/gtest.h>
#include "warehouse_routing/order_assignment.h"

using namespace warehouse_routing;

// namespace {
// 
// // Shelf cells are blocked; every inventory pickup is in a neighboring aisle.
// Grid warehouseGrid() {
//     Grid grid(6, 6);
//     grid.setCellType(1, 1, CellType::Shelf);
//     grid.setCellType(1, 4, CellType::Shelf);
//     grid.setCellType(4, 4, CellType::Shelf);
//     return grid;
// }
// 
// }
// 

TEST(OrderAssignmentTest, ChoosesNearestStockedItemAcrossAllOrderLines) {
    //const Graph graph(warehouseGrid());
    Grid grid(6, 6);
    grid.setCellType(1, 1, CellType::Shelf);
    grid.setCellType(1, 4, CellType::Shelf);
    grid.setCellType(4, 4, CellType::Shelf);
    Graph graph(grid);
//     Agent agent(3, Position{0, 0});
    Agent agent(3, Position{0, 0}, 5);
    std::vector<Order> orders{
        Order(10, {{"FAR", 1}}, Position{5, 5}),
        Order(20, {{"FAR", 2}, {"NEAR", 3}}, Position{5, 5})
    };
    const std::vector<InventoryCell> inventory{
        InventoryCell("FAR", 10, Position{4, 4}, Position{4, 3}),
        InventoryCell("NEAR", 10, Position{1, 1}, Position{0, 1})
    };

    ASSERT_TRUE(assignNearestOrder(graph, agent, orders, inventory));
    EXPECT_EQ(agent.getOrderId(), 20);
    EXPECT_EQ(agent.getStatus(), AgentStatus::PickupPending);
    EXPECT_EQ(orders.at(0).getStatus(), OrderStatus::Unclaimed);
    EXPECT_EQ(orders.at(1).getStatus(), OrderStatus::Assigned);
    EXPECT_EQ(orders.at(1).getLines().at(0).delQuantity, 0);
    EXPECT_EQ(orders.at(1).getLines().at(1).delQuantity, 0);
}

TEST(OrderAssignmentTest, EqualDistanceChoosesLowerOrderID) {
    //const Graph graph(warehouseGrid());
    Grid grid(6, 6);
    grid.setCellType(1, 1, CellType::Shelf);
    grid.setCellType(1, 4, CellType::Shelf);
    grid.setCellType(4, 4, CellType::Shelf);
    Graph graph(grid);
//     Agent agent(3, Position{0, 0});
    Agent agent(3, Position{0, 0}, 5);
    std::vector<Order> orders{
        Order(20, {{"SKU_A", 1}}, Position{5, 5}),
        Order(10, {{"SKU_A", 2}}, Position{5, 5})
    };
    const std::vector<InventoryCell> inventory{
        InventoryCell("SKU_A", 10, Position{1, 1}, Position{0, 1})
    };

    ASSERT_TRUE(assignNearestOrder(graph, agent, orders, inventory));
    EXPECT_EQ(agent.getOrderId(), 10);
    EXPECT_EQ(orders.at(0).getStatus(), OrderStatus::Unclaimed);
    EXPECT_EQ(orders.at(1).getStatus(), OrderStatus::Assigned);
}

TEST(OrderAssignmentTest, SearchesAllLocationsForTheSameItem) {
    //const Graph graph(warehouseGrid());
    Grid grid(6, 6);
    grid.setCellType(1, 1, CellType::Shelf);
    grid.setCellType(1, 4, CellType::Shelf);
    grid.setCellType(4, 4, CellType::Shelf);
    Graph graph(grid);
//     Agent agent(3, Position{0, 0});
    Agent agent(3, Position{0, 0}, 5);
    std::vector<Order> orders{
        Order(10, {{"SKU_B", 1}}, Position{5, 5}),
        Order(20, {{"SKU_A", 1}}, Position{5, 5})
    };
    const std::vector<InventoryCell> inventory{
        InventoryCell("SKU_A", 10, Position{4, 4}, Position{4, 3}),
        InventoryCell("SKU_B", 10, Position{1, 4}, Position{0, 4}),
        InventoryCell("SKU_A", 10, Position{1, 1}, Position{0, 1})
    };

    ASSERT_TRUE(assignNearestOrder(graph, agent, orders, inventory));
    EXPECT_EQ(agent.getOrderId(), 20);
}

TEST(OrderAssignmentTest, ZeroCostPickupIsValidAndBeatsPositiveCost) {
    //const Graph graph(warehouseGrid());
    Grid grid(6, 6);
    grid.setCellType(1, 1, CellType::Shelf);
    grid.setCellType(1, 4, CellType::Shelf);
    grid.setCellType(4, 4, CellType::Shelf);
    Graph graph(grid);
//     Agent agent(3, Position{0, 1});
    Agent agent(3, Position{0, 1}, 5);
    std::vector<Order> orders{
        Order(10, {{"FAR", 1}}, Position{5, 5}),
        Order(20, {{"HERE", 1}}, Position{5, 5})
    };
    const std::vector<InventoryCell> inventory{
        InventoryCell("HERE", 10, Position{1, 1}, Position{0, 1}),
        InventoryCell("FAR", 10, Position{4, 4}, Position{4, 3})
    };

    ASSERT_TRUE(assignNearestOrder(graph, agent, orders, inventory));
    EXPECT_EQ(agent.getOrderId(), 20);
}

TEST(OrderAssignmentTest, BusyAgentsAreNotReassigned) {
    //const Graph graph(warehouseGrid());
    Grid grid(6, 6);
    grid.setCellType(1, 1, CellType::Shelf);
    grid.setCellType(1, 4, CellType::Shelf);
    grid.setCellType(4, 4, CellType::Shelf);
    Graph graph(grid);
    const std::vector<InventoryCell> inventory{
        InventoryCell("SKU_A", 10, Position{1, 1}, Position{0, 1})
    };
    for (bool delivering : {false, true}) {
        SCOPED_TRACE(delivering);
//         Agent agent(3, Position{0, 0});
        Agent agent(3, Position{0, 0}, 5);
        agent.assignOrder(99);
//         if (delivering) agent.pickUp();
        if (delivering) {
            agent.addCarriedItems("SKU_A", 1);
            agent.startDelivery();
        }
        const AgentStatus originalStatus = agent.getStatus();
        std::vector<Order> orders{Order(10, {{"SKU_A", 1}}, Position{5, 5})};
        EXPECT_FALSE(assignNearestOrder(graph, agent, orders, inventory));
        EXPECT_EQ(agent.getOrderId(), 99);
        EXPECT_EQ(agent.getStatus(), originalStatus);
        EXPECT_EQ(orders.at(0).getStatus(), OrderStatus::Unclaimed);
    }
}

TEST(OrderAssignmentTest, NoOrdersAvailableLeavesAgentIdle) {
    //const Graph graph(warehouseGrid());
    Grid grid(6, 6);
    grid.setCellType(1, 1, CellType::Shelf);
    grid.setCellType(1, 4, CellType::Shelf);
    grid.setCellType(4, 4, CellType::Shelf);
    Graph graph(grid);
//     Agent agent(1, Position{0, 0});
    Agent agent(1, Position{0, 0}, 5);
    std::vector<Order> orders;
    const std::vector<InventoryCell> inventory{
        InventoryCell("SKU_A", 10, Position{1, 1}, Position{0, 1})
    };
    EXPECT_FALSE(assignNearestOrder(graph, agent, orders, inventory));
    EXPECT_EQ(agent.getStatus(), AgentStatus::Idle);
    EXPECT_FALSE(agent.getOrderId().has_value());
}

TEST(OrderAssignmentTest, SkipsEmptyStockAndUnrelatedItems) {
    //const Graph graph(warehouseGrid());
    Grid grid(6, 6);
    grid.setCellType(1, 1, CellType::Shelf);
    grid.setCellType(1, 4, CellType::Shelf);
    grid.setCellType(4, 4, CellType::Shelf);
    Graph graph(grid);
//     Agent agent(1, Position{0, 0});
    Agent agent(1, Position{0, 0}, 5);
    std::vector<Order> orders{
        Order(10, {{"EMPTY", 1}}, Position{5, 5}),
        Order(20, {{"AVAILABLE", 1}}, Position{5, 5})
    };
    const std::vector<InventoryCell> inventory{
        InventoryCell("EMPTY", 0, Position{1, 1}, Position{0, 1}),
        InventoryCell("UNRELATED", 10, Position{1, 4}, Position{0, 4}),
        InventoryCell("AVAILABLE", 10, Position{4, 4}, Position{4, 3})
    };
    ASSERT_TRUE(assignNearestOrder(graph, agent, orders, inventory));
    EXPECT_EQ(agent.getOrderId(), 20);
    EXPECT_EQ(orders.at(0).getStatus(), OrderStatus::Unclaimed);
}

TEST(OrderAssignmentTest, NoMatchingStockLeavesOrderUnclaimed) {
    //const Graph graph(warehouseGrid());
    Grid grid(6, 6);
    grid.setCellType(1, 1, CellType::Shelf);
    grid.setCellType(1, 4, CellType::Shelf);
    grid.setCellType(4, 4, CellType::Shelf);
    Graph graph(grid);
    for (int mode : {0, 1, 2}) {
        SCOPED_TRACE(mode);
//         Agent agent(1, Position{0, 0});
        Agent agent(1, Position{0, 0}, 5);
        std::vector<Order> orders{Order(10, {{"SKU_A", 1}}, Position{5, 5})};
        std::vector<InventoryCell> inventory;
        if (mode == 1) inventory.emplace_back("SKU_A", 0, Position{1, 1}, Position{0, 1});
        if (mode == 2) inventory.emplace_back("SKU_B", 10, Position{1, 1}, Position{0, 1});
        EXPECT_FALSE(assignNearestOrder(graph, agent, orders, inventory));
        EXPECT_FALSE(agent.getOrderId().has_value());
        EXPECT_EQ(agent.getStatus(), AgentStatus::Idle);
        EXPECT_EQ(orders.at(0).getStatus(), OrderStatus::Unclaimed);
    }
}

TEST(OrderAssignmentTest, UnreachableStockIsSkippedForReachableAlternative) {
    //Grid grid = warehouseGrid();
    Grid grid(6, 6);
    grid.setCellType(1, 1, CellType::Shelf);
    grid.setCellType(1, 4, CellType::Shelf);
    grid.setCellType(4, 4, CellType::Shelf);
    for (int col = 0; col < grid.getCols(); ++col) grid.setCellType(2, col, CellType::Shelf);
    const Graph graph(grid);
//     Agent agent(1, Position{0, 0});
    Agent agent(1, Position{0, 0}, 5);
    std::vector<Order> orders{Order(10, {{"SKU_A", 1}}, Position{0, 5})};
    std::vector<InventoryCell> inventory{
        InventoryCell("SKU_A", 10, Position{4, 4}, Position{4, 3})
    };
    EXPECT_FALSE(assignNearestOrder(graph, agent, orders, inventory));
    EXPECT_FALSE(agent.getOrderId().has_value());
    EXPECT_EQ(agent.getStatus(), AgentStatus::Idle);
    EXPECT_EQ(orders.at(0).getStatus(), OrderStatus::Unclaimed);

    inventory.emplace_back("SKU_A", 10, Position{1, 4}, Position{0, 4});
    ASSERT_TRUE(assignNearestOrder(graph, agent, orders, inventory));
    EXPECT_EQ(agent.getOrderId(), 10);
}

TEST(OrderAssignmentTest, SkipsAssignedCompletedAndUnfinishedOrders) {
    //const Graph graph(warehouseGrid());
    Grid grid(6, 6);
    grid.setCellType(1, 1, CellType::Shelf);
    grid.setCellType(1, 4, CellType::Shelf);
    grid.setCellType(4, 4, CellType::Shelf);
    Graph graph(grid);
//     Agent agent(1, Position{0, 0});
    Agent agent(1, Position{0, 0}, 5);
    std::vector<Order> orders{
        Order(10, {{"SKU_A", 1}}, Position{5, 5}),
        Order(20, {{"SKU_A", 1}}, Position{5, 5}),
        Order(30, {{"SKU_A", 1}}, Position{5, 5}),
        Order(40, {{"SKU_A", 1}}, Position{5, 5})
    };
    orders.at(0).markAssigned();
    orders.at(1).markAssigned();
    orders.at(1).recordDelivery("SKU_A", 1);
    orders.at(1).markCompleted();
    orders.at(2).markAssigned();
    orders.at(2).markUnfinished();
    const std::vector<InventoryCell> inventory{
        InventoryCell("SKU_A", 10, Position{1, 1}, Position{0, 1})
    };
    ASSERT_TRUE(assignNearestOrder(graph, agent, orders, inventory));
    EXPECT_EQ(agent.getOrderId(), 40);
    EXPECT_EQ(orders.at(0).getStatus(), OrderStatus::Assigned);
    EXPECT_EQ(orders.at(1).getStatus(), OrderStatus::Completed);
    EXPECT_EQ(orders.at(2).getStatus(), OrderStatus::Unfinished);
    EXPECT_EQ(orders.at(3).getStatus(), OrderStatus::Assigned);
}

TEST(OrderAssignmentTest, PartialAvailabilityStillAllowsAssignmentWithoutChangingStock) {
    //const Graph graph(warehouseGrid());
    Grid grid(6, 6);
    grid.setCellType(1, 1, CellType::Shelf);
    grid.setCellType(1, 4, CellType::Shelf);
    grid.setCellType(4, 4, CellType::Shelf);
    Graph graph(grid);
//     Agent agent(1, Position{0, 0});
    Agent agent(1, Position{0, 0}, 5);
    std::vector<Order> orders{
        Order(10, {{"SKU_A", 5}, {"MISSING", 2}}, Position{5, 5})
    };
    const std::vector<InventoryCell> inventory{
        InventoryCell("SKU_A", 1, Position{1, 1}, Position{0, 1})
    };
    ASSERT_TRUE(assignNearestOrder(graph, agent, orders, inventory));
    EXPECT_EQ(agent.getOrderId(), 10);
    EXPECT_EQ(inventory.at(0).getQuantity(), 1);
    for (const auto& line : orders.at(0).getLines()) EXPECT_EQ(line.delQuantity, 0);
}

TEST(OrderAssignmentTest, MultipleAgentsCanTargetSameStockWithoutReservations) {
    //const Graph graph(warehouseGrid());
    Grid grid(6, 6);
    grid.setCellType(1, 1, CellType::Shelf);
    grid.setCellType(1, 4, CellType::Shelf);
    grid.setCellType(4, 4, CellType::Shelf);
    Graph graph(grid);
//     Agent first(1, Position{0, 0});
//     Agent second(2, Position{0, 2});
    Agent first(1, Position{0, 0}, 5);
    Agent second(2, Position{0, 2}, 5);
    std::vector<Order> orders{
        Order(10, {{"SKU_A", 1}}, Position{5, 5}),
        Order(20, {{"SKU_A", 1}}, Position{5, 5})
    };
    const std::vector<InventoryCell> inventory{
        InventoryCell("SKU_A", 1, Position{1, 1}, Position{0, 1})
    };
    ASSERT_TRUE(assignNearestOrder(graph, first, orders, inventory));
    ASSERT_TRUE(assignNearestOrder(graph, second, orders, inventory));
    EXPECT_EQ(first.getOrderId(), 10);
    EXPECT_EQ(second.getOrderId(), 20);
    EXPECT_EQ(inventory.at(0).getQuantity(), 1);
}
