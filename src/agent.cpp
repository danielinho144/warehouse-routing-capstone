#include "warehouse_routing/agent.h"

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
}

