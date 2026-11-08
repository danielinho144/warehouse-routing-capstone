#pragma once
#include <string>
#include "warehouse_routing/position.h"

namespace warehouse_routing {
    //contents of one storage cell
    //different cells can hold same itemID, allows agent to find different fulfillment location
    class InventoryCell {
        public:
            InventoryCell(std::string itemID, 
                int quantity, Position shelfPosition, 
                Position pickupPosition);
                
            const std::string& getItemID() const;
            int getQuantity() const;
            Position getShelfPosition() const;
            Position getPickupPosition() const;

            void removeStock(int quantity);

        private:
            std::string itemID_;
            int quantity_;
            Position shelfPosition_;
            Position pickupPosition_;
    };
}