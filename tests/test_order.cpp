#include <gtest/gtest.h>
#include "warehouse_routing/order.h"

using namespace warehouse_routing;

TEST(OrderTest, OrderMatch) {
    Order order(3, Position{2,3}, Position{7,3});

    EXPECT_EQ(order.getId(), 3);
    EXPECT_EQ(order.getPickupPosition().row, 2);
    EXPECT_EQ(order.getPickupPosition().col, 3);
    EXPECT_EQ(order.getDeliveryPosition().row, 7);
    EXPECT_EQ(order.getDeliveryPosition().col, 3);
    EXPECT_EQ(order.getStatus(), OrderStatus::Unclaimed);
}