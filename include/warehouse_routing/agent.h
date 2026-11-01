#pragma once
#include <optional>
#include "warehouse_routing/position.h"
#include <string>
#include <vector>

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

    struct CarriedItem {
        std::string itemID;
        int quantity;
    };

    class Agent {
        public:
        Agent(int ID, Position position, int capacity);

        int getId() const;

        Position getPosition() const;

        std::optional<int> getOrderId() const;

        AgentStatus getStatus() const;

        void moveTo(Position position); // update agent pos

        void assignOrder(int orderID); // store order ID, change from Idle to PickupPending

        void startDelivery(); // begin delivering a nonempty load

        void completeDelivery(bool needsAnotherTrip); // finish unloading; continue or release order

        const std::vector<CarriedItem>& getCarriedItems() const;

        int getCapacity() const;

        int getCurrentLoad() const;

        int getRemainingCapacity() const;

        void addCarriedItems(std::string itemID, int quantity);

        void removeCarriedItems(std::string itemID, int quantity);

        void releaseOrder();


        private:
        int ID_;
        Position position_;
        std::optional<int> orderID_;
        AgentStatus status_;
        std::vector<CarriedItem> carriedItems_;
        int capacity_;
    };
}
