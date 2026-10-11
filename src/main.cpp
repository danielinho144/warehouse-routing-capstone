#include "warehouse_routing/scenario_loader.h"
#include "warehouse_routing/simulation.h"
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
    int maxTicks = std::stoi(argv[2]);
    while (!sim.isFinished() && sim.getCurrentTick() < maxTicks) {
        sim.step();

        if (sim.getCurrentTick() % 100 == 0) {
            std::cout << "Progress: tick " << sim.getCurrentTick() << "/" << maxTicks
                      << " | Completed orders: " << sim.getCompletedOrderCount()
                      << "/" << sim.getOrders().size() << std::endl;
        }
    }

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
