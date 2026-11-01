# Multi-item independent A* baseline results

## Experiment setup

October 31, 2026 results for independent A* with reactive conflict handling, detours, and idle-agent parking.

Map: `warehouse-10-20-10-2-1.map`
Map dimensions: 63 rows × 161 columns
Workload: The same 20 orders, available at tick 0
Agent counts: 2, 4, 6, 8, 10
Initial positions: Row 1, columns 1 through the agent count
Agent capacity: 5 total item units
Shared delivery position: Row 30, column 10
Tick limit: 5,000 total ticks per run
Movement: Eight directions; no corner cutting through shelves
Movement cost: One tick per straight move, diagonal move, or wait

The workload contains five item types, SKU_A through SKU_E. Each item has four storage locations with 10 units each, giving 40 available units per item and 200 units overall. The orders require 21 units of each item, or 105 units overall. Stock is sufficient for every order.

Fifteen orders contain two item types and require five units total. Five orders contain three item types and require six units total. Those six-unit orders require multiple delivery trips because agent capacity is five.

All five scenarios use the same inventory, orders, capacity, and delivery position. Only the agent count and corresponding starting positions differ.

## Fulfillment behavior

- Each agent handles one assigned order at a time.
- Assignment chooses the order with the nearest reachable stocked pickup for any outstanding item. Equal costs choose the lower order ID.
- Pickup planning subtracts both delivered and already-carried quantities from the order requirement, then chooses the nearest reachable stocked location. Equal pickup costs retain the first candidate in order-line and inventory order.
- Inventory is not reserved. Stock is checked again when the agent arrives at the aisle pickup position.
- Collection is limited by outstanding need, available stock, and remaining carrying capacity.
- Agents can visit several pickup locations before delivering. Full agents deliver and return for another trip when needed.
- Items count toward fulfillment when unloaded at delivery. Orders complete only after every required quantity has been delivered.
- If no outstanding item has stock, the order is marked unfinished. Carried items are delivered before the agent releases the order. Unreachable stock is kept separate from exhaustion.
- A run finishes when every order is completed or unfinished and every agent is idle, empty, and unassigned. The tick limit can stop a run with active work remaining.
- Throughput is fully completed orders divided by elapsed simulation ticks. Partially fulfilled orders do not count as completed.

The reactive movement and conflict rules are described in the [original baseline notes](baseline_results.md#reactive-conflict-handling). Parking now avoids inventory pickup cells. This is still independent A* with reactive handling, not CBS.

## Results

| Agents | Completed orders | Unfinished orders | Active orders | Elapsed ticks | Throughput (orders/tick) | Run finished |
|---:|---:|---:|---:|---:|---:|:---:|
| 2 | 20/20 | 0 | 0 | 2,463 | 0.00812018 | Yes |
| 4 | 20/20 | 0 | 0 | 1,374 | 0.014556 | Yes |
| 6 | 20/20 | 0 | 0 | 1,011 | 0.0197824 | Yes |
| 8 | 20/20 | 0 | 0 | 799 | 0.0250313 | Yes |
| 10 | 8/20 | 0 | 12 | 5,000 | 0.0016 | No |

The 2-, 4-, 6-, and 8-agent runs completed every order. Increasing the agent count reduced completion time across those four runs. The 10-agent run reached the tick limit with twelve active orders. Its 5,000 ticks are elapsed observation time, not a completed-workload completion time.

Raw outputs and recordings:

| Agents | Console output | Replay |
|---:|---|---|
| 2 | [Output](../results/multi_item/2_agents.txt) | [Replay](../results/multi_item/2_agents_replay.json) |
| 4 | [Output](../results/multi_item/4_agents.txt) | [Replay](../results/multi_item/4_agents_replay.json) |
| 6 | [Output](../results/multi_item/6_agents.txt) | [Replay](../results/multi_item/6_agents_replay.json) |
| 8 | [Output](../results/multi_item/8_agents.txt) | [Replay](../results/multi_item/8_agents_replay.json) |
| 10 | [Output](../results/multi_item/10_agents.txt) | [Replay](../results/multi_item/10_agents_replay.json) |

## 10-agent deadlock observation

- The last order completion occurred at tick 278.
- The last recorded movement occurred at tick 384.
- Positions then remained unchanged through tick 5,000, covering 4,616 consecutive movement intervals without progress.
- All ten agents were clustered around row 19, columns 72 through 80, with one agent at row 20, column 80.
- Eight agents remained in pickup-pending status and two remained delivering.
- Eight orders completed, twelve remained active, and none were marked unfinished due to stock exhaustion.

This is recorded as an apparent congestion deadlock in the reactive baseline. The recording establishes sustained lack of movement; it does not independently prove that no coordinated route exists or identify every dependency causing the stall. It is not a stock-shortage outcome.

Keep this run as a baseline tick-limit result. Do not change the routing rules just to make this scenario complete. Revisit it if a separate implementation bug is identified. Use the same workload when testing CBS, without assuming CBS will necessarily complete it.

## Verification and limits

The saved recordings were checked across all five runs. No vertex collisions, swaps, diagonal crossings, blocked-cell occupancy, invalid movement steps, or corner cutting were found. Snapshots cover consecutive ticks, completion counts do not decrease, and throughput matches the recorded counts and elapsed ticks. The recordings' initial inventory and order definitions match the experiment scenarios.

The full test suite passed 231 tests before these experiment runs. Those tests include multi-trip fulfillment, stock shortages, empty-agent release, and two agents competing for the last available stock.

The recordings currently export initial inventory and order requirements, agent positions, statuses, and order IDs. They do not record changing stock, carried quantities, or delivered quantities per order line. These files alone cannot verify inventory conservation or capacity compliance throughout the experiment runs.

These results cover one fixed map and workload. They do not establish general performance or planning runtime. The earlier single-pickup results used a different workload and should remain separate from these measurements.

## Reproducing the runs

Run from the project root after building `warehouse_sim`:

```bash
mkdir -p results/multi_item
for agents in 2 4 6 8 10; do
    if ./build/src/warehouse_sim "data/scenarios/scenario_${agents}_agents.json" 5000 \
        > "results/multi_item/${agents}_agents.txt" 2>&1; then
        cp replay.json "results/multi_item/${agents}_agents_replay.json"
    fi
done
```

This replaces the named console output and successful-run replay files. The executable overwrites the root `replay.json` each time it saves a recording. Preserve the existing results before rerunning if they are needed for comparison. A failed process should be investigated; do not mistake an older replay for a fresh recording.

## CBS comparison plan

Keep the same map, orders, inventory, capacities, assignment policy, service timing, parking behavior, and tick limit. Compare completed orders and throughput, and report tick-limit outcomes explicitly. Record any changes in these rules so their effects are not attributed only to CBS.

Start Week 7 with CBS search for fixed start/goal pairs. Connect it to changing pickup and delivery tasks after the search and its conflict tests work.
