#include <gtest/gtest.h>
#include "warehouse_routing/order.h"
#include <stdexcept>

using namespace warehouse_routing;

TEST(OrderTest, ConstructionPreservesLinesAndDelivery) {
    const Order order(3, {{"SKU_A", 3}, {"SKU_B", 2}}, Position{7, 3});
    EXPECT_EQ(order.getId(), 3);
    EXPECT_EQ(order.getDeliveryPosition().row, 7);
    EXPECT_EQ(order.getDeliveryPosition().col, 3);
    EXPECT_EQ(order.getStatus(), OrderStatus::Unclaimed);
    ASSERT_EQ(order.getLines().size(), 2);
    EXPECT_EQ(order.getLines().at(0).itemID, "SKU_A");
    EXPECT_EQ(order.getLines().at(0).reqQuantity, 3);
    EXPECT_EQ(order.getLines().at(0).delQuantity, 0);
    EXPECT_EQ(order.getLines().at(1).itemID, "SKU_B");
    EXPECT_EQ(order.getLines().at(1).reqQuantity, 2);
    EXPECT_EQ(order.getLines().at(1).delQuantity, 0);
}

TEST(OrderTest, EmptyOrderIsRejected) {
    EXPECT_THROW((Order(3, {}, Position{7, 3})), std::invalid_argument);
}

TEST(OrderTest, EveryLineRequiresNonemptyUniqueItemID) {
    EXPECT_THROW((Order(3, {{"SKU_A", 1}, {"", 1}}, Position{7, 3})), std::invalid_argument);
    EXPECT_THROW((Order(3, {{"SKU_A", 1}, {"SKU_A", 2}}, Position{7, 3})), std::invalid_argument);
}

TEST(OrderTest, EveryLineRequiresPositiveQuantity) {
    for (int quantity : {0, -1}) {
        SCOPED_TRACE(quantity);
        EXPECT_THROW((Order(3, {{"SKU_A", 1}, {"SKU_B", quantity}}, Position{7, 3})),
                     std::invalid_argument);
    }
}

TEST(OrderTest, NewOrdersCannotContainDeliveredQuantities) {
    for (int quantity : {-1, 1}) {
        SCOPED_TRACE(quantity);
        EXPECT_THROW((Order(3, {{"SKU_A", 1}, {"SKU_B", 2, quantity}}, Position{7, 3})),
                     std::invalid_argument);
    }
}

TEST(OrderTest, MultipleDeliveriesAccumulateOnMatchingLineOnly) {
    Order order(3, {{"SKU_A", 3}, {"SKU_B", 2}}, Position{7, 3});
    order.markAssigned();
    order.recordDelivery("SKU_B", 1);
    EXPECT_EQ(order.getLines().at(0).delQuantity, 0);
    EXPECT_EQ(order.getLines().at(1).delQuantity, 1);
    order.recordDelivery("SKU_B", 1);
    order.recordDelivery("SKU_A", 2);
    EXPECT_EQ(order.getLines().at(0).delQuantity, 2);
    EXPECT_EQ(order.getLines().at(1).delQuantity, 2);
    EXPECT_EQ(order.getStatus(), OrderStatus::Assigned);
}

TEST(OrderTest, InvalidDeliveryAmountsPreserveProgress) {
    Order order(3, {{"SKU_A", 5}}, Position{7, 3});
    order.markAssigned();
    order.recordDelivery("SKU_A", 2);
    for (int quantity : {0, -1, 4}) {
        SCOPED_TRACE(quantity);
        EXPECT_THROW(order.recordDelivery("SKU_A", quantity), std::invalid_argument);
        EXPECT_EQ(order.getLines().at(0).delQuantity, 2);
        EXPECT_EQ(order.getStatus(), OrderStatus::Assigned);
    }
    order.recordDelivery("SKU_A", 3);
    EXPECT_EQ(order.getLines().at(0).delQuantity, 5);
    EXPECT_THROW(order.recordDelivery("SKU_A", 1), std::invalid_argument);
    EXPECT_EQ(order.getLines().at(0).delQuantity, 5);
}

TEST(OrderTest, UnknownItemDoesNotChangeAnyLine) {
    Order order(3, {{"SKU_A", 3}, {"SKU_B", 2}}, Position{7, 3});
    order.markAssigned();
    EXPECT_THROW(order.recordDelivery("SKU_C", 1), std::invalid_argument);
    EXPECT_EQ(order.getLines().at(0).delQuantity, 0);
    EXPECT_EQ(order.getLines().at(1).delQuantity, 0);
}

TEST(OrderTest, UnclaimedOrderRejectsDeliveryAndCompletion) {
    Order order(3, {{"SKU_A", 3}}, Position{7, 3});
    EXPECT_THROW(order.recordDelivery("SKU_A", 1), std::runtime_error);
    EXPECT_THROW(order.markCompleted(), std::runtime_error);
    EXPECT_EQ(order.getStatus(), OrderStatus::Unclaimed);
    EXPECT_EQ(order.getLines().at(0).delQuantity, 0);
}

TEST(OrderTest, UnclaimedOrderCanBeMarkedUnfinishedWithoutDeliveries) {
    Order order(3, {{"SKU_A", 3}, {"SKU_B", 2}}, Position{7, 3});
    order.markUnfinished();
    EXPECT_EQ(order.getStatus(), OrderStatus::Unfinished);
    ASSERT_EQ(order.getLines().size(), 2u);
    EXPECT_EQ(order.getLines().at(0).reqQuantity, 3);
    EXPECT_EQ(order.getLines().at(1).reqQuantity, 2);
    for (const OrderLine& line : order.getLines()) {
        EXPECT_EQ(line.delQuantity, 0);
    }
    EXPECT_THROW(order.markUnfinished(), std::runtime_error);
    EXPECT_THROW(order.markAssigned(), std::runtime_error);
    EXPECT_THROW(order.recordDelivery("SKU_A", 1), std::runtime_error);
    EXPECT_THROW(order.markCompleted(), std::runtime_error);
    EXPECT_EQ(order.getStatus(), OrderStatus::Unfinished);
}

TEST(OrderTest, AssignedOrderCannotBeAssignedAgain) {
    Order order(3, {{"SKU_A", 3}}, Position{7, 3});
    order.markAssigned();
    EXPECT_THROW(order.markAssigned(), std::runtime_error);
    EXPECT_EQ(order.getStatus(), OrderStatus::Assigned);
}

TEST(OrderTest, CompletionRequiresEveryLineToBeFullyDelivered) {
    Order order(3, {{"SKU_A", 3}, {"SKU_B", 2}}, Position{7, 3});
    order.markAssigned();
    EXPECT_THROW(order.markCompleted(), std::runtime_error);
    order.recordDelivery("SKU_A", 3);
    order.recordDelivery("SKU_B", 1);
    EXPECT_THROW(order.markCompleted(), std::runtime_error);
    EXPECT_EQ(order.getStatus(), OrderStatus::Assigned);
    EXPECT_EQ(order.getLines().at(1).delQuantity, 1);
    order.recordDelivery("SKU_B", 1);
    EXPECT_EQ(order.getStatus(), OrderStatus::Assigned);
    EXPECT_THROW(order.markUnfinished(), std::runtime_error);
    order.markCompleted();
    EXPECT_EQ(order.getStatus(), OrderStatus::Completed);
}

TEST(OrderTest, UnfinishedPreservesPartialDeliveryWithLaterLineOutstanding) {
    Order order(3, {{"SKU_A", 3}, {"SKU_B", 2}}, Position{7, 3});
    order.markAssigned();
    order.recordDelivery("SKU_A", 3);
    order.recordDelivery("SKU_B", 1);
    order.markUnfinished();
    EXPECT_EQ(order.getStatus(), OrderStatus::Unfinished);
    EXPECT_EQ(order.getLines().at(0).delQuantity, 3);
    EXPECT_EQ(order.getLines().at(1).delQuantity, 1);
}

TEST(OrderTest, OrderWithNoDeliveriesCanBeMarkedUnfinished) {
    Order order(3, {{"SKU_A", 3}}, Position{7, 3});
    order.markAssigned();
    order.markUnfinished();
    EXPECT_EQ(order.getStatus(), OrderStatus::Unfinished);
    EXPECT_EQ(order.getLines().at(0).delQuantity, 0);
}

TEST(OrderTest, CompletedOrderRejectsFurtherUpdates) {
    Order order(3, {{"SKU_A", 3}}, Position{7, 3});
    order.markAssigned();
    order.recordDelivery("SKU_A", 3);
    order.markCompleted();
    EXPECT_THROW(order.markAssigned(), std::runtime_error);
    EXPECT_THROW(order.markCompleted(), std::runtime_error);
    EXPECT_THROW(order.markUnfinished(), std::runtime_error);
    EXPECT_THROW(order.recordDelivery("SKU_A", 1), std::runtime_error);
    EXPECT_EQ(order.getStatus(), OrderStatus::Completed);
    EXPECT_EQ(order.getLines().at(0).delQuantity, 3);
}

TEST(OrderTest, UnfinishedOrderRejectsFurtherUpdates) {
    Order order(3, {{"SKU_A", 3}}, Position{7, 3});
    order.markAssigned();
    order.recordDelivery("SKU_A", 1);
    order.markUnfinished();
    EXPECT_THROW(order.markAssigned(), std::runtime_error);
    EXPECT_THROW(order.markCompleted(), std::runtime_error);
    EXPECT_THROW(order.markUnfinished(), std::runtime_error);
    EXPECT_THROW(order.recordDelivery("SKU_A", 1), std::runtime_error);
    EXPECT_EQ(order.getStatus(), OrderStatus::Unfinished);
    EXPECT_EQ(order.getLines().at(0).delQuantity, 1);
}
