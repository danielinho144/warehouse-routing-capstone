#pragma once
#include "warehouse_routing/position.h"

//order should have 
// ID
// Pickup Position
// Delivery Position
// Status: Unclaimed, Assigned or Completed

namespace warehouse_routing {

    enum class OrderStatus {
        Unclaimed,
        Assigned,
        Completed
    };

    class Order {
        public:
        Order(int ID, Position pickup, Position delivery);

        int getId() const;

        Position getPickupPosition() const;

        Position getDeliveryPosition() const;

        OrderStatus getStatus() const;

        private:
        int ID_;
        Position pickup_;
        Position delivery_;
        OrderStatus status_;
    };
}