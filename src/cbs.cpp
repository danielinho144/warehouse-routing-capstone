#include "warehouse_routing/cbs.h"
#include "warehouse_routing/graph.h"
#include "warehouse_routing/constrained_astar.h"
#include "warehouse_routing/cbs_node.h"
#include "warehouse_routing/conflict_detection.h"
#include <stdexcept>
#include <cstddef>
#include <set>
#include <queue>

namespace warehouse_routing {
    struct CBSQueueEntry {
        CBSNode node;
        std::size_t insertionOrder;
    };
    struct CompareCBSQueueEntries {
        bool operator()(const CBSQueueEntry& a, const CBSQueueEntry& b) const {
            if (a.node.cost != b.node.cost) {
                return a.node.cost > b.node.cost;
            }
            return a.insertionOrder > b.insertionOrder;
        }

    };
    std::optional<std::unordered_map<int, std::vector<int>>> planCBSPaths(
        const Graph& graph, const std::vector<Agent>& agents, 
        const std::unordered_map<int, int>& goals, int maxTick,
        int maxExpandedNodes) {
            if (maxTick < 0) {
                throw std::invalid_argument("maxTick must be non-negative");
            }
            if (maxExpandedNodes <= 0) {
                throw std::invalid_argument("maxExpandedNodes must be positive");
            }
            std::set<int> IDs;
            for (const Agent& agent : agents) {
                if (IDs.contains(agent.getId())) {
                    throw std::invalid_argument("Duplicate ID");
                }
                IDs.insert(agent.getId());
                if (!goals.contains(agent.getId())) {
                    throw std::invalid_argument("Goal is missing");
                }
                int goalID = goals.at(agent.getId());
                Position startPosition = agent.getPosition();
                if (graph.isInBounds(startPosition.row, startPosition.col) == false) {
                    throw std::invalid_argument("Not in bounds");
                }
                int startID = graph.toNodeId(startPosition.row, startPosition.col);
                if (!graph.hasNode(startID) || !graph.hasNode(goalID)) {
                    throw std::invalid_argument("No startID or no goalID");
                }
            }
            CBSNode root;
            for (const Agent& agent : agents) {
                int agentID = agent.getId();
                Position startPosition = agent.getPosition();
                int goalID = goals.at(agent.getId());
                int startID = graph.toNodeId(startPosition.row, startPosition.col);

                std::vector<int> path = constrainedAStar(graph, agentID, startID, goalID, root.constraints, maxTick);
                if (path.empty()) {
                    return std::nullopt;
                }
                root.paths[agentID] = path;
                root.cost += static_cast<int>(path.size() - 1);
            }
            auto conflict = findFirstConflict(graph, root.paths);
            if (!conflict.has_value()) {
                return root.paths;
            }

            std::priority_queue<CBSQueueEntry, std::vector<CBSQueueEntry>, CompareCBSQueueEntries> openNodes;
            std::size_t nextInsertionOrder = 0;
            CBSQueueEntry rootEntry = {
                root,
                nextInsertionOrder
            };
            openNodes.push(rootEntry);
            nextInsertionOrder++;

            int expandedNodes = 0;
            while (!openNodes.empty()) {
                if (expandedNodes >= maxExpandedNodes) {
                    return std::nullopt;
                }
                CBSQueueEntry currentEntry;
                currentEntry = openNodes.top();
                openNodes.pop();
                expandedNodes++;
                auto result = findFirstConflict(graph, currentEntry.node.paths);
                if (!result.has_value()) {
                    return currentEntry.node.paths;
                }
                auto conflict = result.value();
                for (int constrainedAgentID : {conflict.agentA, conflict.agentB}) {
                    CBSNode child = currentEntry.node;
                    int fromNode;
                    int toNode;
                    if (constrainedAgentID == conflict.agentA) {
                        fromNode = conflict.startNodeA;
                        toNode = conflict.endNodeA;
                    }
                    else {
                        fromNode = conflict.startNodeB;
                        toNode = conflict.endNodeB;
                    }
                    constraintInfo constraint = {};
                    if (conflict.type == ConflictType::Vertex) {
                        constraint.type = ConstraintType::Vertex;
                    }
                    else {
                        constraint.type = ConstraintType::Edge;
                    }
                    constraint.agentID = constrainedAgentID;
                    constraint.fromNode = fromNode;
                    constraint.toNode = toNode;
                    constraint.tick = conflict.tick;
                    child.constraints.push_back(constraint);

                    int startNode = currentEntry.node.paths.at(constrainedAgentID).front();
                    int goalNode = goals.at(constrainedAgentID);
                    std::vector<int> newPath;
                    newPath = constrainedAStar(graph, constrainedAgentID, startNode, goalNode, child.constraints, maxTick);
                    if (newPath.empty()) {
                        continue;
                    }
                    int oldPathCost = static_cast<int>(child.paths.at(constrainedAgentID).size() - 1);
                    int newPathCost = static_cast<int>(newPath.size() - 1);
                    child.paths[constrainedAgentID] = newPath;
                    int newChildTotal = child.cost - oldPathCost + newPathCost;
                    child.cost = newChildTotal;
                    openNodes.push({child, nextInsertionOrder});
                    nextInsertionOrder++;
                }
            }
            return std::nullopt;
        }
}