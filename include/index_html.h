#pragma once
#include <Arduino.h>

const char index_html[] PROGMEM = R"=====(
<!html>
<head>
<title>FifthOS terminal</title>
<meta name="viewport" content="width=device-width, initial-scale=1">
<style>
:root {
    --app-height: 100dvh;
    --bg: #020604;
    --bg-soft: #08100b;
    --border: #163120;
    --text: #d7f8df;
    --muted: #6e9078;
    --accent: #7df5ab;
    --danger: #ff9b8d;
}

* {
    box-sizing: border-box;
}

html,
body {
    height: 100%;
    margin: 0;
}

body {
    background:
        radial-gradient(circle at top, rgba(125, 245, 171, 0.08), transparent 28%),
        #020604;
    color: var(--text);
    font-family: "SFMono-Regular", "Menlo", "Consolas", monospace;
    min-height: var(--app-height);
    overflow: hidden;
}

button,
input,
textarea {
    font: inherit;
}

.terminal {
    display: flex;
    flex-direction: column;
    height: var(--app-height);
}

.terminal-bar {
    align-items: center;
    background: rgba(4, 10, 7, 0.92);
    border-bottom: 1px solid var(--border);
    display: flex;
    flex-wrap: wrap;
    gap: 0.7rem;
    justify-content: space-between;
    padding: max(0.65rem, env(safe-area-inset-top)) 1rem 0.65rem;
}

.terminal-title {
    color: var(--accent);
    font-size: 0.85rem;
    letter-spacing: 0.08em;
    text-transform: uppercase;
}

.terminal-tools {
    align-items: center;
    color: var(--muted);
    display: flex;
    flex-wrap: wrap;
    gap: 0.5rem;
}

.tool,
.file-label {
    background: transparent;
    border: 1px solid var(--border);
    border-radius: 0.45rem;
    color: var(--muted);
    cursor: pointer;
    padding: 0.35rem 0.6rem;
}

.tool:hover,
.file-label:hover {
    border-color: var(--accent);
    color: var(--accent);
}

.tool:disabled {
    cursor: not-allowed;
    opacity: 0.5;
}

.file-input {
    display: none;
}

.status {
    color: var(--muted);
    font-size: 0.8rem;
    margin-left: 0.35rem;
}

.viewport {
    flex: 1;
    min-height: 0;
    overflow: auto;
    padding: 1rem;
}

.screen {
    margin: 0 auto;
    max-width: 1100px;
    padding-bottom: 0.25rem;
    white-space: pre-wrap;
    word-break: break-word;
}

.line {
    display: block;
    line-height: 1.5;
    white-space: pre-wrap;
}

.line-system {
    color: #8fb7ff;
}

.line-error {
    color: var(--danger);
}

.line-command {
    color: var(--accent);
}

.composer-wrap {
    background: rgba(4, 10, 7, 0.96);
    border-top: 1px solid var(--border);
    padding: 0.75rem 1rem max(0.75rem, env(safe-area-inset-bottom));
}

.composer {
    align-items: start;
    display: grid;
    gap: 0.7rem;
    grid-template-columns: auto minmax(0, 1fr) auto;
    margin: 0 auto;
    max-width: 1100px;
}

.prompt {
    color: var(--accent);
    line-height: 1.5;
    padding-top: 0.05rem;
}

.prompt-input {
    background: transparent;
    border: 0;
    color: var(--text);
    line-height: 1.5;
    max-height: 16rem;
    min-height: 1.5rem;
    outline: none;
    overflow-y: auto;
    padding: 0;
    resize: none;
    width: 100%;
}

.submit {
    background: transparent;
    border: 1px solid var(--border);
    border-radius: 0.45rem;
    color: var(--muted);
    cursor: pointer;
    padding: 0.45rem 0.75rem;
}

.submit:hover {
    border-color: var(--accent);
    color: var(--accent);
}

.submit:disabled {
    cursor: not-allowed;
    opacity: 0.5;
}

.meta {
    color: var(--muted);
    font-size: 0.78rem;
    margin: 0.45rem auto 0;
    max-width: 1100px;
}

@media (max-width: 720px) {
    .composer {
        grid-template-columns: auto minmax(0, 1fr);
    }

    .submit {
        grid-column: 2;
        justify-self: start;
    }
}
</style>
</head>
<body>
<div class="terminal">
    <header class="terminal-bar">
        <div class="terminal-title">FifthOS terminal</div>
        <div class="terminal-tools">
            <label class="file-label" for="filepick">run file</label>
            <input class="file-input" id="filepick" type="file" name="files[]">
            <button class="tool" data-command="hex">hex</button>
            <button class="tool" data-command="decimal">decimal</button>
            <button class="tool" data-command="words">words</button>
            <button class="tool" id="clearBtn" type="button">clear</button>
            <span class="status" id="status">ready</span>
        </div>
    </header>

    <main class="viewport" id="viewport">
        <div class="screen" id="screen" aria-live="polite"></div>
    </main>

    <div class="composer-wrap">
        <form class="composer" id="composer">
            <label class="prompt" for="prompt">&gt;</label>
            <textarea id="prompt" class="prompt-input" rows="1" spellcheck="false" autocomplete="off" autocorrect="off" autocapitalize="off"></textarea>
            <button class="submit" id="submitBtn" type="submit">run</button>
        </form>
        <div class="meta">Enter runs the command. Shift+Enter inserts a newline. Arrow Up and Arrow Down recall history.</div>
    </div>
</div>

<script>
const HISTORY_KEY = 'fifthos-history-v2';
const HISTORY_LIMIT = 100;

const viewport = document.getElementById('viewport');
const screen = document.getElementById('screen');
const composer = document.getElementById('composer');
const prompt = document.getElementById('prompt');
const submitBtn = document.getElementById('submitBtn');
const clearBtn = document.getElementById('clearBtn');
const filepick = document.getElementById('filepick');
const status = document.getElementById('status');
const tools = Array.from(document.querySelectorAll('[data-command]'));

let history = loadHistory();
let historyIndex = history.length;
let queue = Promise.resolve();
let pendingCount = 0;

function loadHistory() {
    try {
        const saved = localStorage.getItem(HISTORY_KEY);
        const parsed = JSON.parse(saved || '[]');
        return Array.isArray(parsed) ? parsed : [];
    } catch (error) {
        return [];
    }
}

function saveHistory() {
    try {
        localStorage.setItem(HISTORY_KEY, JSON.stringify(history.slice(-HISTORY_LIMIT)));
    } catch (error) {
    }
}

function pushHistory(command) {
    if (!command.trim()) {
        return;
    }
    if (history[history.length - 1] !== command) {
        history.push(command);
        if (history.length > HISTORY_LIMIT) {
            history = history.slice(-HISTORY_LIMIT);
        }
        saveHistory();
    }
    historyIndex = history.length;
}

function setBusyState(text, busy) {
    status.textContent = text;
    submitBtn.disabled = busy;
    tools.forEach((tool) => {
        tool.disabled = busy;
    });
}

function scrollToBottom() {
    viewport.scrollTop = viewport.scrollHeight;
}

function settleScrollToBottom() {
    scrollToBottom();
    requestAnimationFrame(scrollToBottom);
    setTimeout(scrollToBottom, 60);
}

function syncViewportHeight() {
    const height = window.visualViewport ? window.visualViewport.height : window.innerHeight;
    document.documentElement.style.setProperty('--app-height', height + 'px');
}

function autoResizePrompt() {
    prompt.style.height = 'auto';
    prompt.style.height = Math.min(prompt.scrollHeight, 256) + 'px';
}

function appendLine(text, kind) {
    const line = document.createElement('span');
    line.className = 'line' + (kind ? ' line-' + kind : '');
    line.textContent = text;
    screen.appendChild(line);
    settleScrollToBottom();
    return line;
}

function appendOutputBlock(text) {
    if (!text) {
        return;
    }
    const normalized = text.replace(/\r/g, '');
    const cleaned = normalized.replace(/\s*ok>\s*$/g, '');
    if (!cleaned) {
        return;
    }
    const parts = cleaned.split('\n');
    for (let i = 0; i < parts.length; i += 1) {
        if (i === parts.length - 1 && parts[i] === '') {
            continue;
        }
        appendLine(parts[i]);
    }
}

async function httpPost(url, items) {
    const fd = new FormData();
    for (const key in items) {
        fd.append(key, items[key]);
    }
    const response = await fetch(url, { method: 'POST', body: fd });
    if (!response.ok) {
        throw new Error('HTTP ' + response.status);
    }
    return response.text();
}

function toWireCommand(command) {
    return command.endsWith('\n') ? command : command + '\n';
}

function runQueued(task) {
    queue = queue.then(task, task);
    return queue;
}

function submitCommand(command, options = {}) {
    const text = command.trimEnd();
    if (options.echo !== false && text) {
        appendLine('> ' + text, 'command');
    }

    pendingCount += 1;
    setBusyState(options.statusText || 'running...', true);

    return runQueued(async () => {
        try {
            const data = await httpPost('/input', { cmd: toWireCommand(command) });
            appendOutputBlock(data);
            return data;
        } catch (error) {
            appendLine('request failed: ' + error.message, 'error');
            return null;
        } finally {
            pendingCount -= 1;
            setBusyState(pendingCount > 0 ? 'running...' : 'ready', pendingCount > 0);
            settleScrollToBottom();
        }
    });
}

async function handleSubmit(command) {
    const text = command.trimEnd();
    if (!text.trim()) {
        return;
    }
    pushHistory(text);
    prompt.value = '';
    autoResizePrompt();
    await submitCommand(text);
    prompt.focus();
}

function recallHistory(direction) {
    if (history.length === 0) {
        return;
    }
    historyIndex = Math.max(0, Math.min(history.length, historyIndex + direction));
    prompt.value = historyIndex === history.length ? '' : history[historyIndex];
    autoResizePrompt();
    const end = prompt.value.length;
    prompt.setSelectionRange(end, end);
}

function caretAtStart() {
    return prompt.selectionStart === 0 && prompt.selectionEnd === 0;
}

function caretAtEnd() {
    return prompt.selectionStart === prompt.value.length && prompt.selectionEnd === prompt.value.length;
}

prompt.addEventListener('input', autoResizePrompt);

prompt.addEventListener('keydown', async (event) => {
    if (event.key === 'Enter' && !event.shiftKey) {
        event.preventDefault();
        await handleSubmit(prompt.value);
        return;
    }

    if (event.key === 'ArrowUp' && caretAtStart()) {
        event.preventDefault();
        recallHistory(-1);
        return;
    }

    if (event.key === 'ArrowDown' && caretAtEnd()) {
        event.preventDefault();
        recallHistory(1);
    }
});

composer.addEventListener('submit', async (event) => {
    event.preventDefault();
    await handleSubmit(prompt.value);
});

tools.forEach((tool) => {
    tool.addEventListener('click', async () => {
        await submitCommand(tool.dataset.command);
        prompt.focus();
    });
});

clearBtn.addEventListener('click', () => {
    screen.textContent = '';
    appendLine('terminal cleared', 'system');
    prompt.focus();
});

filepick.addEventListener('change', async (event) => {
    const file = event.target.files[0];
    if (!file) {
        return;
    }

    const text = await file.text();
    const commands = text
        .split(/\r?\n/)
        .map((line) => line.trimEnd())
        .filter((line) => line.length > 0);

    appendLine('running ' + file.name + ' (' + commands.length + ' commands)', 'system');

    for (let i = 0; i < commands.length; i += 1) {
        await submitCommand(commands[i], {
            statusText: 'running file ' + (i + 1) + '/' + commands.length + '...'
        });
    }

    appendLine('finished ' + file.name, 'system');
    filepick.value = '';
    prompt.focus();
});

window.addEventListener('load', async () => {
    syncViewportHeight();
    autoResizePrompt();
    appendLine('connected to FifthOS', 'system');
    await submitCommand('', { echo: false, statusText: 'connecting...' });
    prompt.focus();
});

window.addEventListener('resize', syncViewportHeight);

if (window.visualViewport) {
    window.visualViewport.addEventListener('resize', syncViewportHeight);
    window.visualViewport.addEventListener('scroll', syncViewportHeight);
}
</script>
</body>
</html>
)=====";
