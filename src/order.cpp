#include "warehouse_routing/order.h"

namespace warehouse_routing {
    Order::Order(int ID, Position pickup, Position delivery) {
        ID_ = ID;
        pickup_ = pickup;
        delivery_ = delivery;
        status_ = OrderStatus::Unclaimed;
    }

    //get ID
    int Order::getId() const {
        return ID_;
    }

    //get pickupPosition
    Position Order::getPickupPosition() const {
        return pickup_;
    }

    //get deliveryposition
    Position Order::getDeliveryPosition() const {
        return delivery_;
    }

    //get status
    OrderStatus Order::getStatus() const {
        return status_;
    }
}