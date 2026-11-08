#pragma once

#include <cstddef>
#include <deque>
#include <optional>
#include <unordered_map>
#include <vector>

#include "warehouse_routing/graph.h"
#include "warehouse_routing/grid.h"
#include "warehouse_routing/inventory.h"
#include "warehouse_routing/pickup_task.h"
#include "warehouse_routing/scenario.h"

namespace warehouse_routing {
    enum class PlannerType {
        Baseline,
        CBS
    };

    class Simulation {
    public:
        //start and advance the simulation
        explicit Simulation(const Scenario& scenario, PlannerType planner = PlannerType::Baseline);
        void step();
        void run(int maxTicks);
        bool isFinished() const;

        //read simulation state and results
        int getCurrentTick() const;
        const std::vector<Agent>& getAgents() const;
        const std::vector<Order>& getOrders() const;
        int getCompletedOrderCount() const;
        double getThroughput() const;

    private:
        friend class DestinationAccessTestAccess;

        //warehouse state for this run
        Grid grid_;
        Graph graph_;
        std::vector<Agent> agents_;
        std::vector<Order> orders_;
        std::vector<InventoryCell> inventory_;
        PlannerType planner_;
        int currentTick_ = 0;

        //assign orders and handle pickup and delivery
        std::unordered_map<int, PickupTask> pickUpTasks_;
        void assignOrders();
        void handlePickups();
        void handleDeliveries();

        //track route requests, paths, and progress by agent ID
        struct RouteRequest {
            int goalID;
            bool startDelivery = false;
        };
        std::unordered_map<int, RouteRequest> pendingRouteGoals_;
        std::unordered_map<int, std::vector<int>> paths_;
        std::unordered_map<int, std::size_t> pathProgress_;
        std::unordered_map<int, int> buildPlanningGoals() const;
        void planRequestedRoutes();

        //manage owners and waiting agents at shared cells
        struct DestinationAccess {
            std::optional<int> owner;
            std::deque<int> waitingAgents;
            bool clearing = false;
        };
        std::unordered_map<int, DestinationAccess> destinationAccess_;
        void updateDestinationAccess(const std::unordered_map<int, int>& taskGoals);
        bool canUseDestination(int agentID, int destinationID) const;
        void markDestinationClearing(int agentID, int destinationID);

        //plan and execute coordinated CBS routes
        std::unordered_map<int, int> buildCBSGoals(const std::unordered_map<int, int>& taskGoals) const;
        bool tryPlanCBSRoutes(const std::unordered_map<int, int>& taskGoals, int maxTick, int maxExpandedNodes);
        void executeCBSTick();

        //handle parking, movement, and conflicts for the baseline planner
        void clearDeliveryCells();
        void moveAgents();
        std::unordered_map<int, int> proposeMoves() const;
        void attemptDetours(std::unordered_map<int, int>& proposedMoves);
        void resolveConflicts(std::unordered_map<int, int>& proposedMoves);
        void executeMoves(const std::unordered_map<int, int>& proposedMoves);
    };
}
