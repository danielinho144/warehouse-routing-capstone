#include <gtest/gtest.h>
#include "warehouse_routing/agent.h"
#include <stdexcept>

using namespace warehouse_routing;

TEST(AgentTest, StartsIdleWithoutOrder) {
    Agent agent(7, Position{2, 3});

    EXPECT_EQ(agent.getId(), 7);
    EXPECT_EQ(agent.getPosition().row, 2);
    EXPECT_EQ(agent.getPosition().col, 3);
    EXPECT_EQ(agent.getStatus(), AgentStatus::Idle);
    EXPECT_FALSE(agent.getOrderId().has_value());
}

TEST(AgentTest, SuccessfullAgentLifeCycle) {
    Agent agent(7, Position{2, 3});
    agent.assignOrder(3);
    EXPECT_EQ(agent.getStatus(), AgentStatus::PickupPending);
    ASSERT_TRUE(agent.getOrderId().has_value());
    EXPECT_EQ(agent.getOrderId().value(), 3);

    agent.pickUp();
    EXPECT_EQ(agent.getStatus(), AgentStatus::Delivering);
    ASSERT_TRUE(agent.getOrderId().has_value());
    EXPECT_EQ(agent.getOrderId().value(), 3);

    agent.completeDelivery();
    EXPECT_EQ(agent.getStatus(), AgentStatus::Idle);
    EXPECT_FALSE(agent.getOrderId().has_value());
}

TEST(AgentTest, AgentMovement) {
    Agent agent(7, Position{2, 3});
    agent.moveTo(Position{5,6});
    EXPECT_EQ(agent.getPosition().row, 5);
    EXPECT_EQ(agent.getPosition().col, 6);
}

TEST(AgentTest, InvalidAgentActions) {
    Agent agent(7, Position{2, 3});
    EXPECT_THROW(agent.pickUp(), std::runtime_error);
    EXPECT_THROW(agent.completeDelivery(), std::runtime_error);

    agent.assignOrder(3);
    EXPECT_THROW(agent.completeDelivery(), std::runtime_error);
    EXPECT_THROW(agent.assignOrder(4), std::runtime_error);

    EXPECT_EQ(agent.getStatus(), AgentStatus::PickupPending);
    ASSERT_TRUE(agent.getOrderId().has_value());
    EXPECT_EQ(agent.getOrderId().value(), 3);
}