#include <gtest/gtest.h>
#include "warehouse_routing/grid.h"

using namespace warehouse_routing;

TEST(GridTest, DefaultsToFreeCells) {
    Grid grid(5, 5);

    EXPECT_EQ(grid.getRows(), 5);
    EXPECT_EQ(grid.getCols(), 5);
    EXPECT_TRUE(grid.isFree(0, 0));
    EXPECT_TRUE(grid.isFree(4, 4));
}

TEST(GridTest, SetCellTypeMarksShelf) {
    Grid grid(5, 5);

    grid.setCellType(2, 3, CellType::Shelf);

    EXPECT_EQ(grid.getCellType(2, 3), CellType::Shelf);
    EXPECT_FALSE(grid.isFree(2, 3));

    // A neighboring cell should be untouched.
    EXPECT_TRUE(grid.isFree(2, 2));
}