#include <gtest/gtest.h>
#include <stdexcept>
#include "warehouse_routing/inventory.h"

using namespace warehouse_routing;

TEST(InventoryTest, ConstructionPreservesItemQuantityAndPositions) {
    const InventoryCell inventory("SKU_A", 30, Position{5, 35}, Position{5, 36});

    EXPECT_EQ(inventory.getItemID(), "SKU_A");
    EXPECT_EQ(inventory.getQuantity(), 30);
    EXPECT_EQ(inventory.getShelfPosition().row, 5);
    EXPECT_EQ(inventory.getShelfPosition().col, 35);
    EXPECT_EQ(inventory.getPickupPosition().row, 5);
    EXPECT_EQ(inventory.getPickupPosition().col, 36);
}

TEST(InventoryTest, ZeroInitialStockIsAllowed) {
    const InventoryCell inventory("SKU_A", 0, Position{0, 0}, Position{0, 1});
    EXPECT_EQ(inventory.getQuantity(), 0);
}

TEST(InventoryTest, EmptyItemIDIsRejected) {
    EXPECT_THROW((InventoryCell("", 10, Position{1, 1}, Position{1, 2})),
                 std::invalid_argument);
}

TEST(InventoryTest, NegativeInitialQuantityIsRejected) {
    EXPECT_THROW((InventoryCell("SKU_A", -1, Position{1, 1}, Position{1, 2})),
                 std::invalid_argument);
}

TEST(InventoryTest, NegativeCoordinatesAreRejected) {
    EXPECT_THROW((InventoryCell("SKU_A", 10, Position{-1, 1}, Position{1, 2})),
                 std::invalid_argument);
    EXPECT_THROW((InventoryCell("SKU_A", 10, Position{1, -1}, Position{1, 2})),
                 std::invalid_argument);
    EXPECT_THROW((InventoryCell("SKU_A", 10, Position{1, 1}, Position{-1, 2})),
                 std::invalid_argument);
    EXPECT_THROW((InventoryCell("SKU_A", 10, Position{1, 1}, Position{1, -1})),
                 std::invalid_argument);
}

TEST(InventoryTest, IdenticalShelfAndPickupPositionsAreRejected) {
    EXPECT_THROW((InventoryCell("SKU_A", 10, Position{1, 1}, Position{1, 1})),
                 std::invalid_argument);
}

TEST(InventoryTest, RepeatedPickupsReduceRemainingStock) {
    InventoryCell inventory("SKU_A", 10, Position{1, 1}, Position{1, 2});

    inventory.removeStock(3);
    EXPECT_EQ(inventory.getQuantity(), 7);
    inventory.removeStock(2);
    EXPECT_EQ(inventory.getQuantity(), 5);
    inventory.removeStock(5);
    EXPECT_EQ(inventory.getQuantity(), 0);
}

TEST(InventoryTest, NonPositiveRemovalLeavesStockUnchanged) {
    InventoryCell inventory("SKU_A", 10, Position{1, 1}, Position{1, 2});

    EXPECT_THROW(inventory.removeStock(0), std::invalid_argument);
    EXPECT_EQ(inventory.getQuantity(), 10);
    EXPECT_THROW(inventory.removeStock(-3), std::invalid_argument);
    EXPECT_EQ(inventory.getQuantity(), 10);
}

TEST(InventoryTest, ExcessiveRemovalLeavesRemainingStockUnchanged) {
    InventoryCell inventory("SKU_A", 10, Position{1, 1}, Position{1, 2});
    inventory.removeStock(6);

    EXPECT_THROW(inventory.removeStock(5), std::runtime_error);
    EXPECT_EQ(inventory.getQuantity(), 4);
    inventory.removeStock(4);
    EXPECT_EQ(inventory.getQuantity(), 0);
}

TEST(InventoryTest, EmptyCellCannotSupplyStock) {
    InventoryCell inventory("SKU_A", 0, Position{1, 1}, Position{1, 2});

    EXPECT_THROW(inventory.removeStock(1), std::runtime_error);
    EXPECT_EQ(inventory.getQuantity(), 0);
}

TEST(InventoryTest, CellsHoldingSameItemHaveIndependentStock) {
    InventoryCell first("SKU_A", 10, Position{1, 1}, Position{1, 2});
    InventoryCell second("SKU_A", 20, Position{3, 1}, Position{3, 2});

    first.removeStock(4);
    EXPECT_EQ(first.getQuantity(), 6);
    EXPECT_EQ(second.getQuantity(), 20);
}
