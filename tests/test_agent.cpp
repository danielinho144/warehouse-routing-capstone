#include <gtest/gtest.h>
#include "warehouse_routing/agent.h"
#include <stdexcept>

using namespace warehouse_routing;

TEST(AgentTest, StartsIdleWithoutOrder) {
    Agent agent(7, Position{2, 3}, 10);

    EXPECT_EQ(agent.getId(), 7);
    EXPECT_EQ(agent.getPosition().row, 2);
    EXPECT_EQ(agent.getPosition().col, 3);
    EXPECT_EQ(agent.getStatus(), AgentStatus::Idle);
    EXPECT_FALSE(agent.getOrderId().has_value());
}

TEST(AgentTest, SuccessfulAgentLifeCycle) {
    Agent agent(7, Position{2, 3}, 10);
    agent.assignOrder(3);
    EXPECT_EQ(agent.getStatus(), AgentStatus::PickupPending);
    ASSERT_TRUE(agent.getOrderId().has_value());
    EXPECT_EQ(agent.getOrderId().value(), 3);

    agent.addCarriedItems("SKU_A", 2);
    agent.startDelivery();
    EXPECT_EQ(agent.getStatus(), AgentStatus::Delivering);
    ASSERT_TRUE(agent.getOrderId().has_value());
    EXPECT_EQ(agent.getOrderId().value(), 3);

    agent.removeCarriedItems("SKU_A", 2);
    agent.completeDelivery(false);
    EXPECT_EQ(agent.getStatus(), AgentStatus::Idle);
    EXPECT_FALSE(agent.getOrderId().has_value());
}

TEST(AgentTest, AgentMovement) {
    Agent agent(7, Position{2, 3}, 10);
    agent.moveTo(Position{5,6});
    EXPECT_EQ(agent.getPosition().row, 5);
    EXPECT_EQ(agent.getPosition().col, 6);
}

TEST(AgentTest, InvalidAgentActions) {
    Agent agent(7, Position{2, 3}, 10);
    EXPECT_THROW(agent.startDelivery(), std::runtime_error);
    EXPECT_THROW(agent.completeDelivery(false), std::runtime_error);

    agent.assignOrder(3);
    EXPECT_THROW(agent.completeDelivery(false), std::runtime_error);
    EXPECT_THROW(agent.assignOrder(4), std::runtime_error);

    EXPECT_EQ(agent.getStatus(), AgentStatus::PickupPending);
    ASSERT_TRUE(agent.getOrderId().has_value());
    EXPECT_EQ(agent.getOrderId().value(), 3);
}

TEST(AgentTest, StartsEmptyWithFullRemainingCapacity) {
    const Agent agent(7, Position{2, 3}, 10);
    EXPECT_EQ(agent.getCapacity(), 10);
    EXPECT_EQ(agent.getCurrentLoad(), 0);
    EXPECT_EQ(agent.getRemainingCapacity(), 10);
    EXPECT_TRUE(agent.getCarriedItems().empty());
}

TEST(AgentTest, NonpositiveCapacityIsRejected) {
    EXPECT_THROW((Agent(7, Position{2, 3}, 0)), std::invalid_argument);
    EXPECT_THROW((Agent(7, Position{2, 3}, -1)), std::invalid_argument);
    const Agent smallest(7, Position{2, 3}, 1);
    EXPECT_EQ(smallest.getCapacity(), 1);
}

TEST(AgentTest, AddingDifferentItemsUpdatesLoadAndRemainingCapacity) {
    Agent agent(7, Position{2, 3}, 10);
    agent.addCarriedItems("SKU_A", 3);
    agent.addCarriedItems("SKU_B", 2);

    ASSERT_EQ(agent.getCarriedItems().size(), 2u);
    EXPECT_EQ(agent.getCarriedItems().at(0).itemID, "SKU_A");
    EXPECT_EQ(agent.getCarriedItems().at(0).quantity, 3);
    EXPECT_EQ(agent.getCarriedItems().at(1).itemID, "SKU_B");
    EXPECT_EQ(agent.getCarriedItems().at(1).quantity, 2);
    EXPECT_EQ(agent.getCurrentLoad(), 5);
    EXPECT_EQ(agent.getRemainingCapacity(), 5);
    EXPECT_EQ(agent.getCapacity(), 10);
}

TEST(AgentTest, RepeatedItemIsMergedEvenWhenNotFirstEntry) {
    Agent agent(7, Position{2, 3}, 10);
    agent.addCarriedItems("SKU_A", 2);
    agent.addCarriedItems("SKU_B", 1);
    agent.addCarriedItems("SKU_B", 3);

    ASSERT_EQ(agent.getCarriedItems().size(), 2u);
    EXPECT_EQ(agent.getCarriedItems().at(0).quantity, 2);
    EXPECT_EQ(agent.getCarriedItems().at(1).quantity, 4);
    EXPECT_EQ(agent.getCurrentLoad(), 6);
    EXPECT_EQ(agent.getRemainingCapacity(), 4);
}

TEST(AgentTest, ExactCapacityFitIsAllowedForNewAndExistingItems) {
    for (bool sameItem : {false, true}) {
        SCOPED_TRACE(sameItem);
        Agent agent(7, Position{2, 3}, 5);
        agent.addCarriedItems("SKU_A", 2);
        agent.addCarriedItems(sameItem ? "SKU_A" : "SKU_B", 3);
        EXPECT_EQ(agent.getCurrentLoad(), 5);
        EXPECT_EQ(agent.getRemainingCapacity(), 0);
        EXPECT_EQ(agent.getCarriedItems().size(), sameItem ? 1u : 2u);
        EXPECT_THROW(agent.addCarriedItems("SKU_A", 1), std::invalid_argument);
        EXPECT_EQ(agent.getCurrentLoad(), 5);
    }
}

TEST(AgentTest, InvalidAdditionsPreserveExistingLoad) {
    Agent agent(7, Position{2, 3}, 5);
    agent.addCarriedItems("SKU_A", 2);
    EXPECT_THROW(agent.addCarriedItems("", 1), std::invalid_argument);
    EXPECT_THROW(agent.addCarriedItems("SKU_A", 0), std::invalid_argument);
    EXPECT_THROW(agent.addCarriedItems("SKU_A", -1), std::invalid_argument);
    EXPECT_THROW(agent.addCarriedItems("SKU_A", 4), std::invalid_argument);
    EXPECT_THROW(agent.addCarriedItems("SKU_B", 4), std::invalid_argument);
    ASSERT_EQ(agent.getCarriedItems().size(), 1u);
    EXPECT_EQ(agent.getCarriedItems().at(0).quantity, 2);
    EXPECT_EQ(agent.getCurrentLoad(), 2);
    EXPECT_EQ(agent.getRemainingCapacity(), 3);
}

TEST(AgentTest, PartialRemovalUpdatesOnlyMatchingItem) {
    Agent agent(7, Position{2, 3}, 10);
    agent.addCarriedItems("SKU_A", 2);
    agent.addCarriedItems("SKU_B", 5);
    agent.removeCarriedItems("SKU_B", 3);

    ASSERT_EQ(agent.getCarriedItems().size(), 2u);
    EXPECT_EQ(agent.getCarriedItems().at(0).quantity, 2);
    EXPECT_EQ(agent.getCarriedItems().at(1).quantity, 2);
    EXPECT_EQ(agent.getCurrentLoad(), 4);
    EXPECT_EQ(agent.getRemainingCapacity(), 6);
}

TEST(AgentTest, FullRemovalErasesCorrectEntryAtAnyPosition) {
    for (std::size_t index = 0; index < 3; ++index) {
        SCOPED_TRACE(index);
        Agent agent(7, Position{2, 3}, 10);
        const std::vector<std::string> ids{"SKU_A", "SKU_B", "SKU_C"};
        for (const auto& id : ids) agent.addCarriedItems(id, 2);
        agent.removeCarriedItems(ids.at(index), 2);

        ASSERT_EQ(agent.getCarriedItems().size(), 2u);
        std::size_t remainingIndex = 0;
        for (std::size_t originalIndex = 0; originalIndex < ids.size(); ++originalIndex) {
            if (originalIndex == index) continue;
            EXPECT_EQ(agent.getCarriedItems().at(remainingIndex).itemID, ids.at(originalIndex));
            EXPECT_EQ(agent.getCarriedItems().at(remainingIndex).quantity, 2);
            ++remainingIndex;
        }
        EXPECT_EQ(agent.getCurrentLoad(), 4);
        EXPECT_EQ(agent.getRemainingCapacity(), 6);
    }
}

TEST(AgentTest, RemovingLastItemRestoresEmptyLoadAndAllowsRefill) {
    Agent agent(7, Position{2, 3}, 5);
    agent.addCarriedItems("SKU_A", 5);
    agent.removeCarriedItems("SKU_A", 5);
    EXPECT_TRUE(agent.getCarriedItems().empty());
    EXPECT_EQ(agent.getCurrentLoad(), 0);
    EXPECT_EQ(agent.getRemainingCapacity(), 5);
    agent.addCarriedItems("SKU_A", 5);
    ASSERT_EQ(agent.getCarriedItems().size(), 1u);
    EXPECT_EQ(agent.getCurrentLoad(), 5);
}

TEST(AgentTest, InvalidRemovalsPreserveExistingLoad) {
    Agent agent(7, Position{2, 3}, 10);
    agent.addCarriedItems("SKU_A", 2);
    agent.addCarriedItems("SKU_B", 3);
    EXPECT_THROW(agent.removeCarriedItems("", 1), std::invalid_argument);
    EXPECT_THROW(agent.removeCarriedItems("SKU_A", 0), std::invalid_argument);
    EXPECT_THROW(agent.removeCarriedItems("SKU_A", -1), std::invalid_argument);
    EXPECT_THROW(agent.removeCarriedItems("SKU_A", 3), std::invalid_argument);
    EXPECT_THROW(agent.removeCarriedItems("UNKNOWN", 1), std::invalid_argument);
    ASSERT_EQ(agent.getCarriedItems().size(), 2u);
    EXPECT_EQ(agent.getCarriedItems().at(0).quantity, 2);
    EXPECT_EQ(agent.getCarriedItems().at(1).quantity, 3);
    EXPECT_EQ(agent.getCurrentLoad(), 5);
    EXPECT_EQ(agent.getRemainingCapacity(), 5);
}

TEST(AgentTest, CannotRemoveItemsFromEmptyAgent) {
    Agent agent(7, Position{2, 3}, 10);
    EXPECT_THROW(agent.removeCarriedItems("SKU_A", 1), std::invalid_argument);
    EXPECT_TRUE(agent.getCarriedItems().empty());
    EXPECT_EQ(agent.getCurrentLoad(), 0);
    EXPECT_EQ(agent.getRemainingCapacity(), 10);
}

TEST(AgentTest, LoadedAgentCanUnloadBeforeCompletingDelivery) {
    Agent agent(7, Position{2, 3}, 10);
    agent.assignOrder(3);
    agent.addCarriedItems("SKU_A", 3);
    agent.addCarriedItems("SKU_B", 2);
    agent.startDelivery();
    agent.removeCarriedItems("SKU_A", 3);
    agent.removeCarriedItems("SKU_B", 2);
    agent.completeDelivery(false);
    EXPECT_EQ(agent.getStatus(), AgentStatus::Idle);
    EXPECT_FALSE(agent.getOrderId().has_value());
    EXPECT_TRUE(agent.getCarriedItems().empty());
    EXPECT_EQ(agent.getRemainingCapacity(), 10);
}


TEST(AgentTest, StartDeliveryRejectsAssignedAgentWithEmptyLoad) {
    Agent agent(7, Position{2, 3}, 10);
    agent.assignOrder(3);
    EXPECT_THROW(agent.startDelivery(), std::runtime_error);
    EXPECT_EQ(agent.getStatus(), AgentStatus::PickupPending);
    EXPECT_EQ(agent.getOrderId(), 3);
    EXPECT_EQ(agent.getCurrentLoad(), 0);
}

TEST(AgentTest, StartDeliveryRejectsUnassignedAgentEvenWithLoad) {
    Agent agent(7, Position{2, 3}, 10);
    agent.addCarriedItems("SKU_A", 2);
    EXPECT_THROW(agent.startDelivery(), std::runtime_error);
    EXPECT_EQ(agent.getStatus(), AgentStatus::Idle);
    EXPECT_FALSE(agent.getOrderId().has_value());
    EXPECT_EQ(agent.getCurrentLoad(), 2);
}

TEST(AgentTest, MultiplePickupsStayPendingUntilDeliveryExplicitlyStarts) {
    Agent agent(7, Position{2, 3}, 5);
    agent.assignOrder(3);
    agent.addCarriedItems("SKU_A", 2);
    EXPECT_EQ(agent.getStatus(), AgentStatus::PickupPending);
    agent.addCarriedItems("SKU_B", 3);
    EXPECT_EQ(agent.getStatus(), AgentStatus::PickupPending);
    agent.startDelivery();
    EXPECT_EQ(agent.getStatus(), AgentStatus::Delivering);
    EXPECT_EQ(agent.getOrderId(), 3);
    ASSERT_EQ(agent.getCarriedItems().size(), 2u);
    EXPECT_EQ(agent.getCarriedItems().at(0).quantity, 2);
    EXPECT_EQ(agent.getCarriedItems().at(1).quantity, 3);
    EXPECT_EQ(agent.getCurrentLoad(), 5);
    EXPECT_EQ(agent.getRemainingCapacity(), 0);
    EXPECT_THROW(agent.startDelivery(), std::runtime_error);
    EXPECT_EQ(agent.getStatus(), AgentStatus::Delivering);
    EXPECT_EQ(agent.getCurrentLoad(), 5);
}

TEST(AgentTest, CompletionRejectsRemainingLoadForEitherNextTripChoice) {
    for (bool needsAnotherTrip : {false, true}) {
        SCOPED_TRACE(needsAnotherTrip);
        Agent agent(7, Position{2, 3}, 10);
        agent.assignOrder(3);
        agent.addCarriedItems("SKU_A", 2);
        agent.startDelivery();
        agent.removeCarriedItems("SKU_A", 1);
        EXPECT_THROW(agent.completeDelivery(needsAnotherTrip), std::runtime_error);
        EXPECT_EQ(agent.getStatus(), AgentStatus::Delivering);
        EXPECT_EQ(agent.getOrderId(), 3);
        ASSERT_EQ(agent.getCarriedItems().size(), 1u);
        EXPECT_EQ(agent.getCarriedItems().at(0).quantity, 1);
        EXPECT_EQ(agent.getRemainingCapacity(), 9);
    }
}

TEST(AgentTest, CompletionRequiresDeliveringStatusForEitherNextTripChoice) {
    for (bool needsAnotherTrip : {false, true}) {
        SCOPED_TRACE(needsAnotherTrip);
        Agent agent(7, Position{2, 3}, 10);
        EXPECT_THROW(agent.completeDelivery(needsAnotherTrip), std::runtime_error);
        EXPECT_EQ(agent.getStatus(), AgentStatus::Idle);
        EXPECT_FALSE(agent.getOrderId().has_value());
        agent.assignOrder(3);
        EXPECT_THROW(agent.completeDelivery(needsAnotherTrip), std::runtime_error);
        EXPECT_EQ(agent.getStatus(), AgentStatus::PickupPending);
        EXPECT_EQ(agent.getOrderId(), 3);
    }
}

TEST(AgentTest, AnotherTripKeepsOrderAndCanFinishOnSecondDelivery) {
    Agent agent(7, Position{2, 3}, 5);
    agent.assignOrder(3);
    agent.addCarriedItems("SKU_A", 5);
    agent.startDelivery();
    agent.removeCarriedItems("SKU_A", 5);
    agent.completeDelivery(true);
    EXPECT_EQ(agent.getStatus(), AgentStatus::PickupPending);
    EXPECT_EQ(agent.getOrderId(), 3);
    EXPECT_TRUE(agent.getCarriedItems().empty());
    EXPECT_EQ(agent.getRemainingCapacity(), 5);
    EXPECT_THROW(agent.assignOrder(4), std::runtime_error);
    EXPECT_THROW(agent.startDelivery(), std::runtime_error);

    agent.addCarriedItems("SKU_B", 2);
    agent.startDelivery();
    EXPECT_EQ(agent.getOrderId(), 3);
    agent.removeCarriedItems("SKU_B", 2);
    agent.completeDelivery(false);
    EXPECT_EQ(agent.getStatus(), AgentStatus::Idle);
    EXPECT_FALSE(agent.getOrderId().has_value());
    EXPECT_TRUE(agent.getCarriedItems().empty());
    EXPECT_EQ(agent.getRemainingCapacity(), 5);
    EXPECT_THROW(agent.completeDelivery(false), std::runtime_error);
    agent.assignOrder(4);
    EXPECT_EQ(agent.getOrderId(), 4);
}

TEST(AgentTest, ReleaseOrderMakesEmptyPickupAgentIdleAndAllowsReassignment) {
    Agent agent(7, Position{2, 3}, 5);
    agent.assignOrder(3);

    agent.releaseOrder();

    EXPECT_EQ(agent.getStatus(), AgentStatus::Idle);
    EXPECT_FALSE(agent.getOrderId().has_value());
    EXPECT_TRUE(agent.getCarriedItems().empty());
    EXPECT_EQ(agent.getRemainingCapacity(), 5);
    EXPECT_EQ(agent.getPosition().row, 2);
    EXPECT_EQ(agent.getPosition().col, 3);
    agent.assignOrder(4);
    EXPECT_EQ(agent.getOrderId(), 4);
    EXPECT_EQ(agent.getStatus(), AgentStatus::PickupPending);
}

TEST(AgentTest, ReleaseOrderRejectsIdleAgent) {
    Agent agent(7, Position{2, 3}, 5);

    EXPECT_THROW(agent.releaseOrder(), std::runtime_error);

    EXPECT_EQ(agent.getStatus(), AgentStatus::Idle);
    EXPECT_FALSE(agent.getOrderId().has_value());
    EXPECT_EQ(agent.getCurrentLoad(), 0);
}

TEST(AgentTest, ReleaseOrderRejectsPickupAgentWithCarriedItems) {
    Agent agent(7, Position{2, 3}, 5);
    agent.assignOrder(3);
    agent.addCarriedItems("SKU_A", 2);

    EXPECT_THROW(agent.releaseOrder(), std::runtime_error);

    EXPECT_EQ(agent.getStatus(), AgentStatus::PickupPending);
    EXPECT_EQ(agent.getOrderId(), 3);
    ASSERT_EQ(agent.getCarriedItems().size(), 1u);
    EXPECT_EQ(agent.getCarriedItems().at(0).quantity, 2);
    EXPECT_EQ(agent.getCurrentLoad(), 2);
}

TEST(AgentTest, ReleaseOrderRejectsDeliveringAgentEvenAfterUnloading) {
    Agent agent(7, Position{2, 3}, 5);
    agent.assignOrder(3);
    agent.addCarriedItems("SKU_A", 2);
    agent.startDelivery();

    EXPECT_THROW(agent.releaseOrder(), std::runtime_error);
    EXPECT_EQ(agent.getCurrentLoad(), 2);
    EXPECT_EQ(agent.getStatus(), AgentStatus::Delivering);
    EXPECT_EQ(agent.getOrderId(), 3);

    agent.removeCarriedItems("SKU_A", 2);
    EXPECT_THROW(agent.releaseOrder(), std::runtime_error);
    EXPECT_EQ(agent.getStatus(), AgentStatus::Delivering);
    EXPECT_EQ(agent.getOrderId(), 3);
}

TEST(AgentTest, ReleaseOrderCannotBeCalledTwice) {
    Agent agent(7, Position{2, 3}, 5);
    agent.assignOrder(3);
    agent.releaseOrder();

    EXPECT_THROW(agent.releaseOrder(), std::runtime_error);
    EXPECT_EQ(agent.getStatus(), AgentStatus::Idle);
    EXPECT_FALSE(agent.getOrderId().has_value());
}
