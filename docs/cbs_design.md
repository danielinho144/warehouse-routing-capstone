# CBS Design

As of November 8, 2026, CBS is connected to the warehouse simulation. All 284 tests passed.

## Purpose

- Conflict-Based Search coordinates fixed-goal agent routes to prevent vertex, swap, and diagonal conflicts. The simulation supplies a goal for each agent when it requests a plan.
- The independent A* implementation plans each agent's route separately and handles conflicts during the simulation. CBS detects the conflicts in the planned paths, adds restrictions, and replans the affected agents before movement occurs.
- The project will compare how these approaches affect warehouse order throughput.

## Data Structures

### constraintInfo

- Stores a restriction for one agent. The type identifies what type of constraint, whether it be vertex or edge, and the agentID identifies which of the agents are affected. For vertex constraints, toNode is the forbidden cell and tick is the occupancy time. For edge constraints, fromNode and toNode describe the forbidden move and tick is the departure time.

### Conflict

- Describes a vertex, swap, or diagonal conflict between two agents. It stores the conflict type, both agent IDs, each agent's start and end nodes, and the tick. For a vertex conflict, the tick is the occupancy time. For swap and diagonal conflicts, the tick is the departure time for movement during `tick` to `tick + 1`. This information determines which constraints CBS will add when branching.

### CBSNode

- This represents one complete proposed plan for all agents. It stores a list of constraints, agent paths by agent ID, and total cost. Cost is the sum of each path's number of moves and waits which is calculated as path.size() - 1. Each child node will take its parent's constraints and paths, add a constraint, and replan the affected agent.

### CBSQueueEntry

- Stores a CBSNode and its insertion order. The priority queue selects lower cost first, then earlier insertion order when costs tie.

## Solver Interface

- planCBSPaths takes the graph, agents, goal node IDs by agent ID, maxTick, and maxExpandedNodes.
- maxTick must be nonnegative and maxExpandedNodes must be positive.
- Agent IDs must be unique and every agent must have a goal. Start coordinates must be in bounds, and start and goal nodes must exist in the graph.
- Invalid input throws std::invalid_argument.
- Success returns an optional containing paths by agent ID. No agents returns a successful empty map.
- std::nullopt means no plan was returned within the search limits. It does not distinguish exhausted search from a limit being reached.

## Time and Movement Rules

- Planning starts at tick 0.
- Each move or wait costs one tick.
- Vertex constraints forbid occupancy at a specific tick.
- Edge constraints forbid a move starting at a specific tick.
- Agents remain at their goals after their paths end.
- maxTick limits the latest arrival time searched.
- Agents can move horizontally, vertically, or diagonally between free cells, without cutting blocked corners.
- A goal is accepted only if remaining there respects future constraints.

## Implemented Constraint Tree

### Root Node

- Plan each agent's initial path using constrained A* without constraints.
- Store all paths and their combined cost in root node.
- Return std::nullopt if any root path is empty.
- A conflict-free root returns immediately. Otherwise, put it into the priority queue.

### Conflict Selection

- Find the earliest conflict.
- Use consistent ordering when conflicts tie.
- Compare ticks in order, checking vertex conflicts before movement conflicts for each tick.
- Compare agent pairs in ascending ID order.

### Conflict Detection (Week 6)

- findFirstConflict takes the graph and agent paths by agent ID. It returns the earliest Conflict, or std::nullopt if no conflict is found.
- Agent IDs are sorted before comparing pairs so the result does not depend on unordered_map order.
- Scan from tick 0 through the final tick of the longest path. Check all vertex conflicts first, then swap and diagonal conflicts during movement to the next tick.
- The positionAt helper returns the node at a given tick. If an agent's path has ended, it returns the final node so the agent stays at its goal.
- Only vertex conflicts are checked at the final tick because all agents have finished moving.
- Empty paths are rejected with std::runtime_error. An empty map or a single nonempty path returns no conflict.

### Branching

- Create two children.
- Each child restricts one of the conflicting agents.
- Children retain their parent's constraints.
- A vertex conflict adds a vertex constraint for the selected agent at the occupancy tick.
- A swap or diagonal conflict adds an edge constraint using that agent's start and end nodes at the departure tick.
- Both children start from separate copies of the same parent.

### Replanning

- Replan only the newly constrained agent in each child.
- Discard a child if no path is found within maxTick.
- Replanning starts from the agent's original start node, not its position at the conflict.
- Keep the other agents' paths unchanged. Replace the affected path and update total cost by subtracting its old cost and adding the new cost.
- Enqueue the valid child with the next insertion number. New conflicts are checked when that child is selected.

### Node Selection

- Cost is the sum of each path's size minus one.
- Select the lowest-cost node.
- Tie-break: insertion order.
- Create the child constraining the lower-ID agent first.

### Stopping Conditions

- Success: the selected node's paths have no conflicts.
- Return std::nullopt if no candidates remain or maxExpandedNodes is reached.
- expandedNodes increases when a queue entry is removed and examined, not when children are added. The current conflict-free root shortcut returns before this counter is used.

## Hand-Worked Examples

The sketches are in [Hand-Worked Examples.pdf](<Hand-Worked Examples.pdf>).

### Vertex Conflict

- Page 1: two agents occupy the same node at the same tick. Each child restricts one agent's occupancy, producing alternative paths with total cost 4.

### Swap Conflict

- Page 2: two agents exchange cells during the same interval. Waiting removes the swap but creates a vertex conflict that requires another branching step. The final conflict-free paths have total cost 3.

### Diagonal Conflict

- Page 3: two diagonal moves cross during the same interval. Restricting one agent's move allows it to wait while the other crosses. The resulting paths have total cost 3.

## Implementation Status

- Completed structures and constrained A*.
- Completed full-path conflict scanning for week 6.
- Completed fixed-goal CBS root planning, node selection, two-child branching, constrained replanning, cost updates, and search limits.
- Added Graph::isInBounds to validate coordinates before converting them to node IDs.
- 11 constrained A* tests, 15 full-path conflict detection tests, and 22 CBS tests.
- Conflict detection tests cover vertex, swap, and diagonal conflicts, goal waiting, empty input, conflict-free paths, and conflict selection order. Each detected conflict is checked for type, agent IDs, start and end nodes, and tick.
- CBS tests cover vertex, swap, and diagonal resolution; small-case minimum costs; goal waiting; blocked corners; deterministic ties; empty input; invalid inputs; unreachable goals; and search limits. These are targeted checks, not a proof for every warehouse scenario.
- Simulation tests cover a single-agent order, two agents sharing a pickup or delivery cell, planned waits, and a pickup beyond the search limit.

## Week 8 Simulation Integration

- The runner accepts `baseline` or `cbs`. Both planners use the simulation’s order assignment, inventory, and pickup and delivery rules.
- One agent owns a shared destination at a time. A waiting agent keeps its task and uses its current cell as its CBS goal. The owner releases access after leaving the cell. An idle agent blocking a waiter can request a parking cell.
- Each new CBS search starts from the agents’ current cells at planning tick 0. A task change or a handoff can trigger a new joint plan. While an agent still has work, a missing route for an idle teammate also triggers replanning.
- A successful plan replaces the agents’ paths and resets their progress to 0. Each planned move or wait advances the path index by one.

## Search Limits

- The simulation sets `maxTick` to 20 for each CBS search. Each agent has up to 20 moves or waits to reach its current goal. The simulation clock continues across searches.
- The simulation also sets `maxExpandedNodes` to 100. If CBS returns no plan within the limits, `Simulation::step()` throws a runtime error. The solver result does not identify which limit or search condition caused the failure.
- The search-limit test places a pickup 21 moves from the start on an open aisle. The first step raises the expected error with the agent still at its start and the simulation clock at 0.

## Week 9 Comparison Work

Run the five multi-item scenarios with the same inputs and simulation tick limit for both planners. Record completed orders, ticks, throughput, and search-limit failures.

The [multi-item baseline](multi_item_baseline_results.md) completed all 20 orders with 2–8 agents. The 10-agent run completed 8 orders and stopped moving after tick 384 before reaching the 5,000-tick limit. This apparent pickup-aisle congestion deadlock remains a baseline result for comparison, not a guarantee that CBS will resolve it.
