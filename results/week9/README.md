# Week 9 warehouse routing results

Compared independent A* (baseline) and Conflict-Based Search (CBS) on the same 20 orders. Each pair used the same scenario and a 5,000-tick limit. Throughput is completed orders divided by elapsed ticks. All ten runs used the same executable, built from commit `4d9af12` with the local planner changes. [Run folder](comparison/) · [Base revision](comparison/revision.txt) · [Planner changes](comparison/planner_changes.patch)

## Comparison

| Agents | Planner | Completed orders | Ticks | Orders/tick | Outcome |
|---:|---|---:|---:|---:|---|
| 2 | Baseline | 20/20 | 2,601 | 0.00769 | Finished |
| 2 | CBS | 20/20 | 2,823 | 0.00708 | Finished |
| 4 | Baseline | 20/20 | 1,550 | 0.01290 | Finished |
| 4 | CBS | 20/20 | 2,462 | 0.00812 | Finished |
| 6 | Baseline | 20/20 | 1,487 | 0.01345 | Finished |
| 6 | CBS | 20/20 | 2,450 | 0.00816 | Finished |
| 8 | Baseline | 17/20 | 5,000 | 0.00340 | Tick limit |
| 8 | CBS | 20/20 | 2,488 | 0.00804 | Finished |
| 10 | Baseline | 3/20 | 5,000 | 0.00060 | Tick limit |
| 10 | CBS | 0/20 at last log | 200 at last log | — | CBS search limit |

Baseline finished sooner with 2, 4, and 6 agents. With 8 agents, CBS finished all 20 orders and baseline reached the tick limit with 3 orders left.

## Why the 10-agent CBS run stopped

The saved log last showed 0/20 orders at tick 200. The run exited with code 1 before printing final totals. [Same-build CBS log](comparison/10_agents/cbs/summary.txt)

An earlier diagnostic run with the same logged progress and error showed that one order had completed when the planner failed during the step from tick 270 to 271. The details below come from that trace.

Agent 6 moved from (17, 36) to (16, 36) while waiting for pickup access. The simulation updated its waiting goal to its current cell and requested a new plan. That cell was on agent 7’s shortest route to pickup. Agent 0 was also waiting at (19, 80), on agent 9’s shortest route. Both pickups were still reachable around the waiting agents, so the reachability check did not request a step aside.

All ten agents found individual paths within the 150-tick planning limit. CBS then tried to resolve the conflicts between those paths. Of the 200 group plans it checked, 142 had a conflict between agents 6 and 7, and 58 had a conflict between agents 0 and 9. CBS reached its 200-node expansion limit with 201 candidate plans still queued and stopped before the clock advanced to tick 271.
