#include "warehouse_routing/scenario_loader.h"
#include <fstream>
#include <stdexcept>
#include <nlohmann/json.hpp>
#include <filesystem>
#include "warehouse_routing/map_loader.h"
#include <set>
#include <utility>

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

            Agent s(id, Position{start_row, start_col});
            scenario.agents.push_back(s);
        }

        std::set<int>orderIds;

        for (const auto& entry :data.at("orders")) {
            int id = entry.at("id").get<int>();

            if (orderIds.contains(id)) // check if ID is already present
                throw std::runtime_error("Duplicate order ID");
            orderIds.insert(id); //record ID

            int pickup_row = entry.at("pickup").at("row").get<int>();
            int pickup_col = entry.at("pickup").at("col").get<int>();

            if (!grid.isFree(pickup_row, pickup_col)) //make sure pickup position is valid
                throw std::runtime_error("Order pickup must be on free cell");

            int delivery_row = entry.at("delivery").at("row").get<int>();
            int delivery_col = entry.at("delivery").at("col").get<int>();

            if (!grid.isFree(delivery_row, delivery_col)) // make sure delivery position is valid
                throw std::runtime_error("Order delivery must be on free cell");

            Order o(id, Position{pickup_row, pickup_col}, Position{delivery_row, delivery_col});
            scenario.orders.push_back(o);
        }
        return scenario;
    }
}