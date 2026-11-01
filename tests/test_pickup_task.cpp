#include <gtest/gtest.h>
#include <stdexcept>
#include "warehouse_routing/pickup_task.h"

using namespace warehouse_routing;

TEST(PickupTaskTest, ChoosesNearestLocationAndReturnsAllTaskFields) {
    Grid grid(6, 6);
    grid.setCellType(1, 1, CellType::Shelf);
    grid.setCellType(1, 4, CellType::Shelf);
    grid.setCellType(4, 4, CellType::Shelf);
    Graph graph(grid);

    Agent agent(7, Position{0, 0}, 10);
    Order order(3, {{"SKU_A", 5}, {"SKU_B", 2}}, Position{5, 5});
    agent.assignOrder(3);
    order.markAssigned();

    std::vector<InventoryCell> inventory = {
        InventoryCell("SKU_A", 10, Position{4, 4}, Position{4, 3}),
        InventoryCell("SKU_B", 10, Position{1, 4}, Position{0, 4}),
        InventoryCell("SKU_A", 10, Position{1, 1}, Position{0, 1})
    };

    const auto task = findNextPickup(graph, agent, order, inventory);
    ASSERT_TRUE(task.has_value());
    EXPECT_EQ(task->inventoryIndex, 2u);
    EXPECT_EQ(task->itemID, "SKU_A");
    EXPECT_EQ(task->pickupPosition.row, 0);
    EXPECT_EQ(task->pickupPosition.col, 1);
}

TEST(PickupTaskTest, ComparesCandidatesAcrossDifferentOrderLines) {
    Grid grid(6, 6);
    grid.setCellType(1, 1, CellType::Shelf);
    grid.setCellType(1, 4, CellType::Shelf);
    grid.setCellType(4, 4, CellType::Shelf);
    Graph graph(grid);

    Agent agent(7, Position{0, 0}, 10);
    Order order(3, {{"SKU_A", 5}, {"SKU_B", 2}}, Position{5, 5});
    agent.assignOrder(3);
    order.markAssigned();

    std::vector<InventoryCell> inventory = {
        InventoryCell("SKU_A", 10, Position{4, 4}, Position{4, 3}),
        InventoryCell("SKU_B", 10, Position{1, 4}, Position{0, 4}),
        InventoryCell("SKU_A", 10, Position{1, 1}, Position{0, 1})
    };

    agent.moveTo(Position{0, 5});
    const auto task = findNextPickup(graph, agent, order, inventory);
    ASSERT_TRUE(task.has_value());
    EXPECT_EQ(task->inventoryIndex, 1u);
    EXPECT_EQ(task->itemID, "SKU_B");
}

TEST(PickupTaskTest, AlreadyAtPickupIsValidZeroCostTask) {
    Grid grid(6, 6);
    grid.setCellType(1, 1, CellType::Shelf);
    grid.setCellType(1, 4, CellType::Shelf);
    grid.setCellType(4, 4, CellType::Shelf);
    Graph graph(grid);

    Agent agent(7, Position{0, 0}, 10);
    Order order(3, {{"SKU_A", 5}, {"SKU_B", 2}}, Position{5, 5});
    agent.assignOrder(3);
    order.markAssigned();

    std::vector<InventoryCell> inventory = {
        InventoryCell("SKU_A", 10, Position{4, 4}, Position{4, 3}),
        InventoryCell("SKU_B", 10, Position{1, 4}, Position{0, 4}),
        InventoryCell("SKU_A", 10, Position{1, 1}, Position{0, 1})
    };

    agent.moveTo(Position{0, 1});
    const auto task = findNextPickup(graph, agent, order, inventory);
    ASSERT_TRUE(task.has_value());
    EXPECT_EQ(task->inventoryIndex, 2u);
    EXPECT_EQ(task->pickupPosition.row, agent.getPosition().row);
    EXPECT_EQ(task->pickupPosition.col, agent.getPosition().col);
}

TEST(PickupTaskTest, DeliveredAndCarriedQuantitiesTogetherSatisfyAnItem) {
    Grid grid(6, 6);
    grid.setCellType(1, 1, CellType::Shelf);
    grid.setCellType(1, 4, CellType::Shelf);
    grid.setCellType(4, 4, CellType::Shelf);
    Graph graph(grid);

    Agent agent(7, Position{0, 0}, 10);
    Order order(3, {{"SKU_A", 5}, {"SKU_B", 2}}, Position{5, 5});
    agent.assignOrder(3);
    order.markAssigned();

    std::vector<InventoryCell> inventory = {
        InventoryCell("SKU_A", 10, Position{4, 4}, Position{4, 3}),
        InventoryCell("SKU_B", 10, Position{1, 4}, Position{0, 4}),
        InventoryCell("SKU_A", 10, Position{1, 1}, Position{0, 1})
    };

    order.recordDelivery("SKU_A", 3);
    agent.addCarriedItems("SKU_A", 2);
    const auto task = findNextPickup(graph, agent, order, inventory);
    ASSERT_TRUE(task.has_value());
    EXPECT_EQ(task->itemID, "SKU_B");
    EXPECT_EQ(task->inventoryIndex, 1u);
}

TEST(PickupTaskTest, PartiallyCarriedItemStillNeedsPickup) {
    Grid grid(6, 6);
    grid.setCellType(1, 1, CellType::Shelf);
    grid.setCellType(1, 4, CellType::Shelf);
    grid.setCellType(4, 4, CellType::Shelf);
    Graph graph(grid);

    Agent agent(7, Position{0, 0}, 10);
    Order order(3, {{"SKU_A", 5}, {"SKU_B", 2}}, Position{5, 5});
    agent.assignOrder(3);
    order.markAssigned();

    std::vector<InventoryCell> inventory = {
        InventoryCell("SKU_A", 10, Position{4, 4}, Position{4, 3}),
        InventoryCell("SKU_B", 10, Position{1, 4}, Position{0, 4}),
        InventoryCell("SKU_A", 10, Position{1, 1}, Position{0, 1})
    };

    order.recordDelivery("SKU_A", 2);
    agent.addCarriedItems("SKU_A", 2);
    const auto task = findNextPickup(graph, agent, order, inventory);
    ASSERT_TRUE(task.has_value());
    EXPECT_EQ(task->itemID, "SKU_A");
    EXPECT_EQ(task->inventoryIndex, 2u);
}

TEST(PickupTaskTest, FullyDeliveredLineIsSkipped) {
    Grid grid(6, 6);
    grid.setCellType(1, 1, CellType::Shelf);
    grid.setCellType(1, 4, CellType::Shelf);
    grid.setCellType(4, 4, CellType::Shelf);
    Graph graph(grid);

    Agent agent(7, Position{0, 0}, 10);
    Order order(3, {{"SKU_A", 5}, {"SKU_B", 2}}, Position{5, 5});
    agent.assignOrder(3);
    order.markAssigned();

    std::vector<InventoryCell> inventory = {
        InventoryCell("SKU_A", 10, Position{4, 4}, Position{4, 3}),
        InventoryCell("SKU_B", 10, Position{1, 4}, Position{0, 4}),
        InventoryCell("SKU_A", 10, Position{1, 1}, Position{0, 1})
    };

    order.recordDelivery("SKU_A", 5);
    const auto task = findNextPickup(graph, agent, order, inventory);
    ASSERT_TRUE(task.has_value());
    EXPECT_EQ(task->itemID, "SKU_B");
}

TEST(PickupTaskTest, CarriedTotalIsCalculatedSeparatelyForEachItem) {
    Grid grid(6, 6);
    grid.setCellType(1, 1, CellType::Shelf);
    grid.setCellType(1, 4, CellType::Shelf);
    grid.setCellType(4, 4, CellType::Shelf);
    Graph graph(grid);

    Agent agent(7, Position{0, 0}, 10);
    Order order(3, {{"SKU_A", 5}, {"SKU_B", 2}}, Position{5, 5});
    agent.assignOrder(3);
    order.markAssigned();

    std::vector<InventoryCell> inventory = {
        InventoryCell("SKU_A", 10, Position{4, 4}, Position{4, 3}),
        InventoryCell("SKU_B", 10, Position{1, 4}, Position{0, 4}),
        InventoryCell("SKU_A", 10, Position{1, 1}, Position{0, 1})
    };

    agent.addCarriedItems("SKU_A", 5);
    const auto task = findNextPickup(graph, agent, order, inventory);
    ASSERT_TRUE(task.has_value());
    EXPECT_EQ(task->itemID, "SKU_B");
}

TEST(PickupTaskTest, NoTaskWhenAllRequirementsAreDeliveredOrCarried) {
    Grid grid(6, 6);
    grid.setCellType(1, 1, CellType::Shelf);
    grid.setCellType(1, 4, CellType::Shelf);
    grid.setCellType(4, 4, CellType::Shelf);
    Graph graph(grid);

    Agent agent(7, Position{0, 0}, 10);
    Order order(3, {{"SKU_A", 5}, {"SKU_B", 2}}, Position{5, 5});
    agent.assignOrder(3);
    order.markAssigned();

    std::vector<InventoryCell> inventory = {
        InventoryCell("SKU_A", 10, Position{4, 4}, Position{4, 3}),
        InventoryCell("SKU_B", 10, Position{1, 4}, Position{0, 4}),
        InventoryCell("SKU_A", 10, Position{1, 1}, Position{0, 1})
    };

    order.recordDelivery("SKU_A", 3);
    agent.addCarriedItems("SKU_A", 2);
    agent.addCarriedItems("SKU_B", 2);
    EXPECT_FALSE(findNextPickup(graph, agent, order, inventory).has_value());
}

TEST(PickupTaskTest, FullAgentHasNoPickupTask) {
    Grid grid(6, 6);
    grid.setCellType(1, 1, CellType::Shelf);
    grid.setCellType(1, 4, CellType::Shelf);
    grid.setCellType(4, 4, CellType::Shelf);
    Graph graph(grid);

    Agent agent(7, Position{0, 0}, 10);
    Order order(3, {{"SKU_A", 5}, {"SKU_B", 2}}, Position{5, 5});
    agent.assignOrder(3);
    order.markAssigned();

    std::vector<InventoryCell> inventory = {
        InventoryCell("SKU_A", 10, Position{4, 4}, Position{4, 3}),
        InventoryCell("SKU_B", 10, Position{1, 4}, Position{0, 4}),
        InventoryCell("SKU_A", 10, Position{1, 1}, Position{0, 1})
    };

    agent.addCarriedItems("SKU_A", 4);
    agent.addCarriedItems("SKU_B", 6);
    EXPECT_EQ(agent.getRemainingCapacity(), 0);
    EXPECT_FALSE(findNextPickup(graph, agent, order, inventory).has_value());
}

TEST(PickupTaskTest, EmptyAndUnrelatedStockCannotProvidePickup) {
    Grid grid(6, 6);
    grid.setCellType(1, 1, CellType::Shelf);
    grid.setCellType(1, 4, CellType::Shelf);
    grid.setCellType(4, 4, CellType::Shelf);
    Graph graph(grid);

    Agent agent(7, Position{0, 0}, 10);
    Order order(3, {{"SKU_A", 5}, {"SKU_B", 2}}, Position{5, 5});
    agent.assignOrder(3);
    order.markAssigned();

    std::vector<InventoryCell> inventory = {
        InventoryCell("SKU_A", 10, Position{4, 4}, Position{4, 3}),
        InventoryCell("SKU_B", 10, Position{1, 4}, Position{0, 4}),
        InventoryCell("SKU_A", 10, Position{1, 1}, Position{0, 1})
    };

    EXPECT_FALSE(findNextPickup(graph, agent, order, {}).has_value());
    const std::vector<InventoryCell> unavailable{
        InventoryCell("SKU_A", 0, Position{1, 1}, Position{0, 1}),
        InventoryCell("UNRELATED", 10, Position{1, 4}, Position{0, 4})
    };
    EXPECT_FALSE(findNextPickup(graph, agent, order, unavailable).has_value());
}

TEST(PickupTaskTest, EmptyNearestLocationIsSkipped) {
    Grid grid(6, 6);
    grid.setCellType(1, 1, CellType::Shelf);
    grid.setCellType(1, 4, CellType::Shelf);
    grid.setCellType(4, 4, CellType::Shelf);
    Graph graph(grid);

    Agent agent(7, Position{0, 0}, 10);
    Order order(3, {{"SKU_A", 5}, {"SKU_B", 2}}, Position{5, 5});
    agent.assignOrder(3);
    order.markAssigned();

    std::vector<InventoryCell> inventory = {
        InventoryCell("SKU_A", 10, Position{4, 4}, Position{4, 3}),
        InventoryCell("SKU_B", 10, Position{1, 4}, Position{0, 4}),
        InventoryCell("SKU_A", 10, Position{1, 1}, Position{0, 1})
    };

    inventory.at(2).removeStock(10);
    const auto task = findNextPickup(graph, agent, order, inventory);
    ASSERT_TRUE(task.has_value());
    EXPECT_NE(task->inventoryIndex, 2u);
    EXPECT_GT(inventory.at(task->inventoryIndex).getQuantity(), 0);
}

TEST(PickupTaskTest, UnreachableLocationIsSkippedAndAlternativeCanBeSelected) {
    Grid grid(6, 6);
    grid.setCellType(1, 1, CellType::Shelf);
    grid.setCellType(1, 4, CellType::Shelf);
    grid.setCellType(4, 4, CellType::Shelf);
    Graph graph(grid);

    Agent agent(7, Position{0, 0}, 10);
    Order order(3, {{"SKU_A", 5}, {"SKU_B", 2}}, Position{5, 5});
    agent.assignOrder(3);
    order.markAssigned();

    std::vector<InventoryCell> inventory = {
        InventoryCell("SKU_A", 10, Position{4, 4}, Position{4, 3}),
        InventoryCell("SKU_B", 10, Position{1, 4}, Position{0, 4}),
        InventoryCell("SKU_A", 10, Position{1, 1}, Position{0, 1})
    };

    Grid divided = grid;
    for (int col = 0; col < divided.getCols(); ++col) {
        divided.setCellType(2, col, CellType::Shelf);
    }
    const Graph dividedGraph(divided);
    std::vector<InventoryCell> cells{
        InventoryCell("SKU_A", 10, Position{4, 4}, Position{4, 3})
    };
    EXPECT_FALSE(findNextPickup(dividedGraph, agent, order, cells).has_value());
    cells.push_back(InventoryCell("SKU_A", 10, Position{1, 1}, Position{0, 1}));
    const auto task = findNextPickup(dividedGraph, agent, order, cells);
    ASSERT_TRUE(task.has_value());
    EXPECT_EQ(task->inventoryIndex, 1u);
}

TEST(PickupTaskTest, TiesKeepFirstCandidateInOrderLineThenInventoryOrder) {
    Grid grid(6, 6);
    grid.setCellType(1, 1, CellType::Shelf);
    grid.setCellType(1, 4, CellType::Shelf);
    grid.setCellType(4, 4, CellType::Shelf);
    Graph graph(grid);

    Agent agent(7, Position{0, 0}, 10);
    Order order(3, {{"SKU_A", 5}, {"SKU_B", 2}}, Position{5, 5});
    agent.assignOrder(3);
    order.markAssigned();

    std::vector<InventoryCell> inventory = {
        InventoryCell("SKU_A", 10, Position{4, 4}, Position{4, 3}),
        InventoryCell("SKU_B", 10, Position{1, 4}, Position{0, 4}),
        InventoryCell("SKU_A", 10, Position{1, 1}, Position{0, 1})
    };

    Grid tiedGrid(6, 6);
    tiedGrid.setCellType(2, 1, CellType::Shelf);
    tiedGrid.setCellType(1, 2, CellType::Shelf);
    tiedGrid.setCellType(1, 0, CellType::Shelf);
    const Graph tiedGraph(tiedGrid);
    agent.moveTo(Position{0, 1});
    const std::vector<InventoryCell> tied{
        InventoryCell("SKU_B", 10, Position{2, 1}, Position{1, 1}),
        InventoryCell("SKU_A", 10, Position{1, 2}, Position{0, 2}),
        InventoryCell("SKU_A", 10, Position{1, 0}, Position{0, 0})
    };
    const auto task = findNextPickup(tiedGraph, agent, order, tied);
    ASSERT_TRUE(task.has_value());
    EXPECT_EQ(task->itemID, "SKU_A");
    EXPECT_EQ(task->inventoryIndex, 1u);
}

TEST(PickupTaskTest, PlanningDoesNotReserveStockOrModifyAgentOrOrder) {
    Grid grid(6, 6);
    grid.setCellType(1, 1, CellType::Shelf);
    grid.setCellType(1, 4, CellType::Shelf);
    grid.setCellType(4, 4, CellType::Shelf);
    Graph graph(grid);

    Agent agent(7, Position{0, 0}, 10);
    Order order(3, {{"SKU_A", 5}, {"SKU_B", 2}}, Position{5, 5});
    agent.assignOrder(3);
    order.markAssigned();

    std::vector<InventoryCell> inventory = {
        InventoryCell("SKU_A", 10, Position{4, 4}, Position{4, 3}),
        InventoryCell("SKU_B", 10, Position{1, 4}, Position{0, 4}),
        InventoryCell("SKU_A", 10, Position{1, 1}, Position{0, 1})
    };

    inventory.at(2).removeStock(9); // One unit is available against five required.
    const auto first = findNextPickup(graph, agent, order, inventory);
    const auto second = findNextPickup(graph, agent, order, inventory);
    ASSERT_TRUE(first.has_value());
    ASSERT_TRUE(second.has_value());
    EXPECT_EQ(first->inventoryIndex, 2u);
    EXPECT_EQ(first->inventoryIndex, second->inventoryIndex);
    EXPECT_EQ(inventory.at(0).getQuantity(), 10);
    EXPECT_EQ(inventory.at(1).getQuantity(), 10);
    EXPECT_EQ(inventory.at(2).getQuantity(), 1);
    EXPECT_EQ(agent.getCurrentLoad(), 0);
    EXPECT_EQ(agent.getOrderId(), 3);
    EXPECT_EQ(agent.getStatus(), AgentStatus::PickupPending);
    EXPECT_EQ(order.getStatus(), OrderStatus::Assigned);
    for (const OrderLine& line : order.getLines()) {
        EXPECT_EQ(line.delQuantity, 0);
    }
}

TEST(PickupTaskTest, RejectsIdleAndDeliveringAgents) {
    Grid grid(6, 6);
    grid.setCellType(1, 1, CellType::Shelf);
    grid.setCellType(1, 4, CellType::Shelf);
    grid.setCellType(4, 4, CellType::Shelf);
    Graph graph(grid);

    Agent agent(7, Position{0, 0}, 10);
    Order order(3, {{"SKU_A", 5}, {"SKU_B", 2}}, Position{5, 5});
    agent.assignOrder(3);
    order.markAssigned();

    std::vector<InventoryCell> inventory = {
        InventoryCell("SKU_A", 10, Position{4, 4}, Position{4, 3}),
        InventoryCell("SKU_B", 10, Position{1, 4}, Position{0, 4}),
        InventoryCell("SKU_A", 10, Position{1, 1}, Position{0, 1})
    };

    const Agent idle(8, Position{0, 0}, 10);
    EXPECT_THROW(findNextPickup(graph, idle, order, inventory), std::invalid_argument);
    agent.addCarriedItems("SKU_A", 1);
    agent.startDelivery();
    EXPECT_THROW(findNextPickup(graph, agent, order, inventory), std::invalid_argument);
}

TEST(PickupTaskTest, RejectsMismatchedAssignedOrderID) {
    Grid grid(6, 6);
    grid.setCellType(1, 1, CellType::Shelf);
    grid.setCellType(1, 4, CellType::Shelf);
    grid.setCellType(4, 4, CellType::Shelf);
    Graph graph(grid);

    Agent agent(7, Position{0, 0}, 10);
    Order order(3, {{"SKU_A", 5}, {"SKU_B", 2}}, Position{5, 5});
    agent.assignOrder(3);
    order.markAssigned();

    std::vector<InventoryCell> inventory = {
        InventoryCell("SKU_A", 10, Position{4, 4}, Position{4, 3}),
        InventoryCell("SKU_B", 10, Position{1, 4}, Position{0, 4}),
        InventoryCell("SKU_A", 10, Position{1, 1}, Position{0, 1})
    };

    Agent other(8, Position{0, 0}, 10);
    other.assignOrder(99);
    EXPECT_THROW(findNextPickup(graph, other, order, inventory), std::invalid_argument);
}

TEST(PickupTaskTest, RejectsUnclaimedCompletedAndUnfinishedOrders) {
    Grid grid(6, 6);
    grid.setCellType(1, 1, CellType::Shelf);
    grid.setCellType(1, 4, CellType::Shelf);
    grid.setCellType(4, 4, CellType::Shelf);
    Graph graph(grid);

    Agent agent(7, Position{0, 0}, 10);
    Order order(3, {{"SKU_A", 5}, {"SKU_B", 2}}, Position{5, 5});
    agent.assignOrder(3);
    order.markAssigned();

    std::vector<InventoryCell> inventory = {
        InventoryCell("SKU_A", 10, Position{4, 4}, Position{4, 3}),
        InventoryCell("SKU_B", 10, Position{1, 4}, Position{0, 4}),
        InventoryCell("SKU_A", 10, Position{1, 1}, Position{0, 1})
    };

    Order unclaimed(3, {{"SKU_A", 1}}, Position{5, 5});
    EXPECT_THROW(findNextPickup(graph, agent, unclaimed, inventory), std::invalid_argument);
    Order completed(3, {{"SKU_A", 1}}, Position{5, 5});
    completed.markAssigned();
    completed.recordDelivery("SKU_A", 1);
    completed.markCompleted();
    EXPECT_THROW(findNextPickup(graph, agent, completed, inventory), std::invalid_argument);
    Order unfinished(3, {{"SKU_A", 1}}, Position{5, 5});
    unfinished.markAssigned();
    unfinished.markUnfinished();
    EXPECT_THROW(findNextPickup(graph, agent, unfinished, inventory), std::invalid_argument);
}

