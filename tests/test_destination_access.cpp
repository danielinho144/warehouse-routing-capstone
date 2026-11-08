#include <gtest/gtest.h>
#include <stdexcept>
#include "warehouse_routing/simulation.h"

namespace warehouse_routing {
    // Keep allocator access in this helper instead of making it public.
    class DestinationAccessTestAccess {
        public:
        static bool canUse(const Simulation& sim, int agentID, int destination) {
            return sim.canUseDestination(agentID, destination);
        }

        static void update(Simulation& sim, const std::unordered_map<int, int>& goals) {
            sim.updateDestinationAccess(goals);
        }

        static std::unordered_map<int, int> buildCBSGoals(const Simulation& sim, const std::unordered_map<int, int>& taskGoals) {
            return sim.buildCBSGoals(taskGoals);
        }

        static bool tryPlanCBSRoutes(Simulation& sim, const std::unordered_map<int, int>& goals, int maxTick) {
            return sim.tryPlanCBSRoutes(goals, maxTick, 10);
        }

        static void executeCBSTick(Simulation& sim) {
            sim.executeCBSTick();
        }

        static void setRoute(Simulation& sim, int agentID, const std::vector<int>& path, std::size_t progress) {
            sim.paths_[agentID] = path;
            sim.pathProgress_[agentID] = progress;
        }

        static const std::unordered_map<int, std::vector<int>>& paths(const Simulation& sim) {
            return sim.paths_;
        }

        static const std::unordered_map<int, std::size_t>& progress(const Simulation& sim) {
            return sim.pathProgress_;
        }

        static int node(const Simulation& sim, Position position) {
            return sim.graph_.toNodeId(position.row, position.col);
        }

        static std::optional<int> owner(const Simulation& sim, int destination) {
            return sim.destinationAccess_.at(destination).owner;
        }

        static std::deque<int> waiting(const Simulation& sim, int destination) {
            return sim.destinationAccess_.at(destination).waitingAgents;
        }

        static bool isClearing(const Simulation& sim, int destination) {
            return sim.destinationAccess_.at(destination).clearing;
        }

        static void finishService(Simulation& sim, int agentID, int destination) {
            sim.markDestinationClearing(agentID, destination);
        }

        static void moveAgent(Simulation& sim, std::size_t index, Position position) {
            sim.agents_.at(index).moveTo(position);
        }

        static void releaseOrder(Simulation& sim, std::size_t index) {
            sim.agents_.at(index).releaseOrder();
        }
    };
}

using namespace warehouse_routing;

// Reverse ID order checks that selection does not depend on vector order.
static Scenario makeAccessScenario() {
    Scenario reference;
    reference.mapPath = MAP_TEST_DATA_DIR "/fulfillment.map";
    reference.agents = {
        Agent(3, Position{0, 0}, 5),
        Agent(2, Position{0, 1}, 5),
        Agent(1, Position{0, 2}, 5)
    };
    reference.orders = {
        Order(30, {{"SKU_A", 1}}, Position{2, 3}),
        Order(20, {{"SKU_A", 1}}, Position{2, 3}),
        Order(10, {{"SKU_A", 1}}, Position{2, 3})
    };
    reference.inventory = {
        InventoryCell("SKU_A", 3, Position{1, 2}, Position{0, 2})
    };
    for (std::size_t i = 0; i < reference.agents.size(); i++) {
        reference.agents.at(i).assignOrder(reference.orders.at(i).getId());
        reference.orders.at(i).markAssigned();
    }
    return reference;
}

TEST(DestinationAccessTest, SimultaneousRequestsUseAgentIDOrder) {
    Scenario reference = makeAccessScenario();
    Simulation sim(reference);
    int destination = DestinationAccessTestAccess::node(sim, Position{2, 3});

    DestinationAccessTestAccess::update(sim, {{3, destination}, {2, destination}, {1, destination}});

    ASSERT_TRUE(DestinationAccessTestAccess::owner(sim, destination).has_value());
    EXPECT_EQ(DestinationAccessTestAccess::owner(sim, destination).value(), 1);
    EXPECT_EQ(DestinationAccessTestAccess::waiting(sim, destination), (std::deque<int>{2, 3}));
}

TEST(DestinationAccessTest, CBSWaiterUsesCurrentNodeAndTaskGoalsStayUnchanged) {
    Scenario reference = makeAccessScenario();
    Simulation sim(reference);
    int destination = DestinationAccessTestAccess::node(sim, Position{2, 3});
    int otherDestination = DestinationAccessTestAccess::node(sim, Position{3, 4});
    int waiterNode = DestinationAccessTestAccess::node(sim, sim.getAgents().at(1).getPosition());
    std::unordered_map<int, int> taskGoals = {{1, destination}, {2, destination}, {3, otherDestination}};
    const auto originalGoals = taskGoals;

    DestinationAccessTestAccess::update(sim, taskGoals);
    ASSERT_EQ(DestinationAccessTestAccess::owner(sim, destination), std::optional<int>{1});
    ASSERT_EQ(DestinationAccessTestAccess::waiting(sim, destination), (std::deque<int>{2}));
    ASSERT_NE(waiterNode, destination);

    auto cbsGoals = DestinationAccessTestAccess::buildCBSGoals(sim, taskGoals);

    EXPECT_EQ(cbsGoals.at(1), destination);
    EXPECT_EQ(cbsGoals.at(2), waiterNode);
    EXPECT_EQ(cbsGoals.at(3), otherDestination);
    EXPECT_EQ(taskGoals, originalGoals);
}

TEST(DestinationAccessTest, RepeatedUpdatesKeepOwnerAndDoNotDuplicateWaiters) {
    Scenario reference = makeAccessScenario();
    Simulation sim(reference);
    int destination = DestinationAccessTestAccess::node(sim, Position{2, 3});
    std::unordered_map<int, int> goals = {{3, destination}, {2, destination}, {1, destination}};

    DestinationAccessTestAccess::update(sim, goals);
    DestinationAccessTestAccess::update(sim, goals);
    DestinationAccessTestAccess::update(sim, goals);

    ASSERT_TRUE(DestinationAccessTestAccess::owner(sim, destination).has_value());
    EXPECT_EQ(DestinationAccessTestAccess::owner(sim, destination).value(), 1);
    EXPECT_FALSE(DestinationAccessTestAccess::isClearing(sim, destination));
    EXPECT_EQ(DestinationAccessTestAccess::waiting(sim, destination), (std::deque<int>{2, 3}));
}

TEST(DestinationAccessTest, EarlierWaiterStaysAheadOfLaterLowerID) {
    Scenario reference = makeAccessScenario();
    Simulation sim(reference);
    int destination = DestinationAccessTestAccess::node(sim, Position{2, 3});
    int other = DestinationAccessTestAccess::node(sim, Position{3, 4});

    DestinationAccessTestAccess::update(sim, {{3, destination}, {2, other}, {1, other}});
    DestinationAccessTestAccess::update(sim, {{3, destination}, {2, destination}, {1, other}});
    DestinationAccessTestAccess::update(sim, {{3, destination}, {2, destination}, {1, destination}});

    ASSERT_TRUE(DestinationAccessTestAccess::owner(sim, destination).has_value());
    EXPECT_EQ(DestinationAccessTestAccess::owner(sim, destination).value(), 3);
    EXPECT_EQ(DestinationAccessTestAccess::waiting(sim, destination), (std::deque<int>{2, 1}));
}

TEST(DestinationAccessTest, CancelledWaiterIsRemoved) {
    Scenario reference = makeAccessScenario();
    Simulation sim(reference);
    int destination = DestinationAccessTestAccess::node(sim, Position{2, 3});
    std::unordered_map<int, int> goals = {{3, destination}, {2, destination}, {1, destination}};

    DestinationAccessTestAccess::update(sim, goals);
    DestinationAccessTestAccess::releaseOrder(sim, 1);
    DestinationAccessTestAccess::update(sim, goals);

    EXPECT_EQ(DestinationAccessTestAccess::owner(sim, destination), std::optional<int>{1});
    EXPECT_EQ(DestinationAccessTestAccess::waiting(sim, destination), (std::deque<int>{3}));
}

TEST(DestinationAccessTest, ChangedGoalRemovesWaiterFromOldQueue) {
    Scenario reference = makeAccessScenario();
    Simulation sim(reference);
    int destination = DestinationAccessTestAccess::node(sim, Position{2, 3});
    int other = DestinationAccessTestAccess::node(sim, Position{3, 4});

    DestinationAccessTestAccess::update(sim, {{3, destination}, {2, destination}, {1, destination}});
    DestinationAccessTestAccess::update(sim, {{3, destination}, {2, other}, {1, destination}});

    EXPECT_EQ(DestinationAccessTestAccess::waiting(sim, destination), (std::deque<int>{3}));
    EXPECT_EQ(DestinationAccessTestAccess::owner(sim, other), std::optional<int>{2});
}

TEST(DestinationAccessTest, ClearingOwnerKeepsAccessUntilLeaving) {
    Scenario reference = makeAccessScenario();
    Simulation sim(reference);
    int destination = DestinationAccessTestAccess::node(sim, Position{2, 3});
    std::unordered_map<int, int> goals = {{3, destination}, {2, destination}, {1, destination}};

    DestinationAccessTestAccess::update(sim, goals);
    DestinationAccessTestAccess::moveAgent(sim, 2, Position{2, 3});
    DestinationAccessTestAccess::finishService(sim, 1, destination);
    DestinationAccessTestAccess::update(sim, goals);

    EXPECT_EQ(DestinationAccessTestAccess::owner(sim, destination), std::optional<int>{1});
    EXPECT_TRUE(DestinationAccessTestAccess::isClearing(sim, destination));

    DestinationAccessTestAccess::moveAgent(sim, 2, Position{2, 4});
    DestinationAccessTestAccess::update(sim, goals);

    EXPECT_EQ(DestinationAccessTestAccess::owner(sim, destination), std::optional<int>{2});
    EXPECT_FALSE(DestinationAccessTestAccess::isClearing(sim, destination));
    EXPECT_EQ(DestinationAccessTestAccess::waiting(sim, destination), (std::deque<int>{3, 1}));
}

TEST(DestinationAccessTest, CancelledOwnerOnCellMustClearBeforeTransfer) {
    Scenario reference = makeAccessScenario();
    Simulation sim(reference);
    int destination = DestinationAccessTestAccess::node(sim, Position{2, 3});
    std::unordered_map<int, int> goals = {{3, destination}, {2, destination}, {1, destination}};

    DestinationAccessTestAccess::update(sim, goals);
    DestinationAccessTestAccess::moveAgent(sim, 2, Position{2, 3});
    DestinationAccessTestAccess::releaseOrder(sim, 2);
    DestinationAccessTestAccess::update(sim, goals);

    EXPECT_EQ(DestinationAccessTestAccess::owner(sim, destination), std::optional<int>{1});
    EXPECT_TRUE(DestinationAccessTestAccess::isClearing(sim, destination));

    DestinationAccessTestAccess::moveAgent(sim, 2, Position{2, 4});
    DestinationAccessTestAccess::update(sim, goals);

    EXPECT_EQ(DestinationAccessTestAccess::owner(sim, destination), std::optional<int>{2});
    EXPECT_EQ(DestinationAccessTestAccess::waiting(sim, destination), (std::deque<int>{3}));
}

TEST(DestinationAccessTest, CancelledApproachingOwnerReleasesImmediately) {
    Scenario reference = makeAccessScenario();
    Simulation sim(reference);
    int destination = DestinationAccessTestAccess::node(sim, Position{2, 3});
    std::unordered_map<int, int> goals = {{3, destination}, {2, destination}, {1, destination}};

    DestinationAccessTestAccess::update(sim, goals);
    DestinationAccessTestAccess::releaseOrder(sim, 2);
    DestinationAccessTestAccess::update(sim, goals);

    EXPECT_EQ(DestinationAccessTestAccess::owner(sim, destination), std::optional<int>{2});
    EXPECT_FALSE(DestinationAccessTestAccess::isClearing(sim, destination));
    EXPECT_EQ(DestinationAccessTestAccess::waiting(sim, destination), (std::deque<int>{3}));
}

TEST(DestinationAccessTest, UnrelatedOccupantBlocksGrantUntilItLeaves) {
    Scenario reference = makeAccessScenario();
    reference.agents.at(0).releaseOrder();
    reference.agents.at(0).moveTo(Position{2, 3});
    Simulation sim(reference);
    int destination = DestinationAccessTestAccess::node(sim, Position{2, 3});
    std::unordered_map<int, int> goals = {{2, destination}, {1, destination}};

    DestinationAccessTestAccess::update(sim, goals);

    EXPECT_FALSE(DestinationAccessTestAccess::owner(sim, destination).has_value());
    EXPECT_EQ(DestinationAccessTestAccess::waiting(sim, destination), (std::deque<int>{1, 2}));

    DestinationAccessTestAccess::moveAgent(sim, 0, Position{2, 4});
    DestinationAccessTestAccess::update(sim, goals);

    EXPECT_EQ(DestinationAccessTestAccess::owner(sim, destination), std::optional<int>{1});
    EXPECT_EQ(DestinationAccessTestAccess::waiting(sim, destination), (std::deque<int>{2}));
}

TEST(DestinationAccessTest, RequestingOccupantGetsAccessBeforeQueuedAgents) {
    Scenario reference = makeAccessScenario();
    reference.agents.at(0).moveTo(Position{2, 3});
    Simulation sim(reference);
    int destination = DestinationAccessTestAccess::node(sim, Position{2, 3});

    DestinationAccessTestAccess::update(sim, {{3, destination}, {2, destination}, {1, destination}});

    EXPECT_EQ(DestinationAccessTestAccess::owner(sim, destination), std::optional<int>{3});
    EXPECT_EQ(DestinationAccessTestAccess::waiting(sim, destination), (std::deque<int>{1, 2}));
}

TEST(DestinationAccessTest, DifferentDestinationsHaveIndependentOwners) {
    Scenario reference = makeAccessScenario();
    Simulation sim(reference);
    int destination = DestinationAccessTestAccess::node(sim, Position{2, 3});
    int other = DestinationAccessTestAccess::node(sim, Position{3, 4});

    DestinationAccessTestAccess::update(sim, {{3, other}, {2, other}, {1, destination}});

    EXPECT_EQ(DestinationAccessTestAccess::owner(sim, destination), std::optional<int>{1});
    EXPECT_TRUE(DestinationAccessTestAccess::waiting(sim, destination).empty());
    EXPECT_EQ(DestinationAccessTestAccess::owner(sim, other), std::optional<int>{2});
    EXPECT_EQ(DestinationAccessTestAccess::waiting(sim, other), (std::deque<int>{3}));
}

TEST(DestinationAccessTest, AllCancelledRequestsLeaveNoOwnerOrWaiters) {
    Scenario reference = makeAccessScenario();
    Simulation sim(reference);
    int destination = DestinationAccessTestAccess::node(sim, Position{2, 3});

    DestinationAccessTestAccess::update(sim, {{3, destination}, {2, destination}, {1, destination}});
    DestinationAccessTestAccess::releaseOrder(sim, 0);
    DestinationAccessTestAccess::releaseOrder(sim, 1);
    DestinationAccessTestAccess::releaseOrder(sim, 2);
    DestinationAccessTestAccess::update(sim, {});

    EXPECT_FALSE(DestinationAccessTestAccess::owner(sim, destination).has_value());
    EXPECT_TRUE(DestinationAccessTestAccess::waiting(sim, destination).empty());
    EXPECT_FALSE(DestinationAccessTestAccess::isClearing(sim, destination));
}

TEST(DestinationAccessTest, UnknownDestinationDeniesPermission) {
    Scenario reference = makeAccessScenario();
    Simulation sim(reference);
    int destination = DestinationAccessTestAccess::node(sim, Position{2, 3});

    EXPECT_FALSE(DestinationAccessTestAccess::canUse(sim, 1, destination));
}

TEST(DestinationAccessTest, OwnerAllowedAndWaitersDenied) {
    Scenario reference = makeAccessScenario();
    Simulation sim(reference);
    int destination = DestinationAccessTestAccess::node(sim, Position{2, 3});

    DestinationAccessTestAccess::update(sim, {{3, destination}, {2, destination}, {1, destination}});

    ASSERT_TRUE(DestinationAccessTestAccess::owner(sim, destination).has_value());
    ASSERT_EQ(DestinationAccessTestAccess::owner(sim, destination).value(), 1);
    EXPECT_TRUE(DestinationAccessTestAccess::canUse(sim, 1, destination));
    EXPECT_FALSE(DestinationAccessTestAccess::canUse(sim, 2, destination));
    EXPECT_FALSE(DestinationAccessTestAccess::canUse(sim, 3, destination));
}

TEST(DestinationAccessTest, ClearingOwnerDeniedPermission) {
    Scenario reference = makeAccessScenario();
    Simulation sim(reference);
    int destination = DestinationAccessTestAccess::node(sim, Position{2, 3});

    DestinationAccessTestAccess::update(sim, {{3, destination}, {2, destination}, {1, destination}});
    DestinationAccessTestAccess::moveAgent(sim, 2, Position{2, 3});
    ASSERT_TRUE(DestinationAccessTestAccess::canUse(sim, 1, destination));

    DestinationAccessTestAccess::finishService(sim, 1, destination);

    EXPECT_EQ(DestinationAccessTestAccess::owner(sim, destination), std::optional<int>{1});
    EXPECT_TRUE(DestinationAccessTestAccess::isClearing(sim, destination));
    EXPECT_FALSE(DestinationAccessTestAccess::canUse(sim, 1, destination));
}

TEST(DestinationAccessTest, DestinationWithoutOwnerDeniesPermission) {
    Scenario reference = makeAccessScenario();
    reference.agents.at(0).releaseOrder();
    reference.agents.at(0).moveTo(Position{2, 3});
    Simulation sim(reference);
    int destination = DestinationAccessTestAccess::node(sim, Position{2, 3});

    DestinationAccessTestAccess::update(sim, {{2, destination}, {1, destination}});

    ASSERT_FALSE(DestinationAccessTestAccess::owner(sim, destination).has_value());
    EXPECT_FALSE(DestinationAccessTestAccess::canUse(sim, 1, destination));
    EXPECT_FALSE(DestinationAccessTestAccess::canUse(sim, 2, destination));
    EXPECT_FALSE(DestinationAccessTestAccess::canUse(sim, 3, destination));
}

TEST(DestinationAccessTest, ClearingUnknownDestinationThrows) {
    Scenario reference = makeAccessScenario();
    Simulation sim(reference);
    int destination = DestinationAccessTestAccess::node(sim, Position{2, 3});

    EXPECT_THROW(DestinationAccessTestAccess::finishService(sim, 1, destination), std::logic_error);
    EXPECT_FALSE(DestinationAccessTestAccess::canUse(sim, 1, destination));
}

TEST(DestinationAccessTest, ClearingDestinationWithoutOwnerThrows) {
    Scenario reference = makeAccessScenario();
    reference.agents.at(0).releaseOrder();
    reference.agents.at(0).moveTo(Position{2, 3});
    Simulation sim(reference);
    int destination = DestinationAccessTestAccess::node(sim, Position{2, 3});

    DestinationAccessTestAccess::update(sim, {{2, destination}, {1, destination}});
    ASSERT_FALSE(DestinationAccessTestAccess::owner(sim, destination).has_value());

    EXPECT_THROW(DestinationAccessTestAccess::finishService(sim, 1, destination), std::logic_error);
    EXPECT_FALSE(DestinationAccessTestAccess::owner(sim, destination).has_value());
    EXPECT_FALSE(DestinationAccessTestAccess::isClearing(sim, destination));
    EXPECT_EQ(DestinationAccessTestAccess::waiting(sim, destination), (std::deque<int>{1, 2}));
}

TEST(DestinationAccessTest, ClearingByWrongAgentThrows) {
    Scenario reference = makeAccessScenario();
    Simulation sim(reference);
    int destination = DestinationAccessTestAccess::node(sim, Position{2, 3});

    DestinationAccessTestAccess::update(sim, {{3, destination}, {2, destination}, {1, destination}});
    ASSERT_EQ(DestinationAccessTestAccess::owner(sim, destination), std::optional<int>{1});

    EXPECT_THROW(DestinationAccessTestAccess::finishService(sim, 2, destination), std::logic_error);
    EXPECT_EQ(DestinationAccessTestAccess::owner(sim, destination), std::optional<int>{1});
    EXPECT_FALSE(DestinationAccessTestAccess::isClearing(sim, destination));
    EXPECT_TRUE(DestinationAccessTestAccess::canUse(sim, 1, destination));
    EXPECT_EQ(DestinationAccessTestAccess::waiting(sim, destination), (std::deque<int>{2, 3}));
}

TEST(DestinationAccessTest, WaitingAgentLeavesPickupOpenForOwner) {
    Scenario reference;
    reference.mapPath = MAP_TEST_DATA_DIR "/fulfillment.map";
    Position pickup{0, 2};
    reference.agents = {
        Agent(1, Position{0, 0}, 5),
        Agent(2, Position{0, 3}, 5)
    };
    reference.orders = {
        Order(10, {{"SKU_A", 1}}, Position{2, 3}),
        Order(20, {{"SKU_A", 1}}, Position{2, 4})
    };
    reference.inventory = {InventoryCell("SKU_A", 2, Position{1, 2}, pickup)};
    Simulation sim(reference);
    int destination = DestinationAccessTestAccess::node(sim, pickup);

    sim.step();

    EXPECT_EQ(DestinationAccessTestAccess::owner(sim, destination), std::optional<int>{1});
    EXPECT_EQ(DestinationAccessTestAccess::waiting(sim, destination), (std::deque<int>{2}));
    const Agent& waiter = sim.getAgents().at(1);
    ASSERT_EQ(waiter.getId(), 2);
    EXPECT_NE(DestinationAccessTestAccess::node(sim, waiter.getPosition()), destination);
}

TEST(DestinationAccessTest, WaitingAgentPicksUpAfterOwnerLeaves) {
    Scenario reference;
    reference.mapPath = MAP_TEST_DATA_DIR "/fulfillment.map";
    Position pickup{0, 2};
    reference.agents = {
        Agent(1, Position{0, 0}, 5),
        Agent(2, Position{0, 3}, 5)
    };
    reference.orders = {
        Order(10, {{"SKU_A", 1}}, Position{2, 3}),
        Order(20, {{"SKU_A", 1}}, Position{2, 4})
    };
    reference.inventory = {InventoryCell("SKU_A", 2, Position{1, 2}, pickup)};
    Simulation sim(reference);
    int destination = DestinationAccessTestAccess::node(sim, pickup);

    sim.step();

    ASSERT_EQ(DestinationAccessTestAccess::owner(sim, destination), std::optional<int>{1});
    ASSERT_EQ(DestinationAccessTestAccess::waiting(sim, destination), (std::deque<int>{2}));

    bool ownerPickedUp = false;
    bool waiterPickedUp = false;
    for (int tick = 0; tick < 10; tick++) {
        sim.step();

        const Agent& owner = sim.getAgents().at(0);
        const Agent& waiter = sim.getAgents().at(1);
        int ownerNode = DestinationAccessTestAccess::node(sim, owner.getPosition());
        int waiterNode = DestinationAccessTestAccess::node(sim, waiter.getPosition());

        if (ownerNode == destination && owner.getStatus() == AgentStatus::Delivering) {
            EXPECT_EQ(owner.getCurrentLoad(), 1);
            ownerPickedUp = true;
        }
        if (waiterNode == destination) {
            EXPECT_TRUE(ownerPickedUp);
            EXPECT_NE(ownerNode, destination);
            if (waiter.getStatus() == AgentStatus::Delivering) {
                EXPECT_EQ(waiter.getCurrentLoad(), 1);
                waiterPickedUp = true;
                break;
            }
        }
    }
    EXPECT_TRUE(waiterPickedUp);
}

TEST(DestinationAccessTest, WaitingAgentDeliversAfterOwnerClears) {
    Scenario reference;
    reference.mapPath = MAP_TEST_DATA_DIR "/fulfillment.map";
    Position delivery{2, 3};
    reference.agents = {
        Agent(1, Position{0, 2}, 5),
        Agent(2, Position{0, 5}, 5)
    };
    reference.orders = {
        Order(10, {{"SKU_A", 1}}, delivery),
        Order(20, {{"SKU_B", 1}}, delivery)
    };
    reference.inventory = {
        InventoryCell("SKU_A", 1, Position{1, 2}, Position{0, 2}),
        InventoryCell("SKU_B", 1, Position{1, 5}, Position{0, 5})
    };
    Simulation sim(reference);
    int destination = DestinationAccessTestAccess::node(sim, delivery);

    sim.step();

    for (const Agent& agent : sim.getAgents()) {
        ASSERT_EQ(agent.getStatus(), AgentStatus::Delivering);
        ASSERT_EQ(agent.getCurrentLoad(), 1);
    }
    ASSERT_EQ(DestinationAccessTestAccess::owner(sim, destination), std::optional<int>{1});
    ASSERT_EQ(DestinationAccessTestAccess::waiting(sim, destination), (std::deque<int>{2}));

    bool ownerDelivered = false;
    for (int tick = 0; tick < 20; tick++) {
        sim.step();

        const Agent& owner = sim.getAgents().at(0);
        const Agent& waiter = sim.getAgents().at(1);
        int ownerNode = DestinationAccessTestAccess::node(sim, owner.getPosition());
        int waiterNode = DestinationAccessTestAccess::node(sim, waiter.getPosition());

        if (ownerNode == destination && sim.getOrders().at(0).getStatus() == OrderStatus::Completed) {
            ownerDelivered = true;
        }
        if (waiterNode == destination) {
            EXPECT_TRUE(ownerDelivered);
            EXPECT_NE(ownerNode, destination);
        }
        if (sim.getOrders().at(1).getStatus() == OrderStatus::Completed) {
            EXPECT_EQ(waiterNode, destination);
            break;
        }
    }
    EXPECT_TRUE(ownerDelivered);
    EXPECT_EQ(sim.getOrders().at(0).getStatus(), OrderStatus::Completed);
    EXPECT_EQ(sim.getOrders().at(1).getStatus(), OrderStatus::Completed);
    EXPECT_EQ(sim.getAgents().at(0).getCurrentLoad(), 0);
    EXPECT_EQ(sim.getAgents().at(1).getCurrentLoad(), 0);
}

TEST(DestinationAccessTest, IdleAgentClearsRequestedPickup) {
    Scenario reference;
    reference.mapPath = MAP_TEST_DATA_DIR "/fulfillment.map";
    Position pickup{0, 2};
    reference.agents = {
        Agent(1, Position{0, 0}, 5),
        Agent(2, pickup, 5)
    };
    reference.orders = {Order(10, {{"SKU_A", 1}}, Position{4, 6})};
    reference.inventory = {InventoryCell("SKU_A", 1, Position{1, 2}, pickup)};
    Simulation sim(reference);
    int destination = DestinationAccessTestAccess::node(sim, pickup);

    sim.step();

    ASSERT_EQ(sim.getAgents().at(0).getOrderId(), std::optional<int>{10});
    ASSERT_EQ(sim.getAgents().at(1).getStatus(), AgentStatus::Idle);
    ASSERT_FALSE(sim.getAgents().at(1).getOrderId().has_value());

    for (int tick = 0; tick < 20; tick++) {
        sim.step();

        const Agent& requester = sim.getAgents().at(0);
        const Agent& idleAgent = sim.getAgents().at(1);
        int requesterNode = DestinationAccessTestAccess::node(sim, requester.getPosition());
        int idleNode = DestinationAccessTestAccess::node(sim, idleAgent.getPosition());

        if (requesterNode == destination) {
            EXPECT_NE(idleNode, destination);
        }
        if (requester.getCurrentLoad() == 1) {
            EXPECT_EQ(requesterNode, destination);
            EXPECT_NE(idleNode, destination);
            break;
        }
    }
    EXPECT_EQ(sim.getAgents().at(0).getCurrentLoad(), 1);
}

TEST(DestinationAccessTest, CBSRouteSuccessReplacesPathsAndResetsProgress) {
    Scenario reference;
    reference.mapPath = MAP_TEST_DATA_DIR "/fulfillment.map";
    reference.agents = {Agent(3, Position{0, 1}, 5), Agent(7, Position{0, 4}, 5)};
    Simulation sim(reference);
    DestinationAccessTestAccess::setRoute(sim, 3, {0, 1, 2}, 1);
    DestinationAccessTestAccess::setRoute(sim, 7, {6, 5, 4, 3}, 2);
    std::unordered_map<int, int> goals = {{3, 8}, {7, 11}};

    ASSERT_TRUE(DestinationAccessTestAccess::tryPlanCBSRoutes(sim, goals, 1));

    const auto& paths = DestinationAccessTestAccess::paths(sim);
    const auto& progress = DestinationAccessTestAccess::progress(sim);
    EXPECT_EQ(paths.at(3), (std::vector<int>{1, 8}));
    EXPECT_EQ(paths.at(7), (std::vector<int>{4, 11}));
    EXPECT_EQ(progress.at(3), 0);
    EXPECT_EQ(progress.at(7), 0);
}

TEST(DestinationAccessTest, CBSRouteFailureKeepsPathsAndProgress) {
    Scenario reference;
    reference.mapPath = MAP_TEST_DATA_DIR "/fulfillment.map";
    reference.agents = {Agent(3, Position{0, 1}, 5), Agent(7, Position{0, 4}, 5)};
    Simulation sim(reference);
    DestinationAccessTestAccess::setRoute(sim, 3, {0, 1, 2}, 1);
    DestinationAccessTestAccess::setRoute(sim, 7, {6, 5, 4, 3}, 2);
    const auto originalPaths = DestinationAccessTestAccess::paths(sim);
    const auto originalProgress = DestinationAccessTestAccess::progress(sim);
    std::unordered_map<int, int> goals = {{3, 1}, {7, 11}};

    EXPECT_FALSE(DestinationAccessTestAccess::tryPlanCBSRoutes(sim, goals, 0));

    EXPECT_EQ(DestinationAccessTestAccess::paths(sim), originalPaths);
    EXPECT_EQ(DestinationAccessTestAccess::progress(sim), originalProgress);
}

TEST(DestinationAccessTest, CBSPlannedWaitAdvancesProgress) {
    Scenario reference;
    reference.mapPath = MAP_TEST_DATA_DIR "/fulfillment.map";
    reference.agents = {Agent(7, Position{0, 4}, 5)};
    Simulation sim(reference);
    DestinationAccessTestAccess::setRoute(sim, 7, {4, 4, 5}, 0);

    DestinationAccessTestAccess::executeCBSTick(sim);

    EXPECT_EQ(sim.getAgents().at(0).getPosition().row, 0);
    EXPECT_EQ(sim.getAgents().at(0).getPosition().col, 4);
    EXPECT_EQ(DestinationAccessTestAccess::progress(sim).at(7), 1);

    DestinationAccessTestAccess::executeCBSTick(sim);

    EXPECT_EQ(sim.getAgents().at(0).getPosition().row, 0);
    EXPECT_EQ(sim.getAgents().at(0).getPosition().col, 5);
    EXPECT_EQ(DestinationAccessTestAccess::progress(sim).at(7), 2);

    DestinationAccessTestAccess::executeCBSTick(sim);

    EXPECT_EQ(sim.getAgents().at(0).getPosition().row, 0);
    EXPECT_EQ(sim.getAgents().at(0).getPosition().col, 5);
    EXPECT_EQ(DestinationAccessTestAccess::progress(sim).at(7), 2);
}
