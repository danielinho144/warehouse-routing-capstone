#pragma once
#include "warehouse_routing/position.h"
#include <string>
#include <vector>

//order should have 
// ID
// Pickup Position
// Delivery Position
// Status: Unclaimed, Assigned or Completed

namespace warehouse_routing {

    struct OrderLine {
        std::string itemID;
        int reqQuantity;
        int delQuantity = 0;
    };

    enum class OrderStatus {
        Unclaimed,
        Assigned,
        Completed,
        Unfinished
    };

    class Order {
        public:
        Order(int ID, std::vector<OrderLine> lines, Position delivery);

        int getId() const;

        //Position getPickupPosition() const;

        const std::vector<OrderLine>& getLines() const;

        Position getDeliveryPosition() const;

        OrderStatus getStatus() const;

        void markAssigned();

        void markCompleted();

        void markUnfinished();

        void recordDelivery(std::string itemID, int delQuantity);

        private:
        int ID_;
        //Position pickup_;
        Position delivery_;
        OrderStatus status_;
        std::vector<OrderLine> lines_;
    };
}