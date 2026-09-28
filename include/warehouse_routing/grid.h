#pragma once
#include <vector>

//everything in project lives in warehouse_routing
//so no collision with library code or anything else later
namespace warehouse_routing {
    //three possible states a single grid cell can be in
    enum class CellType { 
        Free, //walkable, will be graph node
        Shelf, //obstacle, never becomes graph node
        PickStation // not walkable, pickup/dropoff locations
    };


    //2d layout of cells
    //warehouse layout
    class Grid {
        public:
        //create grid of given size
        //all cells default to Free
        Grid(int rows, int cols); 

        //return number of rows in grid
        int getRows() const;

        //returns numbers of columns in grid
        int getCols() const;

        //return CellType stored at (row,col)
        //throws std::out_of_range if (row,col) not in bounds
        CellType getCellType(int row, int col) const;

        //check if cell at (row,col) is Free
        //returns false if (row,col) is out of bounds
        bool isFree(int row, int col) const;

        //set CellType at (row,col)
        //throws std::out_of_range if (row,col) not in bounds
        void setCellType(int row, int col, CellType type);

        //true if (row,col) is inside the grid
        bool isInBounds(int row, int col) const;

        private:
        int rows_;
        int cols_;

        //stored as a vector of rows, each row is its own vector of cells
        //cells_[row][col] maps directly to (row,col) coordinate
        std::vector<std::vector<CellType>> cells_;
    };
}