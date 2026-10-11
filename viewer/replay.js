"use strict";

const $ = id => document.getElementById(id);
const canvas = $("map");
const context = canvas.getContext("2d");
const palette = ["#75bafa", "#ffb276", "#bd98ff", "#64d9c4", "#f68fb7", "#d4d974", "#88a0ff", "#e3bc8c", "#79d58f", "#ed91f0"];
const statusNames = { idle: "Idle", pickup_pending: "To pickup", delivering: "Delivering" };
let replay = null;
let index = 0;
let selected = null;
let playing = false;
let lastTime = null;
let elapsed = 0;
let zoom = 1;
let cell = 8;
let mapLayer = null;
let agentIds = [];
let orderById = new Map();
let agentButtons = new Map();
let loadVersion = 0;

function validate(data) {
    const fail = message => { throw new Error(message); };
    const integer = n => Number.isSafeInteger(n) && n >= 0;
    const map = data?.map;
    if (!map || !integer(map.rows) || !integer(map.cols) || map.rows < 1 || map.cols < 1 || map.rows * map.cols > 1000000) {
        fail("The replay needs a valid map with rows and cols (up to one million cells).");
    }
    if (!Array.isArray(map.cells) || map.cells.length !== map.rows || map.cells.some(row => typeof row !== "string" || row.length !== map.cols || /[^.T]/.test(row))) {
        fail("Map cells must contain one string per row, using '.' and 'T'.");
    }
    const position = p => p && integer(p.row) && integer(p.col) && p.row < map.rows && p.col < map.cols && map.cells[p.row][p.col] === ".";
    if (!Array.isArray(data.orders)) fail("The replay is missing its orders array.");
    const orders = new Set();
    for (const order of data.orders) {
        if (!Number.isSafeInteger(order.id) || orders.has(order.id) || !position(order.pickup) || !position(order.delivery)) fail("An order has an invalid ID or location.");
        orders.add(order.id);
    }
    if (!Array.isArray(data.snapshots) || !data.snapshots.length) fail("The replay needs at least one snapshot.");
    let ids = null;
    let previousTick = -1;
    let previousCompleted = 0;
    for (const snapshot of data.snapshots) {
        if (!integer(snapshot.tick) || snapshot.tick <= previousTick || !Array.isArray(snapshot.agents)) fail("Snapshots must have increasing tick numbers and agent arrays.");
        if (!integer(snapshot.completed_orders) || snapshot.completed_orders > data.orders.length || snapshot.completed_orders < previousCompleted || !Number.isFinite(snapshot.throughput) || snapshot.throughput < 0) fail("A snapshot contains invalid metrics.");
        const currentIds = new Set();
        for (const agent of snapshot.agents) {
            if (!Number.isSafeInteger(agent.id) || currentIds.has(agent.id) || !position(agent) || !Object.hasOwn(statusNames, agent.status)) fail("A snapshot contains an invalid agent.");
            if (agent.order_id !== null && !orders.has(agent.order_id)) fail("An agent references an unknown order.");
            currentIds.add(agent.id);
        }
        if (ids && (ids.size !== currentIds.size || [...ids].some(id => !currentIds.has(id)))) fail("The set of agents must remain the same across snapshots.");
        ids = currentIds;
        previousTick = snapshot.tick;
        previousCompleted = snapshot.completed_orders;
    }
    return data;
}

function color(id) { return palette[agentIds.indexOf(id) % palette.length]; }
function stop() {
    playing = false;
    lastTime = null;
    elapsed = 0;
    $("play").textContent = "Play";
}
function togglePlayback() {
    if (!replay || replay.snapshots.length < 2) return;
    if (playing) { stop(); return; }
    if (index === replay.snapshots.length - 1) { index = 0; render(); }
    playing = true;
    lastTime = null;
    $("play").textContent = "Pause";
}
function seek(value) {
    if (!replay) return;
    stop();
    index = Math.max(0, Math.min(replay.snapshots.length - 1, value));
    render();
}
function setSelection(id) {
    selected = selected === id ? null : id;
    render();
}

function buildAgentList() {
    $("agentList").replaceChildren();
    agentButtons = new Map();
    for (const id of agentIds) {
        const button = document.createElement("button");
        button.className = "agent-card";
        const title = document.createElement("span");
        title.className = "agent-title";
        const name = document.createElement("span");
        name.className = "agent-name";
        const dot = document.createElement("span");
        dot.className = "dot";
        dot.style.background = color(id);
        name.append(dot, document.createTextNode(`Agent ${id}`));
        const status = document.createElement("span");
        status.className = "status";
        const detail = document.createElement("span");
        detail.className = "agent-detail";
        title.append(name, status);
        button.append(title, detail);
        button.addEventListener("click", () => setSelection(id));
        $("agentList").append(button);
        agentButtons.set(id, { button, status, detail });
    }
}

function resizeMap() {
    if (!replay) return;
    const available = Math.max(180, $("mapViewport").clientWidth - 32);
    cell = Math.max(3, Math.min(20, available / replay.map.cols)) * zoom;
    cell = Math.min(cell, 12000 / replay.map.cols, 12000 / replay.map.rows);
    const width = replay.map.cols * cell;
    const height = replay.map.rows * cell;
    const dpr = Math.min(window.devicePixelRatio || 1, 2, 16000 / Math.max(width, height));
    canvas.width = Math.ceil(width * dpr);
    canvas.height = Math.ceil(height * dpr);
    canvas.style.width = `${width}px`;
    canvas.style.height = `${height}px`;
    context.setTransform(dpr, 0, 0, dpr, 0, 0);
    mapLayer = document.createElement("canvas");
    mapLayer.width = canvas.width;
    mapLayer.height = canvas.height;
    const ctx = mapLayer.getContext("2d");
    ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
    ctx.fillStyle = "#152431";
    ctx.fillRect(0, 0, width, height);
    ctx.fillStyle = "#496171";
    for (let row = 0; row < replay.map.rows; row++) {
        for (let col = 0; col < replay.map.cols; col++) {
            if (replay.map.cells[row][col] === "T") ctx.fillRect(col * cell, row * cell, cell, cell);
        }
    }
    if (cell >= 9) {
        ctx.strokeStyle = "#263745";
        ctx.lineWidth = 0.4;
        ctx.beginPath();
        for (let col = 0; col <= replay.map.cols; col++) { ctx.moveTo(col * cell, 0); ctx.lineTo(col * cell, height); }
        for (let row = 0; row <= replay.map.rows; row++) { ctx.moveTo(0, row * cell); ctx.lineTo(width, row * cell); }
        ctx.stroke();
    }
    const deliveries = new Set();
    for (const order of replay.orders) {
        const x = (order.pickup.col + 0.5) * cell;
        const y = (order.pickup.row + 0.5) * cell;
        ctx.strokeStyle = "#e9b969";
        ctx.lineWidth = 1;
        ctx.beginPath();
        ctx.moveTo(x, y - cell * 0.36); ctx.lineTo(x + cell * 0.36, y);
        ctx.lineTo(x, y + cell * 0.36); ctx.lineTo(x - cell * 0.36, y); ctx.closePath(); ctx.stroke();
        const key = `${order.delivery.row},${order.delivery.col}`;
        if (!deliveries.has(key)) {
            deliveries.add(key);
            ctx.fillStyle = "#b9e689";
            ctx.fillRect((order.delivery.col + 0.15) * cell, (order.delivery.row + 0.15) * cell, cell * 0.7, cell * 0.7);
        }
    }
    draw();
}

function draw() {
    if (!replay || !mapLayer) return;
    const width = parseFloat(canvas.style.width);
    const height = parseFloat(canvas.style.height);
    context.clearRect(0, 0, width, height);
    context.drawImage(mapLayer, 0, 0, width, height);
    const snapshot = replay.snapshots[index];
    if ($("trail").checked) {
        const first = Math.max(0, index - 30);
        for (const id of agentIds) {
            context.strokeStyle = color(id);
            context.globalAlpha = selected === null || selected === id ? 0.85 : 0.15;
            context.lineWidth = Math.max(2, cell * 0.22);
            context.lineCap = "round";
            context.lineJoin = "round";
            context.beginPath();
            for (let i = first; i <= index; i++) {
                const agent = replay.snapshots[i].agents.find(a => a.id === id);
                const x = (agent.col + 0.5) * cell;
                const y = (agent.row + 0.5) * cell;
                if (i === first) context.moveTo(x, y); else context.lineTo(x, y);
            }
            context.stroke();
        }
        context.globalAlpha = 1;
    }
    const focus = snapshot.agents.find(a => a.id === selected);
    const order = focus ? orderById.get(focus.order_id) : null;
    if (order && focus.status !== "idle") {
        const goal = focus.status === "delivering" ? order.delivery : order.pickup;
        context.strokeStyle = color(focus.id);
        context.lineWidth = 1;
        context.setLineDash([4, 4]);
        context.beginPath();
        context.moveTo((focus.col + 0.5) * cell, (focus.row + 0.5) * cell);
        context.lineTo((goal.col + 0.5) * cell, (goal.row + 0.5) * cell);
        context.stroke();
        context.setLineDash([]);
    }
    for (const agent of snapshot.agents) {
        const x = (agent.col + 0.5) * cell;
        const y = (agent.row + 0.5) * cell;
        context.globalAlpha = selected === null || agent.id === selected ? 1 : 0.3;
        context.fillStyle = color(agent.id);
        context.strokeStyle = "#0b151d";
        context.lineWidth = Math.max(1, cell * 0.08);
        context.beginPath(); context.arc(x, y, Math.max(2, cell * 0.43), 0, Math.PI * 2); context.fill(); context.stroke();
        if (agent.id === selected) {
            context.strokeStyle = "#fff";
            context.beginPath(); context.arc(x, y, cell * 0.65, 0, Math.PI * 2); context.stroke();
        }
        if (cell >= 9) {
            context.fillStyle = "#101923";
            context.font = `bold ${Math.max(7, cell * 0.5)}px sans-serif`;
            context.textAlign = "center"; context.textBaseline = "middle";
            context.fillText(String(agent.id), x, y, cell * 0.75);
        }
    }
    context.globalAlpha = 1;
}

function render() {
    if (!replay) return;
    const snapshot = replay.snapshots[index];
    const final = replay.snapshots.at(-1);
    $("tickMetric").textContent = snapshot.tick.toLocaleString();
    $("tickTotal").textContent = `of ${final.tick.toLocaleString()} recorded ticks`;
    $("ordersMetric").textContent = snapshot.completed_orders;
    $("ordersTotal").textContent = `of ${replay.orders.length} orders`;
    $("throughputMetric").textContent = snapshot.throughput.toFixed(5);
    $("agentsMetric").textContent = snapshot.agents.length;
    $("activeMetric").textContent = `${snapshot.agents.filter(a => a.status !== "idle").length} active at this tick`;
    $("timeline").value = index;
    $("timelineText").textContent = `Tick ${snapshot.tick} / ${final.tick}`;
    $("runState").textContent = index !== replay.snapshots.length - 1 ? "Reviewing recording" : final.completed_orders === replay.orders.length ? "All orders completed" : "Recording ended · incomplete";
    $("previous").disabled = index === 0;
    $("next").disabled = index === replay.snapshots.length - 1;
    $("clearSelection").disabled = selected === null;
    for (const agent of snapshot.agents) {
        const elements = agentButtons.get(agent.id);
        elements.button.classList.toggle("selected", agent.id === selected);
        elements.button.setAttribute("aria-pressed", String(agent.id === selected));
        elements.status.textContent = statusNames[agent.status];
        elements.detail.textContent = `(${agent.row}, ${agent.col}) · ${agent.order_id === null ? "No order" : `Order ${agent.order_id}`}`;
    }
    canvas.setAttribute("aria-label", `Warehouse at tick ${snapshot.tick}. ${snapshot.agents.map(a => `Agent ${a.id}: row ${a.row}, column ${a.col}, ${statusNames[a.status]}`).join(". ")}`);
    draw();
}

async function loadFile(file) {
    if (!file) return;
    const version = ++loadVersion;
    stop();
    $("error").hidden = true;
    try {
        if (file.size > 100 * 1024 * 1024) throw new Error("Choose a replay smaller than 100 MB.");
        const data = validate(JSON.parse(await file.text()));
        if (version !== loadVersion) return;
        replay = data;
        index = 0; selected = null; zoom = 1;
        agentIds = replay.snapshots[0].agents.map(a => a.id).sort((a, b) => a - b);
        orderById = new Map(replay.orders.map(order => [order.id, order]));
        $("fileInfo").textContent = `${file.name} · ${replay.snapshots.length.toLocaleString()} snapshots · loaded locally`;
        $("dimensions").textContent = `${replay.map.rows} rows × ${replay.map.cols} columns · scroll to explore`;
        $("empty").hidden = true;
        canvas.hidden = false;
        $("timeline").max = replay.snapshots.length - 1;
        for (const id of ["timeline", "restart", "play", "zoomOut", "zoomIn", "fit"]) $(id).disabled = false;
        $("play").disabled = replay.snapshots.length < 2;
        $("mapViewport").scrollTo(0, 0);
        buildAgentList(); resizeMap(); render();
    } catch (error) {
        if (version !== loadVersion) return;
        $("error").textContent = `Could not load replay: ${error.message}`;
        $("error").hidden = false;
    }
}

$("file").addEventListener("change", event => { loadFile(event.target.files[0]); event.target.value = ""; });
$("play").addEventListener("click", togglePlayback);
$("restart").addEventListener("click", () => seek(0));
$("previous").addEventListener("click", () => seek(index - 1));
$("next").addEventListener("click", () => seek(index + 1));
$("timeline").addEventListener("input", event => seek(Number(event.target.value)));
$("speed").addEventListener("change", () => { elapsed = 0; lastTime = null; });
$("trail").addEventListener("change", draw);
$("clearSelection").addEventListener("click", () => { selected = null; render(); });
$("zoomIn").addEventListener("click", () => { zoom = Math.min(6, zoom * 1.5); resizeMap(); });
$("zoomOut").addEventListener("click", () => { zoom = Math.max(0.5, zoom / 1.5); resizeMap(); });
$("fit").addEventListener("click", () => { zoom = 1; resizeMap(); $("mapViewport").scrollTo(0, 0); });
window.addEventListener("resize", resizeMap);
document.addEventListener("visibilitychange", () => { if (document.hidden) stop(); });
document.addEventListener("keydown", event => {
    if (!replay || event.ctrlKey || event.metaKey || event.altKey || ["INPUT", "SELECT", "BUTTON", "TEXTAREA"].includes(event.target.tagName)) return;
    if (event.code === "Space") { event.preventDefault(); togglePlayback(); }
    if (event.code === "ArrowRight") { event.preventDefault(); seek(index + 1); }
    if (event.code === "ArrowLeft") { event.preventDefault(); seek(index - 1); }
});
canvas.addEventListener("mousemove", event => {
    if (!replay) return;
    const rect = canvas.getBoundingClientRect();
    const col = Math.floor((event.clientX - rect.left) / cell);
    const row = Math.floor((event.clientY - rect.top) / cell);
    const agent = replay.snapshots[index].agents.find(a => a.row === row && a.col === col);
    $("cellInfo").textContent = `Row ${row}, col ${col}${agent ? ` · Agent ${agent.id} · ${statusNames[agent.status]}` : ""}`;
    canvas.style.cursor = agent ? "pointer" : "default";
});
canvas.addEventListener("click", event => {
    if (!replay) return;
    const rect = canvas.getBoundingClientRect();
    const col = Math.floor((event.clientX - rect.left) / cell);
    const row = Math.floor((event.clientY - rect.top) / cell);
    const agent = replay.snapshots[index].agents.find(a => a.row === row && a.col === col);
    if (agent) setSelection(agent.id);
});
document.addEventListener("dragover", event => { event.preventDefault(); document.body.classList.add("dragging"); });
document.addEventListener("dragleave", event => { if (!event.relatedTarget) document.body.classList.remove("dragging"); });
document.addEventListener("drop", event => { event.preventDefault(); document.body.classList.remove("dragging"); loadFile(event.dataTransfer.files[0]); });
function animate(time) {
    if (playing && replay) {
        if (lastTime === null) lastTime = time;
        elapsed += Math.min(time - lastTime, 250) * Number($("speed").value) / 1000;
        lastTime = time;
        const startingIndex = index;
        while (index < replay.snapshots.length - 1) {
            const gap = replay.snapshots[index + 1].tick - replay.snapshots[index].tick;
            if (elapsed < gap) break;
            elapsed -= gap;
            index++;
        }
        if (index !== startingIndex) render();
        if (index === replay.snapshots.length - 1) stop();
    }
    requestAnimationFrame(animate);
}
requestAnimationFrame(animate);
