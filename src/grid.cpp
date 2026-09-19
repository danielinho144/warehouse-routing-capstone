#include "warehouse_routing/grid.h"

namespace warehouse_routing {

    //build grid of given size, every cell is Free
    //Shelf and stations get added after
    Grid::Grid(int rows, int cols) {
        rows_ = rows;
        cols_ = cols;
        //build 'rows' rows, each one a vector of 'cols' Free cells
        cells_ = vector<vector<CellType>>(rows, vector<CellType>(cols, CellType::Free));
    }

    //gets rows
    int Grid::getRows() const {
        return rows_;
    }

    //gets cols
    int Grid::getCols() const {
        return cols_;
    }


    //return celltype at (row,col)
    CellType Grid::getCellType(int row, int col) const {
        return cells_[row][col];
    }

    //convenience check on top of getCellType
    //check if cell == Free
    bool Grid::isFree(int row, int col) const {
        return getCellType(row,col) == CellType::Free;
    }

    //write CellType at given position
    //will be used to make shelves/stations when building warehouse layout
    void Grid::setCellType(int row, int col, CellType type) {
        cells_[row][col] = type;
    }


}