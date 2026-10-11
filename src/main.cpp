#include "warehouse_routing/map_loader.h"
#include "warehouse_routing/scenario_loader.h"
#include "warehouse_routing/simulation.h"
#include "warehouse_routing/replay_export.h"
#include <iostream>
#include <string>

using namespace warehouse_routing;

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: warehouse_sim<scenario_path> <max_ticks>\n";
        return 1;
    }

    const Scenario scenario = loadScenario(argv[1]);;
    Simulation sim(scenario);
    nlohmann::json replay;

    Grid grid = loadMap(scenario.mapPath);
    replay["map"]["rows"] = grid.getRows();
    replay["map"]["cols"] = grid.getCols();
    replay["map"]["cells"] = nlohmann::json::array();

    for (int row = 0; row < grid.getRows(); row++) {
        std::string mapRow;
        for (int col = 0; col < grid.getCols(); col++) {
            if (grid.isFree(row, col)) {
                mapRow += '.';
            }
            else {
                mapRow += 'T';
            }
        }
        replay["map"]["cells"].push_back(mapRow);
    }

    replay["orders"] = nlohmann::json::array();
    for (const Order& order : scenario.orders) {
        nlohmann::json orderData;
        Position pickup = order.getPickupPosition();
        Position delivery = order.getDeliveryPosition();

        orderData["id"] = order.getId();
        orderData["pickup"]["row"] = pickup.row;
        orderData["pickup"]["col"] = pickup.col;
        orderData["delivery"]["row"] = delivery.row;
        orderData["delivery"]["col"] = delivery.col;

        replay["orders"].push_back(orderData);
    }

    replay["snapshots"] = nlohmann::json::array();
    recordSnapshot(sim, replay);

    int maxTicks = std::stoi(argv[2]);
    while (!sim.isFinished() && sim.getCurrentTick() < maxTicks) {
        sim.step();
        recordSnapshot(sim, replay);

        if (sim.getCurrentTick() % 100 == 0) {
            std::cout << "Progress: tick " << sim.getCurrentTick() << "/" << maxTicks
                      << " | Completed orders: " << sim.getCompletedOrderCount()
                      << "/" << sim.getOrders().size() << std::endl;
        }
    }

    saveReplay(replay, "replay.json");

    std::cout << "Ticks: " << sim.getCurrentTick() << "\n";
    std::cout << "Completed orders: " << sim.getCompletedOrderCount() << "\n";
    std::cout << "Throughput: " << sim.getThroughput() << " orders/tick" << "\n";
    std::cout << "Finished: " << std::boolalpha << sim.isFinished() << "\n";

    for (const Agent& agent : sim.getAgents()) {
        Position position = agent.getPosition();

        std::cout << "Agent " << agent.getId() << ": (" << position.row << ", " << position.col << ")\n";
    }

    return 0;
}
