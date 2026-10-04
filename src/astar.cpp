#include "warehouse_routing/astar.h"
#include <queue>
#include <unordered_map>
#include <algorithm>
#include <cstdlib>
#include <tuple>
#include <functional>

namespace warehouse_routing {
    int chebyshevDistance(const Graph& graph, int currentID, int goalID) {
        int row_difference = std::abs(graph.rowOf(currentID) - graph.rowOf(goalID));
        int col_difference = std::abs(graph.colOf(currentID) - graph.colOf(goalID));
        return std::max(row_difference, col_difference);
    }
    
    //creates short name for type
    //QueueEntry means tuple containing three int
    using QueueEntry = std::tuple<int, int, int>; //f, nodeID, g

    std::vector<int> aStar(const Graph& graph, int startID, int goalID) {
        if (!graph.hasNode(startID) || !graph.hasNode(goalID))
            return {};
        if (startID == goalID)
            return {startID};
        
        std::priority_queue<QueueEntry, std::vector<QueueEntry>, std::greater<QueueEntry>> nodesToExplore;
        std::unordered_map<int, int> gScore; //maps nodeID to cheapest cost found so far
        std::unordered_map<int, int> cameFrom; //maps a nodeID to its predecessor, reconstruct path

        gScore[startID] = 0;
        nodesToExplore.push({chebyshevDistance(graph, startID, goalID), startID, 0});
        while (!nodesToExplore.empty()) {
            auto [f, currentID, currentG] = nodesToExplore.top();
            nodesToExplore.pop();

            if (currentG > gScore.at(currentID))
                continue;
            
            if (currentID == goalID) {
                //reconstruct and return path
                std::vector<int> path;
                int node = goalID;
                path.push_back(node);

                while (node != startID) {
                    node = cameFrom.at(node);
                    path.push_back(node);
                }
                std::reverse(path.begin(), path.end());
                return path;
            }
            for (int neighbor : graph.getNeighbors(currentID)) {
                int tentativeCost = currentG + 1;

                if (!gScore.contains(neighbor) || tentativeCost < gScore.at(neighbor)) {
                    //record this better route
                    cameFrom[neighbor] = currentID;
                    gScore[neighbor] = tentativeCost;
                    nodesToExplore.push({tentativeCost + chebyshevDistance(graph, neighbor, goalID), neighbor, tentativeCost});
                }
            }
        }
        return {};
    }
}