#include "warehouse_routing/independent_planner.h"
#include "warehouse_routing/astar.h"
#include <stdexcept>

namespace warehouse_routing {
    std::unordered_map<int, std::vector<int>> planIndependentPaths(const Graph& graph, const std::vector<Agent>& agents, const std::unordered_map<int, int>& goals) {
        std::unordered_map<int, std::vector<int>> paths;
        for (const Agent& agent : agents) {
            //read agent ID and position
            int agentID = agent.getId();
            Position position = agent.getPosition();
            //convert position to startNode ID
            int startID = graph.toNodeId(position.row, position.col);
            //look up goal
            if (!goals.contains(agentID)) {
                throw std::runtime_error("Goals does not contain agent ID");
            }
            //run A* n store path
            int goalID = goals.at(agentID);
            paths[agentID] = aStar(graph, startID, goalID);
        }
        return paths;
    }
}