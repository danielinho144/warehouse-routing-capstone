#include "warehouse_routing/simulation.h"
#include "warehouse_routing/map_loader.h"
#include "warehouse_routing/order_assignment.h"
#include "warehouse_routing/astar.h"
#include "warehouse_routing/conflict_detection.h"
#include <set>
#include <queue>
#include <algorithm>
#include <cstdlib>
#include <stdexcept>
#include "warehouse_routing/inventory.h"

//coordinate everything and advance time
/*
Flow: Choose orders -> Plan Routes -> Propose Moves -> Attempt Detours -> Resolve Conflicts -> Move
-> Pickup/Deliver -> Advance Tick
*/

namespace warehouse_routing {
    namespace {
        enum class RemainingPickupStatus { Available, Exhausted, Unreachable };

        RemainingPickupStatus checkRemainingPickup(const Graph& graph, const Agent& agent,
            const Order& order, const std::vector<InventoryCell>& inventory) {
            bool stockAvailable = false;
            Position position = agent.getPosition();
            int startID = graph.toNodeId(position.row, position.col);

            for (const OrderLine& line : order.getLines()) {
                if (line.delQuantity >= line.reqQuantity) {
                    continue;
                }
                for (const InventoryCell& cell : inventory) {
                    if (cell.getItemID() != line.itemID || cell.getQuantity() == 0) {
                        continue;
                    }
                    stockAvailable = true;
                    Position pickup = cell.getPickupPosition();
                    int goalID = graph.toNodeId(pickup.row, pickup.col);
                    if (!aStar(graph, startID, goalID).empty()) {
                        return RemainingPickupStatus::Available;
                    }
                }
            }
            if (stockAvailable) {
                return RemainingPickupStatus::Unreachable;
            }
            return RemainingPickupStatus::Exhausted;
        }
    }

    Simulation::Simulation(const Scenario& scenario)
    : grid_(loadMap(scenario.mapPath)),
    graph_(grid_),
    agents_(scenario.agents),
    orders_(scenario.orders),
    inventory_(scenario.inventory) {}

    void Simulation::step() {
        //assigns orders
        for (Order& order : orders_) {
            bool flag = false;
            if (order.getStatus() != OrderStatus::Unclaimed) {
                continue;
            }
            for (const OrderLine& line : order.getLines()) {
                if (line.delQuantity == line.reqQuantity) {
                    continue;
                }
                for (const InventoryCell& cell : inventory_) {
                    if (cell.getItemID() == line.itemID && cell.getQuantity() > 0) {
                        flag = true;
                        break;
                    }
                }
                if (flag == true) {
                    break;
                }
            }
            if (flag == false) {
                order.markUnfinished();
            }
        }
        for (Agent& agent : agents_) {
            if (agent.getStatus() == AgentStatus::Idle) {
                if (assignNearestOrder(graph_, agent, orders_, inventory_)) { //assign idle agents orders
                    int orderID = agent.getOrderId().value();
                    //plan pickup routes
                    for (const Order& order : orders_) {
                        if (order.getId() == orderID) {
                            auto task = findNextPickup(graph_, agent, order, inventory_);
                            if (!task.has_value()) {
                                break;
                            }
                            Position start = agent.getPosition();
                            int agentID = agent.getId();
                            pickUpTasks_[agentID] = task.value();
                            Position pickup = task->pickupPosition;
                            //Position pickup = order.getPickupPosition();
                            int startID = graph_.toNodeId(start.row, start.col);
                            int goalID = graph_.toNodeId(pickup.row, pickup.col);

                            paths_[agentID] = aStar(graph_, startID, goalID); //agent's route as nodeIDs
                            pathProgress_[agentID] = 0; //current index within route
                            break;
                        }
                    }
                }
            }
        }
        //cleaning delivery cells
        std::set<int> reservedParking; //parking destinations selected for idle agents
        for (const Agent& agent : agents_) {
            if (agent.getStatus() == AgentStatus::Idle && paths_.contains(agent.getId())) {
                const std::vector<int>& path = paths_.at(agent.getId());
                if (!path.empty()) {
                    reservedParking.insert(path.back());
                }
            }
        }
        for (const Agent& agent : agents_) {
            if (agent.getStatus() != AgentStatus::Idle) {
                continue;
            }
            Position position = agent.getPosition();
            bool nearDelivery = false;
            for (const Order& order : orders_) {
                Position delivery = order.getDeliveryPosition();
                if (std::abs(position.row - delivery.row) <= 1 &&
                    std::abs(position.col - delivery.col) <= 1) {
                    nearDelivery = true;
                    break;
                }
            }
            if (!nearDelivery) {
                continue;
            }

            int agentID = agent.getId();
            int currentID = graph_.toNodeId(position.row, position.col);
            if (paths_.contains(agentID) && !paths_.at(agentID).empty()) {
                reservedParking.erase(paths_.at(agentID).back());
            }
            paths_.erase(agentID);
            pathProgress_.erase(agentID);

            std::set<int> occupied;
            for (const Agent& otherAgent : agents_) {
                if (otherAgent.getId() != agentID) {
                    Position other = otherAgent.getPosition();
                    occupied.insert(graph_.toNodeId(other.row, other.col));
                }
            }
            std::queue<int> candidates;
            std::unordered_map<int, int> previous;
            candidates.push(currentID);
            previous[currentID] = currentID;

            while (!candidates.empty()) {
                int candidateID = candidates.front();
                candidates.pop();
                Position candidate{graph_.rowOf(candidateID), graph_.colOf(candidateID)};
                bool suitable = !reservedParking.contains(candidateID);
                for (const Order& order : orders_) {
                    /*
                    Position pickup = order.getPickupPosition();
                    for (const InventoryCell& item : inventory_) {
                        
                    }
                    Position delivery = order.getDeliveryPosition();
                    if ((candidate.row == pickup.row && candidate.col == pickup.col) ||
                        (std::abs(candidate.row - delivery.row) <= 1 &&
                         std::abs(candidate.col - delivery.col) <= 1)) {
                    */
                   Position delivery = order.getDeliveryPosition();
                    if (std::abs(candidate.row - delivery.row) <= 1 &&
                        std::abs(candidate.col - delivery.col) <= 1) {
                        suitable = false;
                        break;
                    }
                }
                for (const InventoryCell& item : inventory_) {
                    Position pickup = item.getPickupPosition();
                    if (candidate.row == pickup.row && candidate.col == pickup.col) {
                        suitable = false;
                        break;
                    }
                }
                if (suitable) {
                    std::vector<int> path;
                    int node = candidateID;
                    while (node != currentID) {
                        path.push_back(node);
                        node = previous.at(node);
                    }
                    path.push_back(currentID);
                    std::reverse(path.begin(), path.end());
                    paths_[agentID] = path;
                    pathProgress_[agentID] = 0;
                    reservedParking.insert(candidateID);
                    break;
                }
                for (int neighbor : graph_.getNeighbors(candidateID)) {
                    if (previous.contains(neighbor) || occupied.contains(neighbor)) {
                        continue;
                    }
                    previous[neighbor] = candidateID;
                    candidates.push(neighbor);
                }
            }
        }
        //propose moves
        std::unordered_map<int, int> proposedMoves;
        for (Agent& agent : agents_) {
            int agentID = agent.getId();
            Position position = agent.getPosition();
            int currentID = graph_.toNodeId(position.row, position.col);
            proposedMoves[agentID] = currentID; //proposed destination for tick

            if (paths_.contains(agentID)) {
                const std::vector<int>& path = paths_.at(agentID);
                std::size_t index = pathProgress_.at(agentID);

                if (index + 1 < path.size()) {
                    proposedMoves[agentID] = path.at(index + 1);
                }
            }
        }
        for (const Agent& agent : agents_) {
            if (agent.getStatus() != AgentStatus::PickupPending &&
                agent.getStatus() != AgentStatus::Delivering) {
                continue;
            }
            int agentID = agent.getId();
            Position position = agent.getPosition();
            int currentID = graph_.toNodeId(position.row, position.col);
            bool needsDetour = false;
            for (const Agent& otherAgent : agents_) {
                if (otherAgent.getId() == agentID) {
                    continue;
                }
                Position otherPosition = otherAgent.getPosition();
                int otherCurrentID = graph_.toNodeId(otherPosition.row, otherPosition.col);
                int otherNextID = proposedMoves.at(otherAgent.getId());
                if (hasSwapConflict(currentID, proposedMoves.at(agentID),
                                    otherCurrentID, otherNextID) ||
                    (proposedMoves.at(agentID) == otherCurrentID && otherNextID == otherCurrentID)) {
                    needsDetour = true;
                    break;
                }
            }
            if (!needsDetour) {
                continue;
            }

            Grid detourGrid = grid_;
            for (const Agent& otherAgent : agents_) {
                Position occupied = otherAgent.getPosition();
                detourGrid.setCellType(occupied.row, occupied.col, CellType::Shelf);
            }
            Graph detourGraph(detourGrid);

            for (const Order& order : orders_) {
                if (order.getId() != agent.getOrderId().value()) {
                    continue;
                }
                //Position goal = order.getPickupPosition();
                Position goal;
                if (agent.getStatus() == AgentStatus::Delivering) {
                    goal = order.getDeliveryPosition();
                } 
                else {
                    if (!pickUpTasks_.contains(agentID)) {
                        break;
                    }
                    if (pickUpTasks_.contains(agentID)) {
                        goal = pickUpTasks_.at(agentID).pickupPosition;
                    }
                }
                int goalID = graph_.toNodeId(goal.row, goal.col);
                std::vector<int> bestPath;

                for (int neighbor : graph_.getNeighbors(currentID)) {
                    bool suitable = true;
                    for (const Order& otherOrder : orders_) {
                        Position delivery = otherOrder.getDeliveryPosition();
                        if (neighbor != goalID && neighbor == graph_.toNodeId(delivery.row, delivery.col)) {
                            suitable = false;
                            break;
                        }
                    }
                    if (!suitable) {
                        continue;
                    }
                    for (const Agent& otherAgent : agents_) {
                        if (otherAgent.getId() == agentID) {
                            continue;
                        }
                        Position otherPosition = otherAgent.getPosition();
                        int otherCurrentID = graph_.toNodeId(otherPosition.row, otherPosition.col);
                        int otherNextID = proposedMoves.at(otherAgent.getId());
                        if (neighbor == otherCurrentID ||
                            hasVertexConflict(neighbor, otherNextID) ||
                            hasSwapConflict(currentID, neighbor, otherCurrentID, otherNextID) ||
                            hasDiagonalConflict(graph_, currentID, neighbor, otherCurrentID, otherNextID)) {
                            suitable = false;
                            break;
                        }
                    }
                    if (!suitable) {
                        continue;
                    }

                    std::vector<int> detour = aStar(detourGraph, neighbor, goalID);
                    if (detour.empty()) {
                        continue;
                    }
                    for (int node : detour) {
                        if (node == currentID) {
                            suitable = false;
                            break;
                        }
                    }
                    if (!suitable) {
                        continue;
                    }
                    if (bestPath.empty() || detour.size() + 1 < bestPath.size()) {
                        bestPath = {currentID};
                        bestPath.insert(bestPath.end(), detour.begin(), detour.end());
                    }
                }
                if (!bestPath.empty()) {
                    paths_[agentID] = bestPath;
                    pathProgress_[agentID] = 0;
                    proposedMoves[agentID] = bestPath.at(1);
                }
                break;
            }
        }
        //find conflicts
        std::set<int> blockedAgents;
        bool changed;

        do {
            changed = false;

            for (std::size_t i = 0; i < agents_.size(); i++) {
                for (std::size_t j = i + 1; j < agents_.size(); j++) {
                    const Agent& agentA = agents_.at(i);
                    const Agent& agentB = agents_.at(j);

                    Position positionA = agentA.getPosition();
                    Position positionB = agentB.getPosition();

                    int currentNodeA = graph_.toNodeId(positionA.row, positionA.col);
                    int currentNodeB = graph_.toNodeId(positionB.row, positionB.col);

                    int nextNodeA = proposedMoves.at(agentA.getId());
                    int nextNodeB = proposedMoves.at(agentB.getId());

                    if (hasVertexConflict(nextNodeA, nextNodeB)) {
                        if (nextNodeA == currentNodeA) {
                            blockedAgents.insert(agentB.getId());
                        }
                        else if (nextNodeB == currentNodeB) {
                            blockedAgents.insert(agentA.getId());
                        }
                        else if (agentA.getId() < agentB.getId()) {
                            blockedAgents.insert(agentB.getId());
                        }
                        else {
                            blockedAgents.insert(agentA.getId());
                        }
                    }
                    else if (hasSwapConflict(currentNodeA, nextNodeA, currentNodeB, nextNodeB) ||
                    hasDiagonalConflict(graph_, currentNodeA, nextNodeA, currentNodeB, nextNodeB)) {
                        blockedAgents.insert(agentA.getId());
                        blockedAgents.insert(agentB.getId());
                    }

                }
            }
        //make blocked agents wait

            for (Agent& agent : agents_) {
                if (blockedAgents.contains(agent.getId())) {
                    int agentID = agent.getId();
                    Position position = agent.getPosition();
                    int currentID = graph_.toNodeId(position.row, position.col);
                    if(proposedMoves.at(agentID) != currentID) {
                        proposedMoves[agentID] = currentID;
                        changed = true;
                    }
                }
            }
        } while (changed);

        for (Agent& agent : agents_) {
            Position position = agent.getPosition();
            int currentID = graph_.toNodeId(position.row, position.col);
            int agentID = agent.getId();
            int finalNode = proposedMoves.at(agentID);

            if (finalNode != currentID) {
                int finalRow = graph_.rowOf(finalNode);
                int finalCol = graph_.colOf(finalNode);
                Position finalPosition{finalRow, finalCol};

                agent.moveTo(finalPosition);
                pathProgress_.at(agentID)++;
            }
        }
        //pickup handling
        for (Agent& agent : agents_) {
            if (agent.getStatus() != AgentStatus::PickupPending) {
                continue;
            }
            int agentOrderID = agent.getOrderId().value();
            int agentID = agent.getId();
            if (!pickUpTasks_.contains(agentID)) {
                continue;
            }
            const PickupTask& task = pickUpTasks_.at(agentID);
            
            for (Order& order : orders_) {
                if (order.getId() != agentOrderID) {
                    continue;
                }
                Position position = agent.getPosition();
                
                //Position pickupPosition = order.getPickupPosition();
                Position pickupPosition = task.pickupPosition;

                if (position.row == pickupPosition.row && position.col == pickupPosition.col) {
                    InventoryCell& cell = inventory_.at(task.inventoryIndex);
                    int pickupQuantity = 0;

                    for (const OrderLine& line : order.getLines()) {
                        if (line.itemID != task.itemID) {
                            continue;
                        }
                        int carriedQuantity = 0;
                        for (const CarriedItem& item : agent.getCarriedItems()) {
                            if (item.itemID == task.itemID) {
                                carriedQuantity += item.quantity;
                            }
                        }
                        int stillNeeded = line.reqQuantity - line.delQuantity - carriedQuantity;
                        if (stillNeeded > 0) {
                            pickupQuantity = std::min({stillNeeded, cell.getQuantity(), agent.getRemainingCapacity()});
                        }
                        break;
                    }
                    if (pickupQuantity > 0) {
                        agent.addCarriedItems(task.itemID, pickupQuantity);
                        cell.removeStock(pickupQuantity);
                    }
                    pickUpTasks_.erase(agentID);
                    paths_.erase(agentID);
                    pathProgress_.erase(agentID);
                    auto nextTask = findNextPickup(graph_, agent, order, inventory_);
                    if (nextTask.has_value()) {
                        Position pickup = nextTask->pickupPosition;
                        int startID = graph_.toNodeId(position.row, position.col);
                        int goalID = graph_.toNodeId(pickup.row, pickup.col);
                        std::vector<int> pickupPath = aStar(graph_, startID, goalID);

                        if (pickupPath.empty()) {
                            throw std::runtime_error("No route to next pickup");
                        }
                        pickUpTasks_[agentID] = nextTask.value();
                        paths_[agentID] = pickupPath;
                        pathProgress_[agentID] = 0;
                    }
                    else if (agent.getCurrentLoad() > 0) {
                        Position delivery = order.getDeliveryPosition();
                        int startID = graph_.toNodeId(position.row, position.col);
                        int goalID = graph_.toNodeId(delivery.row, delivery.col);
                        std::vector<int> deliveryPath = aStar(graph_, startID, goalID);

                        if (deliveryPath.empty()) {
                            throw std::runtime_error("No route to delivery");
                        }
                        paths_[agentID] = deliveryPath;
                        pathProgress_[agentID] = 0;
                        agent.startDelivery();
                    }
                    else {
                        //throw std::runtime_error("No reachable pickup and agent has nothing to deliver");
                        RemainingPickupStatus remaining = checkRemainingPickup(graph_, agent, order, inventory_);
                        if (remaining == RemainingPickupStatus::Exhausted) {
                            order.markUnfinished();
                            agent.releaseOrder();
                        }
                        else if(remaining == RemainingPickupStatus::Unreachable) {
                            throw std::runtime_error("No path to stock");
                        }
                        else if (remaining == RemainingPickupStatus::Available){
                            throw std::runtime_error("Inconsistency");
                        }
                    }
                    /*
                    Position delivery = order.getDeliveryPosition();
                    int startID = graph_.toNodeId(position.row, position.col);
                    int goalID = graph_.toNodeId(delivery.row, delivery.col);

                    std::vector<int> deliveryPath = aStar(graph_, startID, goalID);

                    if (deliveryPath.empty()) {
                        throw std::runtime_error("No route to delivery");
                    }
                    int agentID = agent.getId();
                    paths_[agentID] = deliveryPath;
                    pathProgress_[agentID] = 0;
                    //agent.pickUp();
                    */
                }
                break;
            }
        }
        //delivery completion
        for (Agent& agent : agents_) {
            if (agent.getStatus() != AgentStatus::Delivering) {
                continue;
            }
            int agentOrderID = agent.getOrderId().value();

            for (Order& order : orders_) {
                if (order.getId() != agentOrderID) {
                    continue;
                }
                Position position = agent.getPosition();
                Position deliveryPosition = order.getDeliveryPosition();

                if (position.row == deliveryPosition.row &&
                position.col == deliveryPosition.col) {
                    //order.markCompleted();
                    //agent.completeDelivery();
                    std::vector<CarriedItem> unload = agent.getCarriedItems();
                    for (const CarriedItem& item : unload) {
                        order.recordDelivery(item.itemID, item.quantity);
                        agent.removeCarriedItems(item.itemID, item.quantity);
                    }
                    bool fullyDelivered = true;
                    for (const OrderLine& line : order.getLines()) {
                        if (line.delQuantity != line.reqQuantity) {
                            fullyDelivered = false;
                            break;
                        }
                    }
                    int agentID = agent.getId();
                    paths_.erase(agentID);
                    pathProgress_.erase(agentID);

                    if (fullyDelivered == true) {
                        order.markCompleted();
                        agent.completeDelivery(false);
                    }
                    else {
                        RemainingPickupStatus remaining = checkRemainingPickup(graph_, agent, order, inventory_);
                        if (remaining == RemainingPickupStatus::Available) {
                            agent.completeDelivery(true);
                            auto nextTask = findNextPickup(graph_, agent, order, inventory_);
                            if (!nextTask.has_value()) {
                                throw std::runtime_error("No pickup task for remaining reachable stock");
                            }
                            Position pickup = nextTask->pickupPosition;
                            int startID = graph_.toNodeId(position.row, position.col);
                            int goalID = graph_.toNodeId(pickup.row, pickup.col);
                            std::vector<int> pickupPath = aStar(graph_, startID, goalID);
                            if (pickupPath.empty()) {
                                throw std::runtime_error("No route to next pickup");
                            }
                            pickUpTasks_[agentID] = nextTask.value();
                            paths_[agentID] = pickupPath;
                            pathProgress_[agentID] = 0;
                        }
                        else if (remaining == RemainingPickupStatus::Exhausted) {
                            order.markUnfinished();
                            agent.completeDelivery(false);
                        }
                        else {
                            throw std::runtime_error("Remaining order stock is unreachable");
                        }
                    }
                }
                break;
            }
        }
        currentTick_++;
    }

    int Simulation::getCurrentTick() const {
        return currentTick_;
    }

    const std::vector<Agent>& Simulation::getAgents() const {
        return agents_;
    }

    const std::vector<Order>& Simulation::getOrders() const {
        return orders_;
    }

    bool Simulation::isFinished() const {
        for (const Order& order : orders_) {
            if (order.getStatus() != OrderStatus::Completed &&
            order.getStatus() != OrderStatus::Unfinished) {
                return false;
            }
        }
        for (const Agent& agent : agents_) {
            if (agent.getCurrentLoad() > 0 || agent.getOrderId().has_value() ||
            agent.getStatus() != AgentStatus::Idle) {
                return false;
            }
        }
        return true;
    }

    void Simulation::run(int maxTicks) {
        while (!isFinished() && currentTick_ < maxTicks) {
            step();
        }
    }

    int Simulation::getCompletedOrderCount() const {
        //count completed orders
        int count = 0;
        for (const Order& order : orders_) {
            if (order.getStatus() == OrderStatus::Completed) {
                count++;
            }
        }
        return count;
    }

    double Simulation::getThroughput() const {
        if (currentTick_ == 0) {
            return 0.0;
        }
        return static_cast<double>(getCompletedOrderCount()) / currentTick_;
    }
}
