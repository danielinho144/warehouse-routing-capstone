#pragma once
#include <optional>
#include "warehouse_routing/position.h"

//agent should have
// ID
// Current Position
// Assigned Order ID or None
// Status: idle, heading to pickup, delivering

namespace warehouse_routing {

    enum class AgentStatus {
        Idle,
        PickupPending,
        Delivering
    };

    class Agent {
        public:
        Agent(int ID, Position position);

        int getId() const;

        Position getPosition() const;

        std::optional<int> getOrderId() const;

        AgentStatus getStatus() const;

        void moveTo(Position position); // update agent pos

        void assignOrder(int orderID); // store order ID, change from Idle to PickupPending

        void pickUp(); // change PickupPending to Delivering

        void completeDelivery(); // clear ordeID, back to idle

        private:
        int ID_;
        Position position_;
        std::optional<int> orderID_;
        AgentStatus status_;
    };
}