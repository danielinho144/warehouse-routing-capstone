#include "warehouse_routing/agent.h"
#include <stdexcept>

namespace warehouse_routing {
    Agent::Agent(int ID,  Position position) {
        ID_ = ID;
        position_ = position;
        status_ = AgentStatus::Idle;
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
        //do stuff
        position_ = position;
    }

    //store orderID, change from Idle to PickupPending
    void Agent::assignOrder(int orderID) {
        //do stuff
        if (status_ != AgentStatus::Idle || orderID_.has_value()) {
            throw std::runtime_error("Order assign error");
        }
        orderID_ = orderID;
        status_ = AgentStatus::PickupPending;
    }

    //change PickupPending to Delivering
    void Agent::pickUp() {
        // do stuff
        if (status_ != AgentStatus::PickupPending || !orderID_.has_value()) {
            throw std::runtime_error("Pickup error");
        }
        status_ = AgentStatus::Delivering;
    }

    //clear orderID, change back to Idle
    void Agent::completeDelivery() {
        //do stuff
        if (status_ != AgentStatus::Delivering || !orderID_.has_value()) {
            throw std::runtime_error("Delivery error");
        }
        orderID_.reset();
        status_ = AgentStatus::Idle;
    }
}

