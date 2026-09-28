#include <gtest/gtest.h>
#include <stdexcept>
#include <vector>
#include "warehouse_routing/grid.h"
#include "warehouse_routing/graph.h"
#include "warehouse_routing/map_loader.h"

using namespace warehouse_routing;

TEST(MapLoaderTest, Dimensions) {
    Grid grid = loadMap("/Users/danilopez/Desktop/warehouse-routing-capstone/data/warehouses/test_cases/test_layout_small.txt");
    EXPECT_EQ(grid.getRows(), 2);
    EXPECT_EQ(grid.getCols(), 3);
    EXPECT_FALSE(grid.isFree(0, 1));
    EXPECT_FALSE(grid.isFree(1, 2));
    EXPECT_TRUE(grid.isFree(0, 0));

}

TEST(MapLoaderTest, MissingFile) {
    EXPECT_THROW(loadMap("something.txt"), std::runtime_error);
}

TEST(MapLoaderTest, InvalidType) {
    EXPECT_THROW(loadMap("/Users/danilopez/Desktop/warehouse-routing-capstone/data/warehouses/test_cases/invalid_header.txt"), std::runtime_error);
}

TEST(MapLoaderTest, ExtraContent) {
    EXPECT_THROW(loadMap("/Users/danilopez/Desktop/warehouse-routing-capstone/data/warehouses/test_cases/extra_content.txt"), std::runtime_error);
}

TEST(MapLoaderTest, InvalidHeight) {
    EXPECT_THROW(loadMap("/Users/danilopez/Desktop/warehouse-routing-capstone/data/warehouses/test_cases/invalid_height.txt"), std::runtime_error);
}

TEST(MapLoaderTest, InvalidMapMarker) {
    EXPECT_THROW(loadMap("/Users/danilopez/Desktop/warehouse-routing-capstone/data/warehouses/test_cases/invalid_map_marker.txt"), std::runtime_error);
}

TEST(MapLoaderTest, InvalidWidth) {
    EXPECT_THROW(loadMap("/Users/danilopez/Desktop/warehouse-routing-capstone/data/warehouses/test_cases/invalid_width.txt"), std::runtime_error);
}

TEST(MapLoaderTest, MissingRow) {
    EXPECT_THROW(loadMap("/Users/danilopez/Desktop/warehouse-routing-capstone/data/warehouses/test_cases/missing_row.txt"), std::runtime_error);
}

TEST(MapLoaderTest, NonNumHeight) {
    EXPECT_THROW(loadMap("/Users/danilopez/Desktop/warehouse-routing-capstone/data/warehouses/test_cases/non_num_height.txt"), std::runtime_error);
}

TEST(MapLoaderTest, RowWidthMismatch) {
    EXPECT_THROW(loadMap("/Users/danilopez/Desktop/warehouse-routing-capstone/data/warehouses/test_cases/row_width_mismatch.txt"), std::runtime_error);
}

TEST(MapLoaderTest, UnsupportedCharacter) {
    EXPECT_THROW(loadMap("/Users/danilopez/Desktop/warehouse-routing-capstone/data/warehouses/test_cases/unsupported_character.txt"), std::runtime_error);
}

TEST(MapLoaderTest, TrailingWhiteSpaceAllowed) {
    Grid grid = loadMap("/Users/danilopez/Desktop/warehouse-routing-capstone/data/warehouses/test_cases/trailing_whitespace_allowed.txt");
    EXPECT_EQ(grid.getRows(), 2);
    EXPECT_EQ(grid.getCols(), 3);
}