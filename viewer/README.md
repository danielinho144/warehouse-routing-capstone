# Warehouse replay viewer

Generate a recording from the repository root:

```bash
cmake --build build
./build/src/warehouse_sim data/scenarios/scenario_2_agents.json 5000
```

The executable writes `replay.json` in the current directory, overwriting the previous recording. Copy or rename it if you want to keep several runs.

On macOS, open the viewer:

```bash
open viewer/index.html
```

Click **Open replay** and choose `replay.json`, or drag the JSON file into the page. No server, installation, or internet connection is required. Keep `index.html`, `style.css`, and `replay.js` together in the viewer directory.

## Controls

- Play/pause, start, and previous/next snapshot buttons.
- A timeline slider to inspect any recorded snapshot.
- Playback speed in simulation ticks per second.
- Zoom controls and scrolling for the warehouse map.
- Agent trails show the last 30 snapshot intervals for all agents by default. Select an agent in the sidebar or on the map to highlight its trail and current goal. Trails appear after movement; a dashed goal line is not a planned route.
- Space toggles playback; left/right arrows step when focus is outside a control.

The panel shows recorded agent status, order ID, position, completed-order count, and throughput. Coordinates are zero-based. All pickup locations remain visible; the current exporter does not record individual order statuses. A final snapshot with unfinished orders is labeled incomplete, without assuming the cause was a deadlock or a tick limit.

The viewer displays recorded C++ positions; it does not run a routing algorithm or interpolate movement. Files are processed in the browser and are not uploaded. The supported schema contains `map` (`rows`, `cols`, `cells`), `orders` (`id`, `pickup`, `delivery`), and `snapshots` (`tick`, `completed_orders`, `throughput`, `agents`). Agent entries contain `id`, `row`, `col`, `status`, and nullable `order_id`.
