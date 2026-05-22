#pragma once
#include <Arduino.h>

const char wifi_setup_html[] PROGMEM = R"=====(
<!html>
<head>
<title>FifthOS WiFi setup</title>
<meta name="viewport" content="width=device-width, initial-scale=1">
<style>
:root {
    --bg: #030605;
    --panel: #09110d;
    --border: #163120;
    --text: #d7f8df;
    --muted: #7a9380;
    --accent: #7df5ab;
    --danger: #ff9b8d;
}
* { box-sizing: border-box; }
body {
    margin: 0;
    background: radial-gradient(circle at top, rgba(125,245,171,0.08), transparent 26%), var(--bg);
    color: var(--text);
    font-family: "SFMono-Regular", "Menlo", "Consolas", monospace;
}
.page {
    margin: 0 auto;
    max-width: 920px;
    padding: 1.25rem;
}
.hero, .panel {
    background: rgba(9, 17, 13, 0.95);
    border: 1px solid var(--border);
    border-radius: 0.8rem;
    padding: 1rem;
}
.hero { margin-bottom: 1rem; }
h1, h2 {
    margin: 0 0 0.75rem;
    font-weight: 600;
}
h1 {
    color: var(--accent);
    font-size: 1.1rem;
    letter-spacing: 0.08em;
    text-transform: uppercase;
}
h2 {
    font-size: 0.95rem;
}
.hero p, .muted {
    color: var(--muted);
}
.grid {
    display: grid;
    gap: 1rem;
    grid-template-columns: 1.2fr 1fr;
}
.statusline, .network-row {
    align-items: center;
    display: flex;
    gap: 0.75rem;
    justify-content: space-between;
}
.statusline {
    border-bottom: 1px solid var(--border);
    margin-bottom: 0.75rem;
    padding-bottom: 0.75rem;
}
.network-list {
    display: grid;
    gap: 0.45rem;
    margin-top: 0.75rem;
}
.network-row {
    background: transparent;
    border: 1px solid var(--border);
    border-radius: 0.55rem;
    cursor: pointer;
    padding: 0.55rem 0.7rem;
    text-align: left;
    width: 100%;
}
.network-row.active {
    border-color: var(--accent);
    color: var(--accent);
}
.network-meta {
    color: var(--muted);
    font-size: 0.82rem;
}
.controls {
    display: grid;
    gap: 0.75rem;
}
label {
    display: grid;
    gap: 0.35rem;
}
input {
    background: #020604;
    border: 1px solid var(--border);
    border-radius: 0.55rem;
    color: var(--text);
    padding: 0.65rem 0.75rem;
}
.actions {
    display: flex;
    flex-wrap: wrap;
    gap: 0.55rem;
}
button, a.action {
    background: transparent;
    border: 1px solid var(--border);
    border-radius: 0.55rem;
    color: var(--text);
    cursor: pointer;
    padding: 0.55rem 0.85rem;
    text-decoration: none;
}
button:hover, a.action:hover {
    border-color: var(--accent);
    color: var(--accent);
}
.danger:hover {
    border-color: var(--danger);
    color: var(--danger);
}
.message {
    color: var(--muted);
    min-height: 1.2rem;
}
.message.error { color: var(--danger); }
code {
    color: var(--accent);
}
@media (max-width: 760px) {
    .grid { grid-template-columns: 1fr; }
}
</style>
</head>
<body>
<div class="page">
    <section class="hero">
        <h1>FifthOS WiFi setup</h1>
        <p>Connect the board to a local WiFi network or use WPS. The REPL remains available over the setup access point while the device is unconfigured.</p>
        <div class="statusline">
            <div id="summary">Loading WiFi state...</div>
            <a class="action" href="/">open REPL</a>
        </div>
        <div class="muted" id="details"></div>
    </section>

    <div class="grid">
        <section class="panel">
            <h2>Available networks</h2>
            <div class="actions">
                <button id="scanBtn" type="button">scan</button>
                <button id="wpsBtn" type="button">start WPS</button>
            </div>
            <div class="network-list" id="networkList"></div>
        </section>

        <section class="panel">
            <h2>Connect selected network</h2>
            <div class="controls">
                <label>
                    <span>Selected SSID</span>
                    <input id="ssid" type="text" readonly>
                </label>
                <label>
                    <span>Password</span>
                    <input id="password" type="password" placeholder="Enter WiFi password">
                </label>
                <div class="actions">
                    <button id="connectBtn" type="button">save and connect</button>
                    <button id="forgetBtn" class="danger" type="button">forget saved WiFi</button>
                </div>
                <div class="message" id="message"></div>
            </div>
        </section>
    </div>
</div>

<script>
const summary = document.getElementById('summary');
const details = document.getElementById('details');
const networkList = document.getElementById('networkList');
const ssidInput = document.getElementById('ssid');
const passwordInput = document.getElementById('password');
const message = document.getElementById('message');

let selectedSsid = '';

async function getJson(url, options) {
    const response = await fetch(url, options);
    if (!response.ok) {
        throw new Error('HTTP ' + response.status);
    }
    return response.json();
}

function setMessage(text, isError) {
    message.textContent = text;
    message.className = 'message' + (isError ? ' error' : '');
}

function pickNetwork(ssid) {
    selectedSsid = ssid;
    ssidInput.value = ssid;
    document.querySelectorAll('.network-row').forEach((row) => {
        row.classList.toggle('active', row.dataset.ssid === ssid);
    });
}

function renderNetworks(state) {
    networkList.textContent = '';
    if (!state.scan.length) {
        const empty = document.createElement('div');
        empty.className = 'muted';
        empty.textContent = 'No scan results yet.';
        networkList.appendChild(empty);
        return;
    }

    state.scan.forEach((network) => {
        const button = document.createElement('button');
        button.type = 'button';
        button.className = 'network-row' + (network.ssid === selectedSsid ? ' active' : '');
        button.dataset.ssid = network.ssid;
        button.innerHTML = '<span>' + network.ssid + '</span><span class="network-meta">' + network.rssi + ' dBm' + (network.secure ? ' / secure' : ' / open') + '</span>';
        button.addEventListener('click', () => pickNetwork(network.ssid));
        networkList.appendChild(button);
    });
}

function renderState(state) {
    summary.textContent = state.status;
    const info = [];
    if (state.connected) {
        info.push('Connected SSID: ' + state.connected_ssid);
        info.push('Station URL: ' + state.repl_url);
    }
    if (state.ap_active) {
        info.push('Setup AP: ' + state.ap_ssid);
        info.push('AP password: ' + state.ap_password);
        info.push('Setup URL: ' + state.setup_url);
    }
    details.innerHTML = info.map((line) => '<div><code>' + line + '</code></div>').join('');

    if (state.selected_ssid && !selectedSsid) {
        pickNetwork(state.selected_ssid);
    }

    renderNetworks(state);
}

async function refreshState() {
    const state = await getJson('/wifi/status');
    renderState(state);
}

document.getElementById('scanBtn').addEventListener('click', async () => {
    setMessage('Scanning...', false);
    await getJson('/wifi/scan', { method: 'POST' });
    await refreshState();
    setMessage('Scan complete.', false);
});

document.getElementById('wpsBtn').addEventListener('click', async () => {
    setMessage('Starting WPS...', false);
    const result = await getJson('/wifi/wps', { method: 'POST' });
    setMessage(result.message, !result.ok);
    await refreshState();
});

document.getElementById('connectBtn').addEventListener('click', async () => {
    if (!ssidInput.value) {
        setMessage('Select a network first.', true);
        return;
    }
    const body = new FormData();
    body.append('ssid', ssidInput.value);
    body.append('password', passwordInput.value);
    const result = await getJson('/wifi/connect', { method: 'POST', body });
    setMessage(result.message, !result.ok);
    await refreshState();
});

document.getElementById('forgetBtn').addEventListener('click', async () => {
    const result = await getJson('/wifi/forget', { method: 'POST' });
    setMessage(result.message, !result.ok);
    passwordInput.value = '';
    selectedSsid = '';
    ssidInput.value = '';
    await refreshState();
});

window.addEventListener('load', async () => {
    try {
        await refreshState();
        setMessage('Ready.', false);
    } catch (error) {
        setMessage('Failed to load WiFi state: ' + error.message, true);
    }
});
</script>
</body>
</html>
)=====";
