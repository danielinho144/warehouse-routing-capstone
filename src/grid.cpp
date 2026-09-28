#include "warehouse_routing/grid.h"
#include <stdexcept>



namespace warehouse_routing {

    //build grid of given size, every cell is Free
    //Shelf and stations get added after
    Grid::Grid(int rows, int cols) {
        rows_ = rows;
        cols_ = cols;
        //build 'rows' rows, each one a vector of 'cols' Free cells
        cells_ = std::vector<std::vector<CellType>>(rows, std::vector<CellType>(cols, CellType::Free));
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
        if (!isInBounds(row, col))
            throw std::out_of_range("Grid: position out of bounds");
        return cells_[row][col];
    }

    //convenience check on top of getCellType
    //check if cell == Free
    bool Grid::isFree(int row, int col) const {
        //return getCellType(row,col) == CellType::Free;
        return isInBounds(row, col) && getCellType(row, col) == CellType::Free;
    }

    //write CellType at given position
    //will be used to make shelves/pickstations when building warehouse layout
    void Grid::setCellType(int row, int col, CellType type) {
        if (!isInBounds(row, col))
            throw std::out_of_range("Grid: position out of bounds");
        cells_[row][col] = type;
    }

    bool Grid::isInBounds(int row, int col) const {
        return row >= 0 && row < rows_ && col >= 0 && col < cols_;
    }

}