#include "warehouse_routing/order_assignment.h"
#include "warehouse_routing/graph.h"
#include "warehouse_routing/agent.h"
#include "warehouse_routing/order.h"
#include "warehouse_routing/astar.h"
#include <vector>

//choose an order for an idle agent

namespace warehouse_routing {

    //changes agent and selected order through their references
    bool assignNearestOrder(const Graph& graph, Agent& agent, std::vector<Order>& orders, const std::vector<InventoryCell>& inventory) {
        if (agent.getStatus() != AgentStatus::Idle) {
            return false; //return false if agent isn't idle
        }

        //convert position into start node
        Position position = agent.getPosition();
        int startID = graph.toNodeId(position.row, position.col);
        Order* bestOrder = nullptr;
        std::size_t bestCost = 0;

        //look through unclaimed orders
        for (Order& order : orders) {
            if (order.getStatus() != OrderStatus::Unclaimed) {
                continue;
            }
            
            //run A* from the agent to each pickup

            bool reachablePickup = false;
            std::size_t shortestPickupCost = 0;

            for (const OrderLine& line : order.getLines()) {
                if (line.delQuantity == line.reqQuantity) {
                    continue;
                }
                for (const InventoryCell& cell : inventory) {
                    if (cell.getItemID() != line.itemID) {
                        continue;
                    }
                    if (cell.getQuantity() == 0) {
                        continue;
                    }
                    Position pickup = cell.getPickupPosition();
                    int goalID = graph.toNodeId(pickup.row, pickup.col);
                    std::vector<int> path = aStar(graph, startID, goalID);

                    if (path.empty()) {
                        continue;
                    }
                    std::size_t cost = path.size() -1;
                    if (!reachablePickup || cost < shortestPickupCost) {
                        shortestPickupCost = cost;
                        reachablePickup = true;
                    }
                }
            }
            if (reachablePickup == false) {
                continue;
            }
            //keep cheapest order; ties choose lower order ID
            if (bestOrder == nullptr || shortestPickupCost < bestCost || (shortestPickupCost == bestCost && order.getId() < bestOrder->getId())) {
                bestOrder = &order;
                bestCost = shortestPickupCost;
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