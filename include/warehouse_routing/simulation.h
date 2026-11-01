#pragma once
#include <vector>
#include <unordered_map>
#include "warehouse_routing/scenario.h"
#include "warehouse_routing/graph.h"
#include "warehouse_routing/grid.h"
#include <cstddef>
#include "warehouse_routing/inventory.h"
#include "warehouse_routing/pickup_task.h"

namespace warehouse_routing {
    class Simulation {
        public:

        explicit Simulation(const Scenario& scenario);
        void step();
        int getCurrentTick() const;
        const std::vector<Agent>& getAgents() const;
        const std::vector<Order>& getOrders() const;
        bool isFinished() const;
        void run(int maxTicks);
        int getCompletedOrderCount() const;
        double getThroughput() const;

        private:
        Grid grid_;
        Graph graph_;
        std::vector <Agent> agents_;
        std::vector <Order> orders_;
        std::unordered_map <int, std::vector<int>> paths_;
        std::unordered_map<int, PickupTask> pickUpTasks_;
        std::unordered_map<int, std::size_t> pathProgress_;
        int currentTick_ = 0;
        std::vector<InventoryCell> inventory_;
    };
}