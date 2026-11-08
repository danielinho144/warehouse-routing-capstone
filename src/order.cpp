#include "warehouse_routing/order.h"
#include <stdexcept>
#include <set>

namespace warehouse_routing {
    Order::Order(int ID, std::vector<OrderLine> lines, Position delivery) {
        ID_ = ID;
        lines_ = lines;
        delivery_ = delivery;
        status_ = OrderStatus::Unclaimed;

        if (lines_.empty()) {
            throw std::invalid_argument("Order needs at least one item line");
        }
        std::set<std::string> itemIDs;

        for (const OrderLine& line : lines_) {
            if (line.itemID.empty()) {
                throw std::invalid_argument("Empty itemIDs");
            }
            if (itemIDs.contains(line.itemID)) {
                throw std::invalid_argument("Duplicate item ID in order");
            }
            if (line.reqQuantity <= 0) {
                throw std::invalid_argument("Required quantity is nonpositive");
            }
            if (line.delQuantity != 0) {
                throw std::invalid_argument("Order already has delivered items");
            }
            itemIDs.insert(line.itemID);
        }
    }

    //get ID
    int Order::getId() const {
        return ID_;
    }


    const std::vector<OrderLine>& Order::getLines() const {
        return lines_;
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
        if (status_ != OrderStatus::Unclaimed) {
            throw std::runtime_error("Cannot assign");
        }
        status_ = OrderStatus::Assigned;
    }

    //mark order completed
    void Order::markCompleted() {
        if (status_ != OrderStatus::Assigned) {
            throw std::runtime_error("Cannot complete");
        }
        for (const OrderLine& line : lines_) {
            if (line.delQuantity != line.reqQuantity) {
                throw std::runtime_error("Order not completed");
            }
        }
        status_ = OrderStatus::Completed;
    }

    void Order::markUnfinished() {
        bool unfinished = false;
        if (status_ != OrderStatus::Assigned && status_ != OrderStatus::Unclaimed) {
            throw std::runtime_error("Order isn't assigned");
        }
        for (const OrderLine& line : lines_) {
            if (line.delQuantity < line.reqQuantity) {
                unfinished = true;
                break;
            }
        }
        if (!unfinished) {
            throw std::runtime_error("Fully delivered order can't be unfinished");
        }
        status_ = OrderStatus::Unfinished;
    }

    void Order::recordDelivery(std::string itemID, int delQuantity) {
        if (status_ != OrderStatus::Assigned) {
            throw std::runtime_error("Order needs to have status Assigned");
        }
        if (delQuantity <= 0) {
            throw std::invalid_argument("Nothing delivered");
        }
        for (OrderLine& line : lines_) {
            if (line.itemID == itemID) {
                int left = line.reqQuantity - line.delQuantity;
                if (left < delQuantity) {
                    throw std::invalid_argument("Exceeds remainder");
                }
                line.delQuantity += delQuantity;
                return;
            }
        }
        throw std::invalid_argument("Unknown itemID");
    }
}