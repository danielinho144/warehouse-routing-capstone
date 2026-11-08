#include "warehouse_routing/agent.h"
#include <stdexcept>

namespace warehouse_routing {
    Agent::Agent(int ID,  Position position, int capacity) {
        ID_ = ID;
        position_ = position;
        status_ = AgentStatus::Idle;
        capacity_ = capacity;

        if (capacity <= 0) {
            throw std::invalid_argument("Agent capacity must be positive");
        }
    }

    //get ID
    int Agent::getId() const {
        return ID_;
    }

    //return position
    Position Agent::getPosition() const {
        return position_;
    }

    //return OrderID
    std::optional<int> Agent::getOrderId() const {
        return orderID_;
    }

    //return AgentStatus
    AgentStatus Agent::getStatus() const {
        return status_;
    }

    //update agent pos
    void Agent::moveTo(Position position) {
        position_ = position;
    }

    //store orderID, change from Idle to PickupPending
    void Agent::assignOrder(int orderID) {
        if (status_ != AgentStatus::Idle || orderID_.has_value()) {
            throw std::runtime_error("Order assign error");
        }
        orderID_ = orderID;
        status_ = AgentStatus::PickupPending;
    }


    void Agent::startDelivery() {
        if (status_ != AgentStatus::PickupPending || !orderID_.has_value()) {
            throw std::runtime_error("Cannot start delivery without an assigned pickup task");
        }
        if (getCurrentLoad() == 0) {
            throw std::runtime_error("Nothing to deliver");
        }
        status_ = AgentStatus::Delivering;
    }

    //finish delivery; continue pickup if another trip is needed, otherwise release order
    void Agent::completeDelivery(bool needsAnotherTrip) {
        if (status_ != AgentStatus::Delivering || !orderID_.has_value()) {
            throw std::runtime_error("Delivery error");
        }
        if (getCurrentLoad() != 0) {
            throw std::runtime_error("Agent still has items");
        }
        if (needsAnotherTrip) {
            status_ = AgentStatus::PickupPending;
        }
        else {
            orderID_.reset();
            status_ = AgentStatus::Idle;
        }
    }

    const std::vector<CarriedItem>& Agent::getCarriedItems() const {
        return carriedItems_;
    }

    int Agent::getCapacity() const {
        return capacity_;
    }

    int Agent::getCurrentLoad() const {
        int total = 0;

        for (const CarriedItem& item : carriedItems_) {
            total += item.quantity;
        }
        return total;
    }

    int Agent::getRemainingCapacity() const {
        return capacity_ - getCurrentLoad();
    }

    void Agent::addCarriedItems(std::string itemID, int quantity) {
        if (itemID.empty() || quantity <= 0 || quantity > getRemainingCapacity()) {
            throw std::invalid_argument("Empty ID, nonpositive quantity, or insufficient space");
        }
        for (CarriedItem& item : carriedItems_) {
            if (item.itemID == itemID) {
                item.quantity += quantity;
                return;
            }
        }
        CarriedItem newItem = {itemID, quantity};
        carriedItems_.push_back(newItem);
    }

    void Agent::removeCarriedItems(std::string itemID, int quantity) {
        if (itemID.empty() || quantity <= 0) {
            throw std::invalid_argument("Empty ID or nonpositive quantity");
        }
        for (std::size_t i = 0; i < carriedItems_.size(); ++i) {
            CarriedItem& item = carriedItems_.at(i);

            if (item.itemID == itemID) {
                if (quantity > item.quantity) {
                    throw std::invalid_argument("Not enough carried");
                }
                item.quantity -= quantity;

                if (item.quantity == 0) {
                    carriedItems_.erase(carriedItems_.begin() + i);
                }
                return;
            }
        }
        throw std::invalid_argument("Unknown carried item ID");
    }

    void Agent::releaseOrder() {
        if (status_ != AgentStatus::PickupPending || !getOrderId().has_value()) {
            throw std::runtime_error("Agent doesn't have pickup pending or orderID");
        }
        if (getCurrentLoad() != 0) {
            throw std::runtime_error("Carried items must be delivered before order release");
        }
        orderID_.reset();
        status_ = AgentStatus::Idle;
    }

}