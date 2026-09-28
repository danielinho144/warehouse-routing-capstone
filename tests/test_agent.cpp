#include <gtest/gtest.h>
#include "warehouse_routing/agent.h"

using namespace warehouse_routing;

TEST(AgentTest, StartsIdleWithoutOrder) {
    Agent agent(7, Position{2, 3});

    EXPECT_EQ(agent.getId(), 7);
    EXPECT_EQ(agent.getPosition().row, 2);
    EXPECT_EQ(agent.getPosition().col, 3);
    EXPECT_EQ(agent.getStatus(), AgentStatus::Idle);
    EXPECT_FALSE(agent.getOrderId().has_value());
}
