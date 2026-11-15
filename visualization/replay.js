const picker = document.getElementById("replayFile");

let replay;
let snapshotIndex = 0;
let timerID = null;


async function readReplay() {
    const file = picker.files[0];
    if (!file) {
        return;
    }
    clearInterval(timerID);
    timerID = null;
    playButton.textContent = "Play";
    const text = await file.text();
    replay = JSON.parse(text);
    snapshotIndex = 0;
    drawGrid(replay.map);
    drawAgents(replay.snapshots[0].agents);
    console.log(replay.snapshots[0].tick);
    tickLabel.textContent = "Tick " + replay.snapshots[0].tick;
}

picker.addEventListener("change", readReplay);

const nextButton = document.getElementById("nextTick");
const tickLabel = document.getElementById("tickLabel");
nextButton.addEventListener("click", showNextTick);
const playButton = document.getElementById("playPause");
playButton.addEventListener("click", togglePlayback);

function togglePlayback() {
    //console.log("Play clicked");
    if (timerID != null) {
        clearInterval(timerID);
        timerID = null;
        playButton.textContent = "Play";
        return;
    } 
    if (!replay || snapshotIndex == replay.snapshots.length -1) {
        return;
    }
    playButton.textContent = "Pause";
    timerID = setInterval(playNextTick, 250);
    
}

function playNextTick() {
    showNextTick();
    if (snapshotIndex === replay.snapshots.length - 1) {
        clearInterval(timerID);
        timerID = null;
        playButton.textContent = "Play";
    }
}

function showNextTick() {
    //console.log("Next tick clicked");
    if (!replay) {
        return;
    }
    if (snapshotIndex == replay.snapshots.length -1) {
        return;
    }
    snapshotIndex++;
    const snapshot = replay.snapshots[snapshotIndex];
    drawGrid(replay.map);
    drawAgents(snapshot.agents);
    console.log(snapshot.tick)
    tickLabel.textContent = "Tick " + snapshot.tick;
}

function drawGrid(map) {
    const canvas = document.getElementById("warehouseMap");
    const context = canvas.getContext("2d");
    const cellSize = 6;
    canvas.width = map.cols * cellSize;
    canvas.height = map.rows * cellSize;
    context.fillStyle = "gray";

    for (let row = 0; row < map.rows; row++) {
        for (let col = 0; col < map.cols; col++) {
            const cell = map.cells[row][col];
            if (cell === "T") {
                context.fillRect(col * cellSize, row * cellSize, cellSize, cellSize);
            }
        }
    }
}

function drawAgents(agents) {
    const canvas = document.getElementById("warehouseMap");
    const context = canvas.getContext("2d");
    const cellSize = 6;
    context.fillStyle = "blue";
    for (const agent of agents) {
       const x = agent.col * cellSize;
       const y = agent.row * cellSize;
       context.fillRect(x, y, cellSize, cellSize);
        
    }
}