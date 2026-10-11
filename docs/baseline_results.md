# Independent A* baseline results

## Experiment setup

October 11, 2026 Results:
for the current independent A* simulation with reactive conflict handling.

Map: `warehouse-10-20-10-2-1.map`
Map dimensions: 63 rows × 161 columns
Workload: The same 20 orders, available at tick 0
Agent counts: 2, 4, 6, 8, 10
Initial positions: Row 1, columns 1 through the agent count
Shared delivery position: Row 30, column 10
Tick limit: 5,000 total ticks per run
Movement: Eight directions; no corner cutting through shelves
Movement cost: One tick per straight move, diagonal move, or wait

Each order has one pickup and one delivery position. Each agent handles one order at a time. Pickup and delivery transitions happen after movement without an additional service tick. An agent can receive another order at the beginning of the next tick.

## Baseline behavior

### Order assignment and routing

Idle agents are processed in scenario vector order. Each receives the nearest reachable unclaimed pickup, measured by A* path length. Equal pickup costs are resolved by choosing the lower order ID. These scenarios list agents in ascending ID order.

Each agent initially plans its own route using A* with Chebyshev distance. Queue ties use node ID, then accumulated path cost. Reaching pickup replaces the pickup route with a route to delivery. An unreachable delivery currently raises an error.

### Reactive conflict handling

Agents propose their next moves before any positions change. The simulation checks:

Vertex conflicts: two agents proposing the same destination.
Swap conflicts: two agents exchanging cells in one tick.
Diagonal crossings: agents traversing opposite diagonals of the same square.

For a shared destination, a stationary occupant keeps its cell. Otherwise, the lower-ID agent gets priority. Unresolved swaps and diagonal crossings cause both agents to wait. The simulation repeats conflict checks after changing moves to waits, because waiting can block following agents. Following an agent into a cell it successfully leaves is allowed.

Active agents attempt detours when their proposal would swap with another agent or enter a stationary agent's cell. Detours use a temporary graph that treats current agent positions as blocked. Candidate first moves are checked against current proposals, and the selected route still goes through the normal conflict checks. Detour processing follows agent vector order. This is reactive replanning, not planning with future cell reservations.

### Idle-agent parking

After order assignment, idle agents on or within one cell of a delivery position search for reachable parking using breadth-first search. Parking destinations must be outside that surrounding area for every delivery position and cannot be pickup cells. Search paths avoid currently occupied cells; pickup cells may be traversed but cannot be parking destinations.

Parking destinations are reserved within the parking-planning phase to discourage agents from selecting the same endpoint. These reservations are not a time-based reservation system. Parking moves use the same conflict checks as order-related moves. If no destination is found, the agent waits.

### Stopping and metrics

The run stops when all orders are completed or the total tick limit is reached. Once all orders are completed, a run does not continue solely to park the final delivering agent.

Throughput is completed orders divided by elapsed simulation ticks, reported as orders per tick. It is zero at tick 0. Simulation ticks are not wall-clock runtime.

## Results

| Agents | Completed orders | Elapsed ticks | Throughput (orders/tick) | Finished |
|---:|---:|---:|---:|:---:|
| 2 | 20/20 | 1,518 | 0.0131752 | Yes |
| 4 | 20/20 | 832 | 0.0240385 | Yes |
| 6 | 20/20 | 646 | 0.0309598 | Yes |
| 8 | 20/20 | 490 | 0.0408163 | Yes |
| 10 | 20/20 | 401 | 0.0498753 | Yes |

All five runs completed within the 5,000-tick limit. For this fixed workload, increasing the number of agents reduced completion time and increased throughput.

Raw outputs: [2 agents](../results/baseline/2_agents.txt), [4 agents](../results/baseline/4_agents.txt), [6 agents](../results/baseline/6_agents.txt), [8 agents](../results/baseline/8_agents.txt), [10 agents](../results/baseline/10_agents.txt).

## Reproducing the runs

Run these commands from the project root:

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure

mkdir -p results/baseline

for agents in 2 4 6 8 10; do
    ./build/src/warehouse_sim \
        "data/scenarios/scenario_${agents}_agents.json" 5000 \
        | tee "results/baseline/${agents}_agents.txt"
done
```

The loop replaces existing output files with fresh results. Progress is printed every 100 ticks; final output includes completion, ticks, throughput, and agent positions.


The latest full test run passed 95 of 95 tests.

## Limitations and comparison plan

- This is independent A* with reactive conflict handling, detours, and parking—not plain independent A* and not CBS.
- The baseline does not guarantee completion, fairness, or globally optimal multi-agent routes. Other arrangements can still deadlock or reach the tick limit.
- Results cover one map and one fixed 20-order workload. They do not establish performance for arbitrary agent counts, layouts, or order streams.
- These measurements do not report planning runtime, repeated-trial variation, or a correctness proof for all possible movements.
- Future timeouts should be recorded as baseline outcomes rather than automatically prompting another change to its routing rules.
- Compare a coordinated planner using the same scenarios, movement rules, workload, and tick limit. Record any differences in assignment, parking, or service behavior so their effects are not attributed solely to the planner.