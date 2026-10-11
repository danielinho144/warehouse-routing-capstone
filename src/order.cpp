#include "warehouse_routing/order.h"
#include <stdexcept>

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

    //mark order assigned
    void Order::markAssigned() {
        //do stuff
        if (status_ != OrderStatus::Unclaimed) {
            throw std::runtime_error("Cannot assign");
        }
        status_ = OrderStatus::Assigned;
    }

    //mark order completed
    void Order::markCompleted() {
        //do stuff
        if (status_ != OrderStatus::Assigned) {
            throw std::runtime_error("Cannot complete");
        }
        status_ = OrderStatus::Completed;
    }
}