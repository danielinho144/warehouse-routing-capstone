#include "warehouse_routing/conflict_detection.h"
#include <cstdlib>
#include "warehouse_routing/agent.h"
#include <algorithm>
#include <stdexcept>

//identify moves that would collide

namespace warehouse_routing {

    //check whther two agents propose the same destination
    bool hasVertexConflict(int nextNodeA, int nextNodeB) {
        return nextNodeA == nextNodeB;
    }

    //check whether two agents exchange positions during the same tick
    bool hasSwapConflict(int currentNodeA, int nextNodeA, int currentNodeB, int nextNodeB) {
        return nextNodeA == currentNodeB && nextNodeB == currentNodeA && currentNodeA != nextNodeA;
    }

    //check whether two diagonal moves cross through the center of the same square
    bool hasDiagonalConflict(const Graph& graph, int currentNodeA, int nextNodeA, int currentNodeB, int nextNodeB) {
        //convert node IDs into row/column coords
        int currentRowA = graph.rowOf(currentNodeA);
        int currentColA = graph.colOf(currentNodeA);
        int nextRowA = graph.rowOf(nextNodeA);
        int nextColA = graph.colOf(nextNodeA);

        int currentRowB = graph.rowOf(currentNodeB);
        int currentColB = graph.colOf(currentNodeB);
        int nextRowB = graph.rowOf(nextNodeB);
        int nextColB = graph.colOf(nextNodeB);

        //check both moves change row and column by 1
        bool diagonalA = std::abs(nextRowA - currentRowA) == 1 && std::abs(nextColA - currentColA) == 1;
        bool diagonalB = std::abs(nextRowB - currentRowB) == 1 && std::abs(nextColB - currentColB) == 1;

        //exclude movement along same diagonal; other conflict checks handle
        if (!diagonalA || !diagonalB) {return false;}
        bool sameMidpoint = currentRowA + nextRowA == currentRowB + nextRowB && currentColA + nextColA == currentColB + nextColB;

        return sameMidpoint && currentNodeA != currentNodeB && currentNodeA != nextNodeB;
    }

    //helper function
    //return node at this tick, staying at goal after path ends
    int positionAt(const std::vector<int>& path, std::size_t tick) {
        if (tick >= path.size()) {
            return path.back();
        }
        return path[tick];
    }

    std::optional<Conflict> findFirstConflict(const Graph& graph, const std::unordered_map<int, std::vector<int>>& paths) {
        std::vector<int> IDs;
        std::size_t longestLength = 0;

        for (const auto& [agentID, path] : paths) {
            if (path.empty()) {
                throw std::runtime_error("Invalid input");
            }
            IDs.push_back(agentID);
            longestLength = std::max(longestLength, path.size());
        }
        std::sort(IDs.begin(), IDs.end());
        
        for (size_t tick = 0; tick < longestLength; tick++) {
            for (size_t i = 0; i < IDs.size(); i++) {
                for (size_t j = i + 1; j < IDs.size(); j++) {
                    auto A = IDs[i];
                    auto B = IDs[j];

                    int currentA = positionAt(paths.at(A), tick);
                    int currentB = positionAt(paths.at(B), tick);

                    if (hasVertexConflict(currentA, currentB)) {
                        return Conflict {
                            ConflictType::Vertex, //type
                            A, //first agent ID
                            B, //second agent ID
                            currentA, //first agent start node
                            currentA, //first agent end node
                            currentB, //second agent start node
                            currentB, //second agent end node
                            static_cast<int>(tick) //tick when both occupy
                        };
                    }
                }
            }
            if (tick == longestLength - 1) {
                break;
            }

            for (size_t i = 0; i < IDs.size(); i++) {
                for (size_t j = i + 1; j < IDs.size(); j++) {
                    auto A = IDs[i];
                    auto B = IDs[j];

                    int currentA = positionAt(paths.at(A), tick);
                    int currentB = positionAt(paths.at(B), tick);
                    int nextA = positionAt(paths.at(A), tick + 1);
                    int nextB = positionAt(paths.at(B), tick + 1);

                    if (hasSwapConflict(currentA, nextA, currentB, nextB)) {
                        return Conflict {
                            ConflictType::Swap, //type
                            A, //first agent ID
                            B, //second agent ID
                            currentA, //first agent start node
                            nextA, //first agent next move
                            currentB, //second agent start node
                            nextB, //second agent next move
                            static_cast<int>(tick)   //tick
                        };
                    }

                    if (hasDiagonalConflict(graph, currentA, nextA, currentB, nextB)) {
                        return Conflict {
                            ConflictType::Diagonal, //type
                            A, //first agent ID
                            B, //second agent ID
                            currentA, //first agent start node
                            nextA, //first agent next node
                            currentB, //second agent start node
                            nextB, //second agent next node
                            static_cast<int>(tick) // tick
                        };
                    }
                }
            }
        }
        return std::nullopt;
    }
}