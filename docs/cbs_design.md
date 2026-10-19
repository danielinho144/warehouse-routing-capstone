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
- 11 constrained A* tests; 106 total tests passing.
- Full-path conflict scanning and CBS search still planned.

## Limitations and Integration

- Failure within maxTick does not prove no longer path exists.
- CBS will plan routes to pickup and delivery locations as assignments change. Shared delivery cells require coordination so agents do not remain there and block others.
