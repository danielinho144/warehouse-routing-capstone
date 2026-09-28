#include <gtest/gtest.h>
#include "warehouse_routing/grid.h"
#include <stdexcept>

using namespace warehouse_routing;

/*
TEST(GridTest, DefaultsToFreeCells) {
    Grid grid(5, 5);

    EXPECT_EQ(grid.getRows(), 5);
    EXPECT_EQ(grid.getCols(), 5);
    EXPECT_TRUE(grid.isFree(0, 0));
    EXPECT_TRUE(grid.isFree(4, 4));
}

*/

TEST(GridTest, SetCellTypeMarksShelf) {
    Grid grid(3, 5);

    grid.setCellType(2, 3, CellType::Shelf);

    EXPECT_EQ(grid.getCellType(2, 3), CellType::Shelf);
    EXPECT_FALSE(grid.isFree(2, 3));

    // A neighboring cell should be untouched.
    EXPECT_TRUE(grid.isFree(2, 2));
}

TEST(GridTest, NewGridIsAllFree) {
    Grid grid(3,4);
    EXPECT_EQ(grid.getRows(), 3);
    EXPECT_EQ(grid.getCols(), 4);
    EXPECT_TRUE(grid.isFree(0, 0));
    EXPECT_TRUE(grid.isFree(2, 3));
}

TEST(GridTest, GetCellTypeThrowsOutOfBounds) {
    Grid grid(3, 4);
    EXPECT_THROW(grid.getCellType(-1, 0), std::out_of_range);
    EXPECT_THROW(grid.getCellType(3, 0), std::out_of_range);   // row == rows
    EXPECT_THROW(grid.getCellType(0, 4), std::out_of_range);   // col == cols
}

TEST(GridTest, IsInBoundsChecksEveryEdge) {
    Grid grid(3, 4);

    // the four corners are inside
    EXPECT_TRUE(grid.isInBounds(0, 0));
    EXPECT_TRUE(grid.isInBounds(0, 3));
    EXPECT_TRUE(grid.isInBounds(2, 0));
    EXPECT_TRUE(grid.isInBounds(2, 3));

    // one step past each edge is outside
    EXPECT_FALSE(grid.isInBounds(-1, 0));
    EXPECT_FALSE(grid.isInBounds(3, 0));
    EXPECT_FALSE(grid.isInBounds(0, -1));
    EXPECT_FALSE(grid.isInBounds(0, 4));
}

TEST(GridTest, IsFreeReturnsFalseOutOfBounds) {
    Grid grid(3, 4);
    EXPECT_FALSE(grid.isFree(-1, 0));
    EXPECT_FALSE(grid.isFree(3, 0));
    EXPECT_FALSE(grid.isFree(0, 4));
}

TEST(GridTest, SetCellTypeThrowsOutOfBounds) {
    Grid grid(3, 4);
    EXPECT_THROW(grid.setCellType(-1, 0, CellType::Free), std::out_of_range);
    EXPECT_THROW(grid.setCellType(3, 0, CellType::PickStation), std::out_of_range);   // row == rows
    EXPECT_THROW(grid.setCellType(0, 4, CellType::Shelf), std::out_of_range);   // col == cols
}

TEST(GridTest, PickStationNotFree) {
    Grid grid (3,4);

    grid.setCellType(2, 3, CellType::PickStation);
    EXPECT_FALSE(grid.isFree(2,3));
    EXPECT_EQ(grid.getCellType(2,3), CellType::PickStation);
}