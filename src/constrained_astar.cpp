#include "warehouse_routing/constrained_astar.h"
#include <queue>
#include <tuple>
#include <functional>
#include <algorithm>
#include <cstdlib>
#include <utility>
#include <set>
#include <map>

namespace warehouse_routing {
    std::vector<int> constrainedAStar(const Graph& graph, int agentID,
        int startID, int goalID, 
        const std::vector<constraintInfo>& constraints, 
        int maxTick) {
        if (!graph.hasNode(startID) || !graph.hasNode(goalID)) {
            return {};
        }

        if (maxTick < 0) {
            return {};
        }

        for (const constraintInfo& constraint : constraints) {
            if (constraint.agentID == agentID && 
            constraint.type == ConstraintType::Vertex &&
            constraint.tick == 0 && constraint.toNode == startID) {
                return {};
            }
        }
        using QueueEntry = std::tuple<int, int, int>; // f score, node ID, tick
        std::priority_queue<QueueEntry, std::vector<QueueEntry>, std::greater<QueueEntry>> nodesToExplore;
        int nodeID = startID;
        int tick = 0;
        int row_difference = std::abs(graph.rowOf(startID) - graph.rowOf(goalID));
        int col_difference = std::abs(graph.colOf(startID) - graph.colOf(goalID));
        int estimatedCost = std::max(row_difference,  col_difference);
        nodesToExplore.push({estimatedCost, nodeID, tick});

        using State = std::pair<int, int>;
        State state = {nodeID, tick};
        std::set<State> visited;
        std::map<State, State> cameFrom;
        visited.insert(state);

        while (!nodesToExplore.empty()) {
            auto [f, currentID, currentTick] = nodesToExplore.top();
            nodesToExplore.pop();
            State currentState = {currentID, currentTick};

            if (currentID == goalID) {
                bool canStayAtGoal = true;
                for (const constraintInfo& constraint : constraints) {
                    if (constraint.agentID != agentID) {
                        continue;
                    }
                    if (constraint.tick >= currentTick) {
                        if (constraint.type == ConstraintType::Vertex && constraint.toNode == goalID) {
                            canStayAtGoal = false;
                            break;
                        }
                        if (constraint.type == ConstraintType::Edge &&
                        constraint.fromNode == goalID && constraint.toNode == goalID) {
                            canStayAtGoal = false;
                            break;
                        }
                    }

                }

                if (canStayAtGoal) {
                    std::vector<int> path;
                    State trace = currentState;

                    while (true) {
                        path.push_back(trace.first);

                        if (trace.first == startID && trace.second == 0) {
                            break;
                        }
                        trace = cameFrom.at(trace);
                    }
                    std::reverse(path.begin(), path.end());
                    return path;
                }
            }

            if (currentTick >= maxTick) {
                continue;
            }

            std::vector<int> possibleMoves = graph.getNeighbors(currentID);
            possibleMoves.push_back(currentID);

            for (int nextID : possibleMoves) {
                int nextTick = currentTick + 1;
                bool blocked = false;

                for (const constraintInfo& constraint : constraints) {
                    if (constraint.agentID != agentID) {
                        continue;
                    }

                    if (constraint.type == ConstraintType::Vertex) {
                        if (constraint.tick == nextTick && constraint.toNode == nextID) {
                            blocked = true;
                            break;
                        }
                    }

                    if (constraint.type == ConstraintType::Edge) {
                        if (constraint.tick == currentTick && constraint.fromNode == currentID && constraint.toNode == nextID) {
                            blocked = true;
                            break;
                        }
                    }
                }
                if (blocked) {
                    continue;
                }
                State nextState = {nextID, nextTick};

                if (visited.contains(nextState)) {
                    continue;
                }
                visited.insert(nextState);
                cameFrom[nextState] = currentState;

                int row_difference2 = std::abs(graph.rowOf(nextID) - graph.rowOf(goalID));
                int col_difference2 = std::abs(graph.colOf(nextID) - graph.colOf(goalID));
                int estimatedCost2 = std::max(row_difference2,  col_difference2);
                nodesToExplore.push({nextTick + estimatedCost2, nextID, nextTick});
            }
        }
        return {};
    }

}
