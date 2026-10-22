# CBS Design

## Purpose

- Conflict-Based Search will allow agents to coordinate in order to prevent vertex, swap, and diagonal conflicts.
- The independent A* implementation plans each agent's route separately and handles conflicts during the simulation. CBS will detect the conflicts in the planned paths, add restrictions, and replan the affected agents before movement occurs.
- The project will compare how these approaches affect warehouse order throughput.

## Data Structures

### constraintInfo

- Stores a restriction for one agent. The type identifies what type of constraint, whether it be vertex or edge, and the agentID identifies which of the agents are affected. For vertex constraints, toNode is the forbidden cell and tick is the occupancy time. For edge constraints, fromNode and toNode describe the forbidden move and tick is the departure time.

### Conflict

- Describes a vertex, swap, or diagonal conflict between two agents. It stores the conflict type, both agent IDs, each agent's start and end nodes, and the tick. For a vertex conflict, the tick is the occupancy time. For swap and diagonal conflicts, the tick is the departure time for movement during `tick` to `tick + 1`. This information determines which constraints CBS will add when branching.

### CBSNode

- This represents one complete proposed plan for all agents. It stores a list of constraints, agent paths by agent ID, and total cost. Cost is the sum of each path's number of moves and waits which is calculated as path.size() - 1. Each child node will take its parent's constraints and paths, add a constraint, and replan the affected agent.

## Time and Movement Rules

- Planning starts at tick 0.
- Each move or wait costs one tick.
- Vertex constraints forbid occupancy at a specific tick.
- Edge constraints forbid a move starting at a specific tick.
- Agents remain at their goals after their paths end.
- maxTick limits the latest arrival time searched.
- Agents can move horizontally, vertically, or diagonally between free cells, without cutting blocked corners.
- A goal is accepted only if remaining there respects future constraints.

## Planned Constraint Tree

### Root Node

- Plan each agent's initial path without constraints.
- Store all paths and their combined cost in root node.

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

### Replanning

- Replan only the newly constrained agent in each child.
- Discard a child if no path is found within maxTick.

### Node Selection

- Cost is the sum of each path's size minus one.
- Select the lowest-cost node.
- Planned tie-break: insertion order.
- Create the child constraining the lower-ID agent first.

### Stopping Conditions

- Success: the selected node's paths have no conflicts.
- Failure: no candidate nodes remain within the search limits.

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
- 11 constrained A* tests and 15 full-path conflict detection tests; 121 total tests passing.
- Conflict detection tests cover vertex, swap, and diagonal conflicts, goal waiting, empty input, conflict-free paths, and conflict selection order. Each detected conflict is checked for type, agent IDs, start and end nodes, and tick.
- Constraint tree branching and replanning are next for week 7. Full CBS search is still planned.

## Limitations and Integration

- Failure within maxTick does not prove no longer path exists.
- CBS will plan routes to pickup and delivery locations as assignments change. Shared delivery cells require coordination so agents do not remain there and block others.
