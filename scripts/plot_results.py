from pathlib import Path
import matplotlib.pyplot as plt

project_root = Path(__file__).resolve().parent.parent


def make_charts(run_folders, charts_dir, label):
    charts_dir.mkdir(parents=True, exist_ok=True)
    print(label)
    agent_counts = [2, 4, 6, 8, 10]
    planners = ["baseline", "cbs"]
    results = []

    for agent_count in agent_counts:
        for planner in planners:
            summary_path = run_folders[agent_count] / planner / "summary.txt"

            result = {
                "agent_count": agent_count,
                "planner": planner,
                "completed_orders": None,
                "ticks": None,
                "throughput": None,
                "outcome": None
            }

            with summary_path.open("r", encoding="utf-8") as file:
                for line in file:
                    line = line.strip()

                    if line.startswith("Completed orders:"):
                        result["completed_orders"] = int(line.split(":", 1)[1].strip())
                    elif line.startswith("Ticks:"):
                        result["ticks"] = int(line.split(":", 1)[1].strip())
                    elif line.startswith("Throughput:"):
                        value = line.split(":", 1)[1].strip()
                        result["throughput"] = float(value.split()[0])
                    elif line.startswith("Run finished:"):
                         value = line.split(":", 1)[1].strip()

                         if value == "true":
                            result["outcome"] = "finished"
                         else:
                            result["outcome"] = "tick limit"
                    elif line.startswith("Error:"):
                        if "CBS could not find routes within the search limits" in line:
                            result["outcome"] = "search failure"
                        else:
                            result["outcome"] = "error"
            results.append(result)

    for result in results:
        print(result)

    ##the completed orders chart
    x_positions = list(range(len(agent_counts)))
    bar_width = 0.35

    fig, ax = plt.subplots()

    for planner_index, planner in enumerate(planners):
        positions = []
        completed_orders = []

        for index, agent_count in enumerate(agent_counts):
            for result in results:
                if result["agent_count"] == agent_count and result["planner"] == planner:
                    position = index + planner_index * bar_width

                    if result["completed_orders"] is None:
                        ax.text(position, 1, "Search\nfailure", ha="center")
                    else:
                        positions.append(position)
                        completed_orders.append(result["completed_orders"])
        ax.bar(positions, completed_orders, width=bar_width, label=planner)
    tick_positions = [position + bar_width / 2 for position in x_positions]

    ax.set_xticks(tick_positions)
    ax.set_xlim(-0.5, len(agent_counts))
    ax.set_xticklabels(agent_counts)
    ax.set_xlabel("Number of agents")
    ax.set_ylabel("Completed orders")
    ax.set_title(f"{label}: completed orders", pad=40)
    ax.set_ylim(0, 22)
    ax.legend(loc="lower center", bbox_to_anchor=(0.5, 1.02), ncol=2)

    plt.tight_layout()
    fig.savefig(charts_dir / "completed_orders.png", dpi=300, bbox_inches="tight")
    plt.close(fig)

    ##the throughput chart
    fig, ax = plt.subplots()

    for planner_index, planner in enumerate(planners):
        positions = []
        throughput_values = []

        for index, agent_count in enumerate(agent_counts):
            for result in results:
                if result["agent_count"] == agent_count and result["planner"] == planner:
                    position = index + planner_index * bar_width

                    if result["throughput"] is None:
                        ax.text(position, 0.0005, "Search\nfailure", ha="center")
                    else:
                        positions.append(position)
                        throughput_values.append(result["throughput"])

        ax.bar(positions, throughput_values, width=bar_width, label=planner)

    ax.set_xticks(tick_positions)
    ax.set_xlim(-0.5, len(agent_counts))
    ax.set_xticklabels(agent_counts)
    ax.set_xlabel("Number of agents")
    ax.set_ylabel("Throughput (orders/tick)")
    ax.set_title(f"{label}: throughput", pad=40)
    ax.set_ylim(bottom=0)
    ax.legend(loc="lower center", bbox_to_anchor=(0.5, 1.02), ncol=2)

    plt.tight_layout()
    fig.savefig(charts_dir / "throughput.png", dpi=300, bbox_inches="tight")
    plt.close(fig)


#single delivery results
comparison_dir = project_root / "results" / "week9" / "comparison"
run_folders = {
    count: comparison_dir / f"{count}_agents"
    for count in [2, 4, 6, 8, 10]
}
make_charts(run_folders, project_root / "results" / "week9" / "charts", "Single delivery cell")

#four delivery results
comparison_dir = project_root / "results" / "four_deliveries_comparison_20261123_100905"
run_folders = {
    count: comparison_dir / f"{count}_agents"
    for count in [4, 6, 8, 10]
}
run_folders[2] = project_root / "results" / "four_deliveries_2_agents_20261123_095534"
make_charts(run_folders, project_root / "results" / "four_deliveries", "Four delivery cells")
