#include "warehouse_routing/inventory.h"
#include <stdexcept>

namespace warehouse_routing {
    InventoryCell::InventoryCell(std::string itemID,
        int quantity, Position shelfPosition,
        Position pickupPosition) {
            itemID_ = itemID;
            quantity_ = quantity;
            shelfPosition_ = shelfPosition;
            pickupPosition_ = pickupPosition;

            if (itemID_.empty()) {
                throw std::invalid_argument("Item ID can't be empty");
            }

            if (quantity_ < 0) {
                throw std::invalid_argument("Inventory quantity can't be negative");
            }

            if (shelfPosition_.row < 0 || shelfPosition_.col < 0 ||
                pickupPosition_.row < 0 || pickupPosition_.col < 0) {
                    throw std::invalid_argument("Inventory positions can't be negative");
                }
            if (shelfPosition_.row == pickupPosition_.row &&
                shelfPosition_.col == pickupPosition_.col) {
                    throw std::invalid_argument("Shelf and pickup positions must be different");
                }
        }
    
    const std::string& InventoryCell::getItemID() const {
        return itemID_;
    }

    int InventoryCell::getQuantity() const {
        return quantity_;
    }

    Position InventoryCell::getShelfPosition() const {
        return shelfPosition_;
    }

    Position InventoryCell::getPickupPosition() const {
        return pickupPosition_;
    }

    void InventoryCell::removeStock(int quantity) {
        if (quantity <= 0) {
            throw std::invalid_argument("Removal quantity must be positive");
        }
        if (quantity > quantity_) {
            throw std::runtime_error("Not enough stock at this cell");
        }
        quantity_ -= quantity;
    }
}