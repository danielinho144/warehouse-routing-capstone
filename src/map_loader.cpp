#include "warehouse_routing/map_loader.h"
#include <fstream>
#include <stdexcept>

namespace warehouse_routing {

    Grid loadMap(const std::string& filePath) {
    std::ifstream file(filePath);

    if (!file.is_open()) {
        throw std::runtime_error("Could not open map: " + filePath);
    }
    std::string label;
    std::string type;
    int height;
    int width;
    std::string map;


    // Next: read the header, create the Grid, and fill its cells.
    if (!(file >> label >> type)) {
        //reading failed
        throw std::runtime_error("Reading failed");
    } 
    else if (label != "type" || type != "octile") {
        throw std::runtime_error("Expected 'type octile' in map header");   
    }


    if (!(file >> label >> height)) {
        //reading failed
        throw std::runtime_error("Reading failed");
    }
    else if (label != "height" || height <= 0) {
        throw std::runtime_error("Expected 'Height' component");
    }
    

    if (!(file >> label >> width)) {
        //reading failed
        throw std::runtime_error("Reading failed");
    }
    else if (label != "width" || width <= 0) {
        throw std::runtime_error("Expected 'Width' component");
    }

    if (!(file >> map)) {
        //reading failed
        throw std::runtime_error("Reading Failed");
    }
    else if(map != "map") {
        throw std::runtime_error("Expected Map");
    }

    //create grid
    Grid s = Grid(height, width);
    std::string mapLine;
    for (int row = 0; row < height; row++) {
        //read map line into string
        if (!(file >> mapLine)) {
            throw std::runtime_error("Mapline read failed");
        }
        if (mapLine.length() != width) {
            throw std::runtime_error("Width does not match");
        }

        for (int col = 0; col < width; col++) {
            if (mapLine[col] == '.') {
                s.setCellType(row, col, CellType::Free); 
            }
            else if (mapLine[col] == 'T') {
                s.setCellType(row, col, CellType::Shelf);
            }
            else {
                throw std:: runtime_error("Unsupported Character");
            }
        } // end col loop
    } // end row loop

    std::string extra;
    if (file >> extra)
        throw std::runtime_error("Unexpected content after map rows");
    
    return s;
    }
}