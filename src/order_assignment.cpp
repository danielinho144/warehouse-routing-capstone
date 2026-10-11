#include "warehouse_routing/order_assignment.h"
#include "warehouse_routing/graph.h"
#include "warehouse_routing/agent.h"
#include "warehouse_routing/order.h"
#include "warehouse_routing/astar.h"
#include <vector>

namespace warehouse_routing {
    bool assignNearestOrder(const Graph& graph, Agent& agent, std::vector<Order>& orders) {
        if (agent.getStatus() != AgentStatus::Idle) {
            return false;
        }
        Position position = agent.getPosition();
        int startID = graph.toNodeId(position.row, position.col);
        std::size_t bestCost = 0;
        Order* bestOrder = nullptr;
        for (Order& order : orders) {
            if (order.getStatus() != OrderStatus::Unclaimed) {
                continue;
            }
            

            Position pickup = order.getPickupPosition();
            int goalID = graph.toNodeId(pickup.row, pickup.col);
            std::vector<int> path = aStar(graph, startID, goalID);

            if (path.empty()) {
                continue;
            }

            std::size_t cost = path.size() -1;

            if (bestOrder == nullptr || cost < bestCost || (cost == bestCost && order.getId() < bestOrder->getId())) {
                bestOrder = &order;
                bestCost = cost;
            }
        }
        if (bestOrder == nullptr) {
            return false;
        }
        agent.assignOrder(bestOrder->getId());
        bestOrder->markAssigned();
        return true;
    }
}