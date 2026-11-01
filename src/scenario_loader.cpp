#include "warehouse_routing/scenario_loader.h"
#include <fstream>
#include <stdexcept>
#include <nlohmann/json.hpp>
#include <filesystem>
#include "warehouse_routing/map_loader.h"
#include <set>
#include <utility>
#include <cstdlib>

namespace warehouse_routing {
    Scenario loadScenario(const std::string& filePath) {
        std::ifstream file(filePath);

        if (!file.is_open()) {
            throw std::runtime_error("Could not open scenario: " + filePath);
        }
        nlohmann::json data;
        file >> data;

        Scenario scenario;
        scenario.mapPath = data.at("map_path").get<std::string>();
        
        std::filesystem::path scenarioPath(filePath); //create path from scenario filename
        std::filesystem::path mapPath = scenarioPath.parent_path() / scenario.mapPath; //get folder containing file and join path
        scenario.mapPath = mapPath.lexically_normal().string(); //simplify pieces into warehouses
        Grid grid = loadMap(scenario.mapPath); // load warehouse to check positions against it

        std::set<int> agentIds;
        std::set<std::pair<int, int>> agentStarts;
        std::set<std::pair<int,int>> shelfPositions;

        if (data.contains("inventory")) {
            if (!data.at("inventory").is_array()) {
                throw std::runtime_error("Inventory must be array");
            }
            for (const auto& entry : data.at("inventory")) {
                std::string item_id = entry.at("item_id").get<std::string>();
                int quantity = entry.at("quantity").get<int>();

                int shelf_row = entry.at("shelf").at("row").get<int>();
                int shelf_col = entry.at("shelf").at("col").get<int>();
                if (!grid.isInBounds(shelf_row, shelf_col)) {
                    throw std::runtime_error("Inventory shelf position is outside map");
                }
                if (grid.getCellType(shelf_row, shelf_col) != CellType::Shelf) {
                    throw std::runtime_error("Inventory storage position must be shelf");
                }

                if (shelfPositions.contains({shelf_row, shelf_col})) {
                    throw std::runtime_error("Duplicate inventory shelf positions");
                }
                shelfPositions.insert({shelf_row, shelf_col});

                int pickup_row = entry.at("pickup").at("row").get<int>();
                int pickup_col = entry.at("pickup").at("col").get<int>();
                if (!grid.isFree(pickup_row, pickup_col)) {
                    throw std::runtime_error("Pickup cell must be walkable");
                }

                int abs_diff_row = std::abs(shelf_row - pickup_row);
                int abs_diff_col = std::abs(shelf_col - pickup_col);

                if (abs_diff_row + abs_diff_col != 1) {
                    throw std::runtime_error("No cardinal adjacency");
                }
                InventoryCell s(item_id, quantity, Position {shelf_row, shelf_col}, Position {pickup_row, pickup_col});
                scenario.inventory.push_back(s);
            }
        }
        
        for (const auto& entry : data.at("agents")) {
            int id = entry.at("id").get<int>();

            if (agentIds.contains(id)) //check if ID is already present
                throw std::runtime_error("Duplicate agent ID");
            agentIds.insert(id); //record ID

            int start_row = entry.at("start").at("row").get<int>();
            int start_col = entry.at("start").at("col").get<int>();

            if (agentStarts.contains({start_row, start_col}))
                throw std::runtime_error("Duplicate agent start");
            agentStarts.insert({start_row, start_col});

            if (!grid.isFree(start_row, start_col))
                throw std::runtime_error("Agent start must be on free cell");

            int capacity = entry.at("capacity").get<int>();

            Agent s(id, Position{start_row, start_col}, capacity);
            scenario.agents.push_back(s);
        }

        std::set<int>orderIds;

        for (const auto& entry :data.at("orders")) {
            int id = entry.at("id").get<int>();

            if (orderIds.contains(id)) // check if ID is already present
                throw std::runtime_error("Duplicate order ID");
            orderIds.insert(id); //record ID

            //int pickup_row = entry.at("pickup").at("row").get<int>();
            //int pickup_col = entry.at("pickup").at("col").get<int>();

            //if (!grid.isFree(pickup_row, pickup_col)) //make sure pickup position is valid
                //throw std::runtime_error("Order pickup must be on free cell");
            std::vector<OrderLine> lines;

            if (!entry.at("lines").is_array()) {
                throw std::invalid_argument("Lines isn't an array");
            }
            for (const auto& lineEntry : entry.at("lines")) {
                std::string itemID = lineEntry.at("item_id").get<std::string>();
                int quantity = lineEntry.at("quantity").get<int>();

                OrderLine line = {itemID, quantity};
                lines.push_back(line);
            }

            int delivery_row = entry.at("delivery").at("row").get<int>();
            int delivery_col = entry.at("delivery").at("col").get<int>();

            if (!grid.isFree(delivery_row, delivery_col)) // make sure delivery position is valid
                throw std::runtime_error("Order delivery must be on free cell");

            Order o(id, lines, Position{delivery_row, delivery_col});
            scenario.orders.push_back(o);
        }
        return scenario;
    }
}