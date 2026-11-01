#include "warehouse_routing/pickup_task.h"
#include <stdexcept>
#include "warehouse_routing/astar.h"

namespace warehouse_routing {
    std::optional<PickupTask> findNextPickup(const Graph& graph,
        const Agent& agent, const Order& order, 
        const std::vector<InventoryCell>& inventory) {
            if (agent.getStatus() != AgentStatus::PickupPending) {
                throw std::invalid_argument("Agent needs to be pending pickup");
            }
            if (agent.getOrderId() != order.getId()) {
                throw std::invalid_argument("Order ID mismatch");
            }
            if (order.getStatus() != OrderStatus::Assigned) {
                throw std::invalid_argument("Order is not in assigned status");
            }
            if (agent.getRemainingCapacity() == 0) {
                return std::nullopt;
            }

            std::optional<PickupTask> bestTask;
            std::size_t bestCost = 0;
            std::vector<int> path;
            Position agentPosition = agent.getPosition();
            int agentStartID = graph.toNodeId(agentPosition.row, agentPosition.col);

            for (const OrderLine& lines : order.getLines()) {
                int carriedQuantity = 0;
                for (const CarriedItem& item : agent.getCarriedItems()) {
                    if (item.itemID == lines.itemID) {
                        carriedQuantity += item.quantity;
                    }
                }
                int stillNeeded = lines.reqQuantity - lines.delQuantity - carriedQuantity;
                if (stillNeeded <= 0) {
                    continue;
                }
                for (std::size_t index = 0; index < inventory.size(); index++) {
                    const InventoryCell& cell = inventory.at(index);
                    if (cell.getItemID() != lines.itemID || cell.getQuantity() == 0) {
                        continue;
                    }
                    Position pickupPosition = cell.getPickupPosition();
                    int goalID = graph.toNodeId(pickupPosition.row, pickupPosition.col);
                    path = aStar(graph, agentStartID, goalID);
                    if (path.empty()) {
                        continue;
                    }
                    std::size_t cost = path.size() -1;
                    if (!bestTask.has_value() || cost < bestCost) {
                        bestTask = PickupTask{
                            index,
                            cell.getItemID(),
                            pickupPosition
                        };
                        bestCost = cost;
                    }
                }
            }
            return bestTask;
        }
}
