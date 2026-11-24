# Four-delivery warehouse results

Compared independent A* with reactive detours (baseline) and CBS using four delivery cells instead of one. Each pair used the same scenario and a 5,000-tick run limit. Throughput is completed orders divided by elapsed simulation ticks.

## Setup

- Used the existing warehouse map with 63 rows and 161 columns.
- Delivery cells are (5, 5), (5, 155), (57, 5), and (57, 155), using zero-based row and column coordinates.
- Each delivery cell is near the outer corner of one quadrant, with open space around it.
- The 20-order scenarios assign five orders to each delivery cell. Agent capacity is five items.
- CBS used a 150-tick planning horizon and a 200-node expansion limit for the initial comparisons.

The saved executables for the 2-agent comparison, the 4–10-agent comparison, and the initial 20-agent experiment are identical. The later 20-agent CBS rerun increased the expansion limit to 1,000.

## 20-order comparison

| Agents | Planner | Completed orders | Ticks | Orders/tick | Outcome |
|---:|---|---:|---:|---:|---|
| 2 | Baseline | 1/20 | 5,000 | 0.00020 | Tick limit |
| 2 | CBS | 20/20 | 2,384 | 0.00839 | Finished |
| 4 | Baseline | 20/20 | 1,434 | 0.01395 | Finished |
| 4 | CBS | 20/20 | 1,524 | 0.01312 | Finished |
| 6 | Baseline | 20/20 | 1,055 | 0.01896 | Finished |
| 6 | CBS | 20/20 | 1,047 | 0.01910 | Finished |
| 8 | Baseline | 20/20 | 867 | 0.02307 | Finished |
| 8 | CBS | 20/20 | 996 | 0.02008 | Finished |
| 10 | Baseline | 9/20 | 5,000 | 0.00180 | Tick limit |
| 10 | CBS | 20/20 | 1,084 | 0.01845 | Finished |

[2-agent runs](../four_deliveries_2_agents_20261123_095534/) · [4-, 6-, 8-, and 10-agent runs](../four_deliveries_comparison_20261123_100905/)

CBS completed every order at all five agent counts. Baseline finished sooner with 4 and 8 agents. CBS finished eight ticks sooner with 6 agents.

The 2-agent baseline run kept moving but stopped making progress. Replay tracing showed the agents approaching each other, choosing detours, and meeting again on another row. Both repeated this pattern instead of reaching their deliveries. The run completed only one order. This was a repeated movement cycle, not a slow route-planning call.

Compared with the [single-delivery results](../week9/README.md), the 8-agent baseline improved from 17 completed orders to all 20. The 10-agent baseline improved from 3 to 9, and CBS completed all 20 instead of stopping during planning. The 2-agent baseline performed worse. Changing delivery locations also changes travel distances and routes, so these results do not isolate congestion alone.

## 20 agents and 400 orders

This scenario uses the same map and four delivery cells. The original 20 orders were repeated to create 400 orders, with 100 assigned to each delivery cell. Inventory quantities were increased to cover demand. Agent capacity remained five items.

| Planner | CBS expansion limit | Completed orders | Ticks | Orders/tick | Outcome |
|---|---:|---:|---:|---:|---|
| Baseline | — | 17/400 | 5,000 | 0.00340 | Tick limit |
| CBS | 200 | 0/400 at last log | 200 at last log | — | CBS search limit |
| CBS | 1,000 | 2/400 at last log | 300 at last log | — | CBS search limit |

[Initial baseline and CBS runs](../four_deliveries_20_agents_400_orders_20261123_101937/) · [CBS rerun with 1,000 expansions](../four_deliveries_20_agents_400_orders_cbs_nodes1000_20261123_103525/)

Both CBS runs exited with code 1 before printing final totals. Their table entries show the last saved progress message, not the exact failure tick or final completed-order count. Final throughput is unavailable for those runs.

## Why the larger CBS runs stopped

Separate traces of the failing runs showed:

- With 200 expansions, CBS failed during the step from tick 237 to 238, after one order had completed. It reached the expansion limit with 201 candidate plans still queued.
- With 1,000 expansions, CBS got past that failure but stopped during the step from tick 354 to 355, after three orders had completed. It reached the expansion limit with 999 candidate plans still queued.

At both failures, all agents found initial individual paths within the 150-tick planning horizon. CBS then ran out of expansions while resolving conflicts between those paths. Waiting agents occupied cells along the routes needed by active agents. The pickup was still reachable around them, so the reachability check did not request a step aside or provide a backup goal for these attempts.

The higher expansion limit allowed more progress, but did not finish the larger workload. These failures do not prove that no conflict-free routes exist; the search stopped at its configured limit.

## Findings

Four delivery cells helped several runs finish, but did not remove pickup queues or repeated detours. CBS completed the 20-order scenarios with up to 10 agents, while the 20-agent workload still exceeded its search limits. Baseline avoided the CBS search cost but could continue moving without completing more orders.

These are individual runs on fixed scenarios. Simulation ticks measure movement time, not the wall-clock time spent planning.
