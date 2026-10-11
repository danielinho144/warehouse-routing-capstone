#include <gtest/gtest.h>
#include "warehouse_routing/order_assignment.h"
#include <stdexcept>

using namespace warehouse_routing;

TEST(OrderAssignmentTest, NearestOrder) {
    Grid grid(5, 5);
    Graph graph(grid);

    Agent agent(3, Position{0, 0});

    std::vector<Order> orders = {
    Order(10, Position{4, 4}, Position{0, 4}),
    Order(20, Position{1, 1}, Position{0, 4})
    };

    ASSERT_TRUE(assignNearestOrder(graph, agent, orders));
    ASSERT_TRUE(agent.getOrderId().has_value());
    EXPECT_EQ(agent.getOrderId().value(), 20);
    EXPECT_EQ(agent.getStatus(), AgentStatus::PickupPending);
    EXPECT_EQ(orders.at(0).getStatus(), OrderStatus::Unclaimed);
    EXPECT_EQ(orders.at(1).getStatus(), OrderStatus::Assigned);
}

TEST(OrderAssignmentTest, EqualDistanceChoosesLowerId) {
    Grid grid(5, 5);
    Graph graph(grid);

    Agent agent(3, Position{0, 0});
    std::vector<Order> orders = {
        Order(20, Position{1, 0}, Position{0, 4}),
        Order(10, Position{0, 1}, Position{0, 4})
    };

    ASSERT_TRUE(assignNearestOrder(graph, agent, orders));
    EXPECT_EQ(agent.getOrderId(), 10);
    EXPECT_EQ(orders.at(1).getStatus(), OrderStatus::Assigned);
    EXPECT_EQ(orders.at(0).getStatus(), OrderStatus::Unclaimed);
}

TEST(OrderAssignmentTest, BusyAgentIsNotReassigned) {
    Grid grid(5, 5);
    Graph graph(grid);

    Agent agent(3, Position{0, 0});
    agent.assignOrder(99);
    EXPECT_EQ(agent.getOrderId(), 99);
    
    std::vector<Order> orders = {
    Order (10, Position{2, 2}, Position{4, 4})
    };

    EXPECT_FALSE(assignNearestOrder(graph, agent, orders));
    EXPECT_EQ(agent.getOrderId(), 99);
    EXPECT_EQ(agent.getStatus(), AgentStatus::PickupPending);
    EXPECT_EQ(orders.at(0).getStatus(), OrderStatus::Unclaimed);
}

TEST(OrderAssignmentTest, NoOrdersAvailable) {
    Grid grid(5, 5);
    Graph graph(grid);
    Agent agent(1, Position{0, 0});
    std::vector<Order> orders = {};
    EXPECT_FALSE(assignNearestOrder(graph, agent, orders));
    EXPECT_EQ(agent.getStatus(), AgentStatus::Idle);
    EXPECT_FALSE(agent.getOrderId().has_value());
}

TEST(OrderAssignmentTest, SkipsUnreachablePickup) {
    Grid grid(3, 3);
    grid.setCellType(1, 0, CellType::Shelf);
    grid.setCellType(1, 1, CellType::Shelf);
    grid.setCellType(1, 2, CellType::Shelf);

    Graph graph(grid);
    Agent agent(1, Position{0, 0});

    std::vector<Order> orders = {
        Order(10, Position{2, 0}, Position{2, 2}),
        Order(20, Position{0, 2}, Position{0, 1})
    };

    EXPECT_TRUE(assignNearestOrder(graph, agent, orders));
    EXPECT_EQ(agent.getOrderId(), 20);
    EXPECT_EQ(orders.at(0).getStatus(), OrderStatus::Unclaimed);
    EXPECT_EQ(orders.at(1).getStatus(), OrderStatus::Assigned);
}

TEST(OrderAssignmentTest, SkipsClaimedAndCompleteOrders) {
    Grid grid(5, 5);
    Graph graph(grid);
    Agent agent(1, Position{0, 0});
    std::vector<Order> orders = {
        Order(10, Position{0, 1}, Position{2, 2}),
        Order(20, Position{1, 0}, Position{0, 1}),
        Order(30, Position{4, 4}, Position{1, 1})
    };
    orders.at(0).markAssigned();
    orders.at(1).markAssigned();
    orders.at(1).markCompleted();

    EXPECT_TRUE(assignNearestOrder(graph, agent, orders));
    EXPECT_EQ(agent.getOrderId(), 30);
    EXPECT_EQ(orders.at(0).getStatus(), OrderStatus::Assigned);
    EXPECT_EQ(orders.at(1).getStatus(), OrderStatus::Completed);
    EXPECT_EQ(orders.at(2).getStatus(), OrderStatus::Assigned);
}

TEST(OrderAssignmentTest, AllPickupsUnreachable) {
    Grid grid(3, 3);
    grid.setCellType(1, 0, CellType::Shelf);
    grid.setCellType(1, 1, CellType::Shelf);
    grid.setCellType(1, 2, CellType::Shelf);
    Graph graph(grid);

    Agent agent(1, Position{0, 0});
    std::vector<Order> orders = {
        Order(10, Position{2, 0}, Position{2, 2})
    };

    EXPECT_FALSE(assignNearestOrder(graph, agent, orders));
    EXPECT_EQ(agent.getStatus(), AgentStatus::Idle);
    EXPECT_FALSE(agent.getOrderId().has_value());
    EXPECT_EQ(orders.at(0).getStatus(), OrderStatus::Unclaimed);
}