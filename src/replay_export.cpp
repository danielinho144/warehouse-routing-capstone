#include "warehouse_routing/replay_export.h"
#include <fstream>
#include <stdexcept>

namespace warehouse_routing {

    void recordSnapshot(const Simulation& simulation, nlohmann::json& replay) {
        nlohmann::json snapshot;

        snapshot["tick"] = simulation.getCurrentTick();
        snapshot["completed_orders"] = simulation.getCompletedOrderCount();
        snapshot["throughput"] = simulation.getThroughput();
        snapshot["agents"] = nlohmann::json::array();

        for (const Agent& agent : simulation.getAgents()) {
            nlohmann::json agentData;
            Position position = agent.getPosition();

            agentData["id"] = agent.getId();
            agentData["row"] = position.row;
            agentData["col"] = position.col;

            if (agent.getStatus() == AgentStatus::Idle) {
            agentData["status"] = "idle";
            }
            else if (agent.getStatus() == AgentStatus::PickupPending) {
                agentData["status"] = "pickup_pending";
            }
            else {
                agentData["status"] = "delivering";
            }

            if (agent.getOrderId().has_value()) {
                agentData["order_id"] = agent.getOrderId().value();
            }
            else {
                agentData["order_id"] = nullptr;
            }
            snapshot["agents"].push_back(agentData);
        }
        replay["snapshots"].push_back(snapshot);
    }

    void saveReplay(const nlohmann::json& replay, const std::string& filePath) {
        std::ofstream file(filePath);
        if (!file.is_open()) {
            throw std::runtime_error("Could not open replay file: " + filePath);
        }
        file << replay.dump(2);
        file.close();

        if(!file) {
            throw std::runtime_error("Could not write replay file: " + filePath);
        }
    }
}