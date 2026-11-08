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
#include <string>
#include <utility>
#include "warehouse_routing/inventory.h"
#include "warehouse_routing/cbs.h"

namespace warehouse_routing {
    //start and advance the simulation
    Simulation::Simulation(const Scenario& scenario, PlannerType planner)
    : grid_(loadMap(scenario.mapPath)),
    graph_(grid_),
    agents_(scenario.agents),
    orders_(scenario.orders),
    inventory_(scenario.inventory),
    planner_(planner) {}

    void Simulation::step() {
        assignOrders();
        updateDestinationAccess(buildPlanningGoals());
        planRequestedRoutes();

        bool allAgentsHavePath = true;
        if (planner_ == PlannerType::CBS) {
            for (const Agent& agent : agents_) {
                int agentID = agent.getId();
                if (!paths_.contains(agentID) || !pathProgress_.contains(agentID)) {
                    allAgentsHavePath = false;
                    break;
                }
            }
            if (allAgentsHavePath) {
                executeCBSTick();
            }
        }
        else {
            clearDeliveryCells();
            moveAgents();
        }

        handlePickups();
        planRequestedRoutes();
        updateDestinationAccess(buildPlanningGoals());

        handleDeliveries();
        planRequestedRoutes();
        currentTick_++;
    }

    void Simulation::run(int maxTicks) {
        while (!isFinished() && currentTick_ < maxTicks) {
            step();
        }
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

    //read simulation state and results
    int Simulation::getCurrentTick() const {
        return currentTick_;
    }

    const std::vector<Agent>& Simulation::getAgents() const {
        return agents_;
    }

    const std::vector<Order>& Simulation::getOrders() const {
        return orders_;
    }

    int Simulation::getCompletedOrderCount() const {
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

    //assign orders and handle pickup and delivery
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

    void Simulation::assignOrders() {
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
                if (assignNearestOrder(graph_, agent, orders_, inventory_)) {
                    int orderID = agent.getOrderId().value();
                    for (const Order& order : orders_) {
                        if (order.getId() == orderID) {
                            auto task = findNextPickup(graph_, agent, order, inventory_);
                            if (!task.has_value()) {
                                break;
                            }
                            int agentID = agent.getId();
                            pickUpTasks_[agentID] = task.value();
                            Position pickup = task->pickupPosition;
                            int goalID = graph_.toNodeId(pickup.row, pickup.col);
                            pendingRouteGoals_[agentID] = {goalID};
                            break;
                        }
                    }
                }
            }
        }
    }

    void Simulation::handlePickups() {
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

                Position pickupPosition = task.pickupPosition;

                if (position.row == pickupPosition.row && position.col == pickupPosition.col) {
                    int pickupNodeID = graph_.toNodeId(pickupPosition.row, pickupPosition.col);
                    if (canUseDestination(agentID, pickupNodeID) == false) {
                        break;
                    }
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
                        int goalID = graph_.toNodeId(pickup.row, pickup.col);
                        pickUpTasks_[agentID] = nextTask.value();
                        pendingRouteGoals_[agentID] = {goalID};
                    }
                    else if (agent.getCurrentLoad() > 0) {
                        Position delivery = order.getDeliveryPosition();
                        int goalID = graph_.toNodeId(delivery.row, delivery.col);
                        pendingRouteGoals_[agentID] = {goalID, true};
                    }
                    else {
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
                    if (!pendingRouteGoals_.contains(agentID) || pendingRouteGoals_.at(agentID).goalID != pickupNodeID) {
                        markDestinationClearing(agentID, pickupNodeID);
                    }
                }
                break;
            }
        }
    }

    void Simulation::handleDeliveries() {
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
                    int deliveryNodeID = graph_.toNodeId(deliveryPosition.row, deliveryPosition.col);
                    if (canUseDestination(agent.getId(), deliveryNodeID) == false) {
                        break;
                    }
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
                            int goalID = graph_.toNodeId(pickup.row, pickup.col);
                            pickUpTasks_[agentID] = nextTask.value();
                            pendingRouteGoals_[agentID] = {goalID};
                        }
                        else if (remaining == RemainingPickupStatus::Exhausted) {
                            order.markUnfinished();
                            agent.completeDelivery(false);
                        }
                        else {
                            throw std::runtime_error("Remaining order stock is unreachable");
                        }
                    }
                    if (!pendingRouteGoals_.contains(agentID) || pendingRouteGoals_.at(agentID).goalID != deliveryNodeID) {
                        markDestinationClearing(agentID, deliveryNodeID);
                    }
                }
                break;
            }
        }
    }

    //track route requests, paths, and progress by agent ID
    std::unordered_map<int, int> Simulation::buildPlanningGoals() const {
        std::unordered_map<int, int> goals;

        for (const Agent& agent : agents_) {
            int agentID = agent.getId();

            if (pendingRouteGoals_.contains(agentID)) {
                goals[agentID] = pendingRouteGoals_.at(agentID).goalID;
                continue;
            }

            if (agent.getStatus() == AgentStatus::PickupPending) {
                if (!pickUpTasks_.contains(agentID)) {
                    throw std::logic_error("Pickup agent has no pickup task");
                }
                Position pickup = pickUpTasks_.at(agentID).pickupPosition;
                goals[agentID] = graph_.toNodeId(pickup.row, pickup.col);
                continue;
            }
            if (agent.getStatus() == AgentStatus::Delivering) {
                bool found = false;
                for (const Order& order : orders_) {
                    if (agent.getOrderId() != order.getId()) {
                        continue;
                    }
                    Position delivery = order.getDeliveryPosition();
                    goals[agentID] = graph_.toNodeId(delivery.row, delivery.col);
                    found = true;
                    break;
                }
                if (!found) {
                    throw std::logic_error("Delivering agent has no matching order");
                }
                continue;
            }
            if (paths_.contains(agentID) && !paths_.at(agentID).empty()) {
                goals[agentID] = paths_.at(agentID).back();
                continue;
            }
            Position position = agent.getPosition();
            goals[agentID] = graph_.toNodeId(position.row, position.col);
        }
        return goals;
    }

    void Simulation::planRequestedRoutes() {
        if (planner_ == PlannerType::CBS) {
            std::set<int> reservedParking;
            for (const Agent& agent : agents_) {
                if (agent.getStatus() != AgentStatus::Idle) {
                    continue;
                }
                int agentID = agent.getId();
                if (pendingRouteGoals_.contains(agentID)) {
                    auto request = pendingRouteGoals_.at(agentID);
                    reservedParking.insert(request.goalID);
                } else if (paths_.contains(agentID)) {
                    const std::vector<int>& path = paths_.at(agentID);
                    if (!path.empty()) {
                        reservedParking.insert(path.back());
                    }
                }
            }
            for (Agent& agent : agents_) {
                if (agent.getStatus() == AgentStatus::Idle) {
                    Position node = agent.getPosition();
                    int pos = graph_.toNodeId(node.row, node.col);
                    if (!destinationAccess_.contains(pos)) {
                        continue;
                    }
                    const DestinationAccess& access = destinationAccess_.at(pos);
                    if (access.waitingAgents.empty()) {
                        continue;
                    }
                    int agentID = agent.getId();
                    if (pendingRouteGoals_.contains(agentID) && pendingRouteGoals_.at(agentID).goalID != pos) {
                        continue;
                    }
                    if (paths_.contains(agentID)) {
                        const std::vector<int>& path = paths_.at(agentID);
                        if (!path.empty() && path.back() != pos) {
                            continue;
                        }
                    }
                    std::set<int> occupied;
                    for (Agent& otherAgent : agents_) {
                        if (otherAgent.getId() == agentID) {
                            continue;
                        }
                        Position position = otherAgent.getPosition();
                        occupied.insert(graph_.toNodeId(position.row, position.col));
                    }
                    std::queue<int> candidates;
                    std::set<int> visited;
                    candidates.push(pos);
                    visited.insert(pos);
                    while (!candidates.empty()) {
                        int candidate = candidates.front();
                        candidates.pop();
                        Position canPos = {graph_.rowOf(candidate), graph_.colOf(candidate)};
                        bool suitable = candidate != pos && !reservedParking.contains(candidate);

                        for (InventoryCell& item : inventory_) {
                            Position pickup = item.getPickupPosition();
                            if (canPos.row == pickup.row && canPos.col == pickup.col) {
                                suitable = false;
                                break;
                            }
                        }

                        for (Order& order : orders_) {
                            Position deliveryPos = order.getDeliveryPosition();
                            if (std::abs(canPos.row - deliveryPos.row) <= 1 &&
                                std::abs(canPos.col - deliveryPos.col) <= 1) {
                                    suitable = false;
                                    break;
                                }
                        }

                        if (suitable) {
                            pendingRouteGoals_[agentID] = {candidate, false};
                            reservedParking.insert(candidate);
                            break;
                        }

                        for (int neighbor : graph_.getNeighbors(candidate)) {
                            if (visited.contains(neighbor) || occupied.contains(neighbor)) {
                                continue;
                            }
                            visited.insert(neighbor);
                            candidates.push(neighbor);
                        }
                    }
                }
            }
            auto taskGoals = buildPlanningGoals();
            updateDestinationAccess(taskGoals);
            auto cbsGoals = buildCBSGoals(taskGoals);

            bool needsPlan = !pendingRouteGoals_.empty();
            bool hasActiveAgent = false;
            for (Agent& agent : agents_) {
                if (agent.getStatus() == AgentStatus::PickupPending ||
                    agent.getStatus() == AgentStatus::Delivering) {
                        hasActiveAgent = true;
                        break;
                    }
            }
            if (hasActiveAgent) {
                for (Agent& agent : agents_) {
                    int agentID = agent.getId();
                    if (!paths_.contains(agentID) || paths_.at(agentID).empty() || !pathProgress_.contains(agentID)) {
                        needsPlan = true;
                        break;
                    }
                    if (paths_.at(agentID).back() != cbsGoals.at(agentID)) {
                        needsPlan = true;
                        break;
                    }
                }
            }
            if (!needsPlan) {
                return;
            }

            const int maxTick = 20;
            const int maxExpandedNodes = 100;
            if (!tryPlanCBSRoutes(taskGoals, maxTick, maxExpandedNodes)) {
                throw std::runtime_error("CBS could not find routes within the search limits");
            }
            for (Agent& agent : agents_) {
                int agentID = agent.getId();
                if (pendingRouteGoals_.contains(agentID) && pendingRouteGoals_.at(agentID).startDelivery) {
                    agent.startDelivery();
                }
            }
            pendingRouteGoals_.clear();
            return;
        }
        for (Agent& agent : agents_) {
            int agentID = agent.getId();

            if (!pendingRouteGoals_.contains(agentID)) {
                continue;
            }
            Position start = agent.getPosition();
            int startID = graph_.toNodeId(start.row, start.col);
            const RouteRequest& request = pendingRouteGoals_.at(agentID);
            int goalID = request.goalID;
            std::vector<int> path = aStar(graph_, startID, goalID);
            if (path.empty()) {
                throw std::runtime_error("No route for agent " + std::to_string(agentID) + " to node " + std::to_string(goalID));
            }
            paths_[agentID] = std::move(path);
            pathProgress_[agentID] = 0;
            if (request.startDelivery) {
                agent.startDelivery();
            }
        }
        pendingRouteGoals_.clear();
    }

    //manage owners and waiting agents at shared cells
    void Simulation::updateDestinationAccess(const std::unordered_map<int, int>& taskGoals) {

        std::unordered_map<int, std::set<int>> requests;
        std::unordered_map<int, int> currentNodes;

        for (const Agent& agent : agents_) {
            int agentID = agent.getId();
            Position position = agent.getPosition();

            currentNodes[agentID] = graph_.toNodeId(position.row, position.col);

            if (agent.getStatus() != AgentStatus::PickupPending &&
                agent.getStatus() != AgentStatus::Delivering) {
                continue;
            }

            int goalID = taskGoals.at(agentID);
            requests[goalID].insert(agentID);
            destinationAccess_.try_emplace(goalID);
        }
        for (auto& [destination, access] : destinationAccess_) {
            auto requestIt = requests.find(destination);

            auto i = access.waitingAgents.begin();
            while (i != access.waitingAgents.end()) {
                int agentID = *i;
                bool cancelled = requestIt == requests.end() ||
                                 !requestIt->second.contains(agentID);
                bool isOwner = access.owner.has_value() &&
                               access.owner.value() == agentID;

                if (cancelled || isOwner) {
                    i = access.waitingAgents.erase(i);
                }
                else {
                    ++i;
                }
            }

            if (access.owner.has_value()) {
                int ownerID = access.owner.value();

                bool ownerStillRequesting = (requestIt != requests.end()) && requestIt->second.contains(ownerID);
                if (!ownerStillRequesting) {
                    access.clearing = true;
                }

                if (access.clearing && currentNodes.at(ownerID) != destination) {
                    access.owner.reset();
                    access.clearing = false;
                }
            }

            if (requestIt != requests.end()) {
                for (int agentID : requestIt->second) {
                    if (access.owner.has_value() && access.owner.value() == agentID) {
                        continue;
                    }

                    bool alreadyWaiting = std::find(access.waitingAgents.begin(), access.waitingAgents.end(), agentID) != access.waitingAgents.end();
                    if (!alreadyWaiting) {
                        access.waitingAgents.push_back(agentID);
                    }
                }
            }

            if (access.owner.has_value()) {
                continue;
            }

            std::optional<int> occupant;
            for (const auto& [agentID, nodeID] : currentNodes) {
                if (nodeID == destination) {
                    occupant = agentID;
                    break;
                }
            }

            if (occupant.has_value()) {
                bool occupantIsRequesting = requestIt != requests.end() &&
                                            requestIt->second.contains(occupant.value());
                if (occupantIsRequesting) {
                    access.owner = occupant.value();
                    access.clearing = false;
                    std::erase(access.waitingAgents, occupant.value());
                }
                continue;
            }

            if (!access.waitingAgents.empty()) {
                access.owner = access.waitingAgents.front();
                access.waitingAgents.pop_front();
                access.clearing = false;
            }
        }
    }

    bool Simulation::canUseDestination(int agentID, int destinationID) const {
        if (!destinationAccess_.contains(destinationID)) {
            return false;
        }
        const DestinationAccess& state = destinationAccess_.at(destinationID);
        if (!state.owner.has_value()) {
            return false;
        }
        if (state.owner.value() != agentID) {
            return false;
        }
        if (state.clearing) {
            return false;
        }
        return true;
    }

    void Simulation::markDestinationClearing(int agentID, int destinationID) {
        if (!destinationAccess_.contains(destinationID)) {
            throw std::logic_error("Destination does not exist");
        }
        DestinationAccess& state = destinationAccess_.at(destinationID);
        if (!state.owner.has_value() || state.owner.value() != agentID) {
            throw std::logic_error("State has no owner or owner value does not match");
        }
        state.clearing = true;
    }

    //plan and execute coordinated CBS routes
    std::unordered_map<int, int> Simulation::buildCBSGoals(const std::unordered_map<int, int>& taskGoals) const {
        std::unordered_map<int, int> cbsGoals = taskGoals;
        for (const Agent& agent : agents_) {
            int agentID = agent.getId();
            auto taskGoal = taskGoals.at(agentID);
            if (!destinationAccess_.contains(taskGoal)) {
                continue;
            }
            const std::deque<int>& waiters = destinationAccess_.at(taskGoal).waitingAgents;
            if (std::find(waiters.begin(), waiters.end(), agentID) != waiters.end()) {
                Position position = agent.getPosition();
                cbsGoals[agentID] = graph_.toNodeId(position.row, position.col);
            }
        }
        return cbsGoals;
    }

    bool Simulation::tryPlanCBSRoutes(const std::unordered_map<int, int>& taskGoals, int maxTick, int maxExpandedNodes) {
        std::unordered_map<int, int> cbsGoals = buildCBSGoals(taskGoals);
        auto result = planCBSPaths(graph_, agents_, cbsGoals, maxTick, maxExpandedNodes);
        if (!result.has_value()) {
            return false;
        }
        paths_ = result.value();
        pathProgress_.clear();

        for (const Agent& agent : agents_) {
            pathProgress_[agent.getId()] = 0;
        }
        return true;
    }

    void Simulation::executeCBSTick() {
        for (Agent& agent : agents_) {
            int agentID = agent.getId();
            const std::vector<int>& path = paths_.at(agentID);
            std::size_t& progress = pathProgress_.at(agentID);
            if (progress + 1 >= path.size()) {
                continue;
            }
            int nextNode = path.at(progress + 1);
            agent.moveTo(Position {graph_.rowOf(nextNode), graph_.colOf(nextNode)});
            progress++;
        }
    }

    //handle parking, movement, and conflicts for the baseline planner
    void Simulation::clearDeliveryCells() {
        std::set<int> reservedParking;
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
            int currentID = graph_.toNodeId(position.row, position.col);
            bool nearDelivery = false;
            for (const Order& order : orders_) {
                Position delivery = order.getDeliveryPosition();
                if (std::abs(position.row - delivery.row) <= 1 &&
                    std::abs(position.col - delivery.col) <= 1) {
                    nearDelivery = true;
                    break;
                }
            }
            bool blockingRequest = false;
            if (destinationAccess_.contains(currentID)) {
                const DestinationAccess& record = destinationAccess_.at(currentID);
                if (!record.waitingAgents.empty()) {
                    blockingRequest = true;
                }
            }
            if (!nearDelivery && !blockingRequest) {
                continue;
            }

            int agentID = agent.getId();
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
    }

    void Simulation::moveAgents() {
        auto proposedMoves = proposeMoves();
        attemptDetours(proposedMoves);
        for (Agent& agent : agents_) {
            int agentID = agent.getId();
            auto nextMove = proposedMoves.at(agentID);
            if (!destinationAccess_.contains(nextMove)) {
                continue;
            }
            const DestinationAccess& record = destinationAccess_.at(nextMove);
            const std::deque<int>& waiters = record.waitingAgents;

            if (std::find(waiters.begin(), waiters.end(), agentID) != waiters.end()) {
                Position position = agent.getPosition();
                proposedMoves.at(agentID) = graph_.toNodeId(position.row, position.col);
            }
        }
        resolveConflicts(proposedMoves);
        executeMoves(proposedMoves);

    }

    std::unordered_map<int, int> Simulation::proposeMoves() const {
        std::unordered_map<int, int> proposedMoves;
        for (const Agent& agent : agents_) {
            int agentID = agent.getId();
            Position position = agent.getPosition();
            int currentID = graph_.toNodeId(position.row, position.col);
            proposedMoves[agentID] = currentID;

            if (paths_.contains(agentID)) {
                const std::vector<int>& path = paths_.at(agentID);
                std::size_t index = pathProgress_.at(agentID);

                if (index + 1 < path.size()) {
                    proposedMoves[agentID] = path.at(index + 1);
                }
            }
        }
        return proposedMoves;
    }

    void Simulation::attemptDetours(std::unordered_map<int, int>& proposedMoves) {
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
    }

    void Simulation::resolveConflicts(std::unordered_map<int, int>& proposedMoves) {
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
    }

    void Simulation::executeMoves(const std::unordered_map<int, int>& proposedMoves) {
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
    }
}
