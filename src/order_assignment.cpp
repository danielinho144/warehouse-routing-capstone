#include "warehouse_routing/order_assignment.h"
#include "warehouse_routing/graph.h"
#include "warehouse_routing/agent.h"
#include "warehouse_routing/order.h"
#include "warehouse_routing/astar.h"
#include <vector>

//choose an order for an idle agent

namespace warehouse_routing {

    //changes agent and selected order through their references
    bool assignNearestOrder(const Graph& graph, Agent& agent, std::vector<Order>& orders) {
        if (agent.getStatus() != AgentStatus::Idle) {
            return false; //return false if agent isn't idle
        }

        //convert position into start node
        Position position = agent.getPosition();
        int startID = graph.toNodeId(position.row, position.col);
        std::size_t bestCost = 0;
        Order* bestOrder = nullptr;

        //look through unclaimed orders
        for (Order& order : orders) {
            if (order.getStatus() != OrderStatus::Unclaimed) {
                continue;
            }
            
            //run A* from the agent to each pickup
            Position pickup = order.getPickupPosition();
            int goalID = graph.toNodeId(pickup.row, pickup.col);
            std::vector<int> path = aStar(graph, startID, goalID);

            if (path.empty()) { //skip unreachable pickups
                continue;
            }

            std::size_t cost = path.size() -1;

            //keep cheapest route
            if (bestOrder == nullptr || cost < bestCost || (cost == bestCost && order.getId() < bestOrder->getId())) {
                bestOrder = &order;
                bestCost = cost;
            } //if costs tie, choose lower ID
        }
        if (bestOrder == nullptr) {
            return false;
        }
        agent.assignOrder(bestOrder->getId());
        bestOrder->markAssigned();
        return true;
    }
}