#pragma once
#include "warehouse_routing/position.h"
#include <string>
#include <vector>

//order stores 
// ID
// Item lines with required and delivered quantities
// Delivery Position
// Status: Unclaimed, Assigned, Completed or Unfinished

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


        const std::vector<OrderLine>& getLines() const;

        Position getDeliveryPosition() const;

        OrderStatus getStatus() const;

        void markAssigned();

        void markCompleted();

        void markUnfinished();

        void recordDelivery(std::string itemID, int delQuantity);

        private:
        int ID_;
        Position delivery_;
        OrderStatus status_;
        std::vector<OrderLine> lines_;
    };
}