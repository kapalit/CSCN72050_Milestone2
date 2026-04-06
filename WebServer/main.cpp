// =============================================================
// CSCN72050 – Milestone 3   Command-and-Control Web Server
// =============================================================
//
// Person 1 – Project setup, CROW server, HTML/CSS/JS GUI,
//             GET /  and  POST /connect/<ip>/<port>
//
// Person 2 – PUT  /telecommand/
// Person 3 – GET  /telementry_request/
//             GET  /routing_table/
//
// Pre-requisites (see CROW_SETUP.txt):
//   1. crow_all.h  – placed in this folder (WebServer/)
//   2. NuGet asio  – restored automatically by Visual Studio
// =============================================================

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601   // Windows 7+
#endif

// Must appear BEFORE crow_all.h so Crow picks up Winsock2
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

// Crow and its Asio backend (both in standalone / header-only mode)
#define ASIO_STANDALONE
#define CROW_STANDALONE
#include "crow_all.h"

// Our project classes
#include "../CSCN72050_Milestone1/PktDef.h"
#include "../MySocket/MySocket.h"

#include <string>
#include <sstream>
#include <mutex>
#include <vector>
#include <chrono>
#include <ctime>
#include <algorithm>

// =============================================================
// Global shared state
// =============================================================
static std::string     g_robotIP    = "";
static int             g_robotPort  = 0;
static MySocket*       g_socket     = nullptr;
static std::mutex      g_socketMtx;
static ConnectionType  g_connType   = UDP;  // tracks current protocol for teammates

// Rolling packet log (up to 200 entries, newest last)
static std::vector<std::string> g_packetLog;
static std::mutex               g_logMtx;

static void appendLog(const std::string& msg)
{
    auto now = std::chrono::system_clock::now();
    auto t   = std::chrono::system_clock::to_time_t(now);
    char tbuf[32] = {};
    ctime_s(tbuf, sizeof(tbuf), &t);
    std::string ts(tbuf);
    // strip trailing newline that ctime adds
    ts.erase(ts.find_last_not_of("\r\n") + 1);

    std::lock_guard<std::mutex> lk(g_logMtx);
    g_packetLog.push_back("[" + ts + "] " + msg);
    if (g_packetLog.size() > 200)
        g_packetLog.erase(g_packetLog.begin());
}

// =============================================================
// HTML / CSS / JavaScript GUI  (Person 1)
// =============================================================
static const std::string GUI_HTML = R"HTML(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>COIL Robot C2 GUI</title>
<style>
  /* ── Reset & base ── */
  *, *::before, *::after { box-sizing: border-box; margin: 0; padding: 0; }
  body {
    font-family: 'Segoe UI', Arial, sans-serif;
    background: #0d1117;
    color: #c9d1d9;
    min-height: 100vh;
    padding: 16px;
  }

  /* ── Header bar ── */
  .header {
    display: flex;
    align-items: center;
    gap: 14px;
    border-bottom: 1px solid #21262d;
    padding-bottom: 12px;
    margin-bottom: 16px;
  }
  .header h1 {
    font-size: 1.35rem;
    font-weight: 700;
    color: #58a6ff;
    letter-spacing: 1px;
  }
  .status-dot {
    width: 12px; height: 12px;
    border-radius: 50%;
    background: #3d3d3d;
    flex-shrink: 0;
    transition: background .3s;
  }
  .status-dot.connected  { background: #3fb950; box-shadow: 0 0 6px #3fb950; }
  .status-dot.connecting { background: #d29922; box-shadow: 0 0 6px #d29922; }
  #statusText { font-size: .85rem; color: #8b949e; }

  /* ── Main layout ── */
  .layout {
    display: grid;
    grid-template-columns: 300px 1fr;
    grid-template-rows: auto auto;
    gap: 14px;
  }
  @media (max-width: 700px) {
    .layout { grid-template-columns: 1fr; }
  }

  /* ── Cards ── */
  .card {
    background: #161b22;
    border: 1px solid #21262d;
    border-radius: 8px;
    padding: 14px 16px;
  }
  .card-title {
    font-size: .75rem;
    text-transform: uppercase;
    letter-spacing: 1px;
    color: #8b949e;
    margin-bottom: 12px;
    border-bottom: 1px solid #21262d;
    padding-bottom: 6px;
  }

  /* ── Connection panel ── */
  .conn-grid {
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: 8px;
    margin-bottom: 10px;
  }
  label { font-size: .8rem; color: #8b949e; display: block; margin-bottom: 3px; }
  input[type=text], input[type=number] {
    width: 100%;
    background: #0d1117;
    border: 1px solid #30363d;
    border-radius: 6px;
    color: #e6edf3;
    padding: 6px 10px;
    font-size: .9rem;
    outline: none;
    transition: border-color .2s;
  }
  input:focus { border-color: #58a6ff; }

  /* ── Drive pad ── */
  .dpad {
    display: grid;
    grid-template-columns: repeat(3, 56px);
    grid-template-rows: repeat(3, 56px);
    gap: 6px;
    justify-content: center;
    margin: 14px 0 10px;
  }
  .dpad button { width: 56px; height: 56px; }

  /* ── Sliders ── */
  .slider-group { margin-bottom: 10px; }
  .slider-group label { display: flex; justify-content: space-between; }
  input[type=range] { width: 100%; accent-color: #58a6ff; }

  /* ── Action buttons ── */
  .action-row {
    display: flex;
    gap: 8px;
    flex-wrap: wrap;
    margin-top: 10px;
  }

  /* ── Button styles ── */
  button {
    border: none;
    border-radius: 6px;
    cursor: pointer;
    font-size: .85rem;
    font-weight: 600;
    padding: 8px 14px;
    transition: filter .15s, transform .1s;
    color: #fff;
  }
  button:active { transform: scale(.96); }
  button:hover  { filter: brightness(1.15); }
  button:disabled { opacity: .4; cursor: not-allowed; transform: none; }

  .btn-primary { background: #1f6feb; }
  .btn-success { background: #2ea043; }
  .btn-danger  { background: #da3633; }
  .btn-warn    { background: #9e6a03; }
  .btn-info    { background: #1f6feb88; border: 1px solid #58a6ff; }
  .btn-nav     { background: #21262d; border: 1px solid #30363d; font-size: 1.3rem; }

  /* ── Response panel ── */
  .response-box {
    background: #0d1117;
    border: 1px solid #30363d;
    border-radius: 6px;
    padding: 10px;
    min-height: 120px;
    font-family: 'Consolas', monospace;
    font-size: .82rem;
    color: #3fb950;
    white-space: pre-wrap;
    word-break: break-all;
  }

  /* ── Packet log ── */
  .log-area {
    grid-column: 1 / -1;
  }
  #packetLog {
    background: #0d1117;
    border: 1px solid #30363d;
    border-radius: 6px;
    padding: 8px 10px;
    height: 180px;
    overflow-y: auto;
    font-family: 'Consolas', monospace;
    font-size: .78rem;
    color: #8b949e;
    display: flex;
    flex-direction: column;
    gap: 2px;
  }
  #packetLog .log-entry { animation: fadein .3s; }
  #packetLog .log-entry.ack   { color: #3fb950; }
  #packetLog .log-entry.nack  { color: #f85149; }
  #packetLog .log-entry.send  { color: #58a6ff; }
  #packetLog .log-entry.info  { color: #8b949e; }
  @keyframes fadein { from { opacity: 0; } to { opacity: 1; } }

  /* ── Telemetry badge ── */
  .telem-badge {
    display: inline-block;
    background: #21262d;
    border: 1px solid #30363d;
    border-radius: 12px;
    padding: 2px 10px;
    font-size: .78rem;
    margin: 2px;
  }
  .telem-badge span { color: #58a6ff; font-weight: 700; }
</style>
</head>
<body>

<!-- ── Header ── -->
<div class="header">
  <div class="status-dot" id="statusDot"></div>
  <h1>&#x1F916; COIL Robot C&#178;GUI</h1>
  <span id="statusText">Not connected</span>
</div>

<!-- ── Main layout ── -->
<div class="layout">

  <!-- LEFT COLUMN -->
  <div>

    <!-- Connection card -->
    <div class="card" style="margin-bottom:14px">
      <div class="card-title">&#x1F4E1; Connection</div>
      <div class="conn-grid">
        <div>
          <label for="ipInput">Robot IP</label>
          <input type="text" id="ipInput" value="127.0.0.1" placeholder="e.g. 192.168.1.50">
        </div>
        <div>
          <label for="portInput">Port</label>
          <input type="number" id="portInput" value="5000" min="1" max="65535">
        </div>
      </div>
      <div style="display:flex;gap:16px;margin-bottom:10px;font-size:.85rem">
        <label style="display:flex;align-items:center;gap:5px;cursor:pointer">
          <input type="radio" name="proto" value="udp" checked> UDP
        </label>
        <label style="display:flex;align-items:center;gap:5px;cursor:pointer">
          <input type="radio" name="proto" value="tcp"> TCP
        </label>
      </div>
      <button class="btn-primary" style="width:100%" onclick="connectRobot()">Connect</button>
    </div>

    <!-- Drive controls card -->
    <div class="card">
      <div class="card-title">&#x1F3AE; Drive Controls</div>

      <div class="dpad">
        <!-- row 1 -->
        <div></div>
        <button class="btn-nav" title="Forward"  onclick="sendDrive(1)"  id="btnFwd">&#x2191;</button>
        <div></div>
        <!-- row 2 -->
        <button class="btn-nav" title="Left"     onclick="sendDrive(3)"  id="btnLeft">&#x2190;</button>
        <button class="btn-nav" title="Stop"     onclick="sendSleep()"   id="btnStop">&#x23F9;</button>
        <button class="btn-nav" title="Right"    onclick="sendDrive(4)"  id="btnRight">&#x2192;</button>
        <!-- row 3 -->
        <div></div>
        <button class="btn-nav" title="Backward" onclick="sendDrive(2)"  id="btnBwd">&#x2193;</button>
        <div></div>
      </div>

      <!-- Duration slider -->
      <div class="slider-group">
        <label>Duration <span id="durVal">500</span> ms</label>
        <input type="range" id="duration" min="100" max="3000" step="100" value="500"
               oninput="document.getElementById('durVal').textContent=this.value">
      </div>

      <!-- Power slider -->
      <div class="slider-group">
        <label>Power <span id="powVal">100</span> %</label>
        <input type="range" id="power" min="10" max="100" step="5" value="100"
               oninput="document.getElementById('powVal').textContent=this.value">
      </div>

      <div class="action-row">
        <button class="btn-warn"  onclick="sendSleep()"      id="btnSleep">&#x1F4A4; Sleep</button>
        <button class="btn-info"  onclick="requestTelemetry()" id="btnTelem">&#x1F4CA; Telemetry</button>
      </div>
    </div>

  </div><!-- end LEFT -->

  <!-- RIGHT COLUMN – Response / Telemetry panel -->
  <div class="card" style="display:flex;flex-direction:column;gap:10px">
    <div class="card-title">&#x1F4E5; Last Response / ACK</div>
    <div class="response-box" id="responseBox">Awaiting response…</div>

    <!-- Telemetry grid (shown after housekeeping response) -->
    <div id="telemGrid" style="display:none">
      <div class="card-title">&#x1F6F0;&#xFE0F; Housekeeping Telemetry</div>
      <div id="telemBadges"></div>
    </div>
  </div>

  <!-- PACKET LOG (full width) -->
  <div class="card log-area">
    <div class="card-title" style="display:flex;justify-content:space-between">
      <span>&#x1F4DD; Packet Log</span>
      <button class="btn-danger" style="padding:2px 8px;font-size:.75rem" onclick="clearLog()">Clear</button>
    </div>
    <div id="packetLog"></div>
  </div>

</div><!-- end layout -->

<script>
// ======================================================
// State
// ======================================================
let connected = false;
const BASE = window.location.origin;   // e.g. http://localhost:8080

// ======================================================
// UI helpers
// ======================================================
function setStatus(state, text) {
  const dot  = document.getElementById('statusDot');
  const info = document.getElementById('statusText');
  dot.className  = 'status-dot ' + state;
  info.textContent = text;
  connected = (state === 'connected');
  const ctrlBtns = ['btnFwd','btnBwd','btnLeft','btnRight','btnStop','btnSleep','btnTelem'];
  ctrlBtns.forEach(id => document.getElementById(id).disabled = !connected);
}

function showResponse(text) {
  document.getElementById('responseBox').textContent = text;
}

function addLog(msg, cls) {
  const log  = document.getElementById('packetLog');
  const line = document.createElement('div');
  line.className = 'log-entry ' + (cls || 'info');
  const now  = new Date().toLocaleTimeString();
  line.textContent = '[' + now + '] ' + msg;
  log.appendChild(line);
  log.scrollTop = log.scrollHeight;
}

function clearLog() {
  document.getElementById('packetLog').innerHTML = '';
}

// ======================================================
// Connect to robot  –  POST /connect/<ip>/<port>
// ======================================================
async function connectRobot() {
  const ip    = document.getElementById('ipInput').value.trim();
  const port  = parseInt(document.getElementById('portInput').value);
  const proto = document.querySelector('input[name="proto"]:checked').value; // "udp" or "tcp"

  if (!ip || isNaN(port) || port < 1 || port > 65535) {
    alert('Please enter a valid IP address and port number.');
    return;
  }

  setStatus('connecting', 'Connecting to ' + ip + ':' + port + ' (' + proto.toUpperCase() + ')…');
  addLog('Sending POST /connect/' + ip + '/' + port + '?type=' + proto, 'send');

  try {
    const res = await fetch(BASE + '/connect/' + encodeURIComponent(ip) + '/' + port + '?type=' + proto, {
      method: 'POST'
    });
    const data = await res.json();

    if (data.simulator === 'reachable') {
      setStatus('connected', 'Connected to ' + data.ip + ':' + data.port + ' (' + (data.protocol||'UDP') + ')');
      addLog('Connected OK – ' + (data.protocol||'UDP') + ' – ' + data.message, 'ack');
      showResponse(data.message + '\nIP: ' + data.ip + '  Port: ' + data.port + '  Protocol: ' + (data.protocol||'UDP'));
    } else if (data.simulator === 'no_response') {
      setStatus('', 'No response from simulator');
      addLog('Socket created but simulator did not respond – check IP/port/VPN', 'nack');
      showResponse('Warning: ' + data.message);
      connected = false;
    } else {
      setStatus('', 'Connection failed');
      addLog('Connection failed: ' + (data.error || res.status), 'nack');
      showResponse('Error: ' + JSON.stringify(data, null, 2));
    }
  } catch (err) {
    setStatus('', 'Network error');
    addLog('Network error: ' + err.message, 'nack');
    showResponse('Network error: ' + err.message);
  }
}

// ======================================================
// Drive command  –  PUT /telecommand/
// Direction: FORWARD=1  BACKWARD=2  RIGHT=3  LEFT=4
// ======================================================
async function sendDrive(direction) {
  if (!connected) return;

  const dirNames = { 1:'Forward', 2:'Backward', 3:'Right', 4:'Left' };
  const dur   = parseInt(document.getElementById('duration').value);
  const power = parseInt(document.getElementById('power').value);

  const body = { cmd: 'DRIVE', direction, duration: dur, power };
  addLog('PUT /telecommand/ → Drive ' + dirNames[direction]
         + ' dur=' + dur + 'ms pwr=' + power + '%', 'send');

  await sendTelecommand(body);
}

// ======================================================
// Sleep command  –  PUT /telecommand/
// ======================================================
async function sendSleep() {
  if (!connected) return;
  addLog('PUT /telecommand/ → Sleep', 'send');
  await sendTelecommand({ cmd: 'SLEEP' });
}

// ======================================================
// Shared telecommand helper
// ======================================================
async function sendTelecommand(body) {
  try {
    const res = await fetch(BASE + '/telecommand/', {
      method: 'PUT',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(body)
    });
    const data = await res.json();
    displayAck(data);
  } catch (err) {
    addLog('Telecommand error: ' + err.message, 'nack');
    showResponse('Error: ' + err.message);
  }
}

// ======================================================
// Telemetry request  –  GET /telementry_request/
// ======================================================
async function requestTelemetry() {
  if (!connected) return;
  addLog('GET /telementry_request/', 'send');

  try {
    const res  = await fetch(BASE + '/telementry_request/');
    const data = await res.json();

    if (res.ok) {
      addLog('Telemetry ACK received (pkt #' + (data.pktCount || '?') + ')', 'ack');
      showResponse(JSON.stringify(data, null, 2));
      // If telemetry data fields are present, render the badge grid
      if (data.telemetry) renderTelemetry(data.telemetry);
    } else {
      addLog('Telemetry NACK: ' + (data.error || res.status), 'nack');
      showResponse(JSON.stringify(data, null, 2));
    }
  } catch (err) {
    addLog('Telemetry error: ' + err.message, 'nack');
    showResponse('Error: ' + err.message);
  }
}

// ======================================================
// Display ACK / NACK returned by telecommand handler
// ======================================================
function displayAck(data) {
  showResponse(JSON.stringify(data, null, 2));
  if (data.ack === true) {
    addLog('ACK received (pkt #' + (data.pktCount || '?') + ')', 'ack');
  } else if (data.ack === false) {
    addLog('NACK received (pkt #' + (data.pktCount || '?') + ')', 'nack');
  } else {
    // Could be a 501 stub response
    addLog('Response: ' + JSON.stringify(data), 'info');
  }
}

// ======================================================
// Render housekeeping telemetry badges
// ======================================================
function renderTelemetry(telem) {
  const grid  = document.getElementById('telemGrid');
  const badges = document.getElementById('telemBadges');
  badges.innerHTML = '';
  for (const [k, v] of Object.entries(telem)) {
    const b = document.createElement('div');
    b.className = 'telem-badge';
    b.innerHTML = k + ': <span>' + v + '</span>';
    badges.appendChild(b);
  }
  grid.style.display = 'block';
}

// ======================================================
// Init: disable controls until connected
// ======================================================
window.addEventListener('load', () => {
  setStatus('', 'Not connected');
});
</script>
</body>
</html>)HTML";

// =============================================================
// main()
// =============================================================
int main()
{
    crow::SimpleApp app;

    // ----------------------------------------------------------
    // GET /
    // Returns the full HTML Command-and-Control GUI.
    // 
    // ----------------------------------------------------------
    CROW_ROUTE(app, "/")
    ([]()
    {
        crow::response res;
        res.code = 200;
        res.set_header("Content-Type", "text/html; charset=utf-8");
        res.write(GUI_HTML);
        return res;
    });

    // ----------------------------------------------------------
    // POST /connect/<ip>/<port>
    // Stores the robot's IP and port, creates a UDP CLIENT
    // MySocket ready to send commands.
    // 
    // ----------------------------------------------------------
    CROW_ROUTE(app, "/connect/<string>/<int>")
    .methods(crow::HTTPMethod::Post)
    ([](const crow::request& req, const std::string& ip, int port)
    {
        std::lock_guard<std::mutex> lk(g_socketMtx);

        // Read optional ?type=tcp or ?type=udp query parameter (default UDP)
        auto typeParam  = req.url_params.get("type");
        bool useTCP     = (typeParam && std::string(typeParam) == "tcp");
        ConnectionType connType = useTCP ? TCP : UDP;

        // Tear down any previous connection
        if (g_socket)
        {
            if (g_connType == TCP)
                g_socket->DisconnectTCP();
            delete g_socket;
            g_socket = nullptr;
        }

        // Validate inputs
        if (ip.empty() || port <= 0 || port > 65535)
        {
            crow::json::wvalue err;
            err["error"] = "Invalid IP or port.";
            return crow::response(400, err);
        }

        g_robotIP   = ip;
        g_robotPort = port;
        g_connType  = connType;
        g_socket    = new MySocket(CLIENT, ip, (unsigned int)port, connType, 1024);

        bool reachable = false;

        if (useTCP)
        {
            // TCP has a real handshake — ConnectTCP() proves reachability
            // Set a socket-level timeout so it doesn't hang forever
            DWORD tvMs = 3000;
            setsockopt(g_socket->GetConnectionSocket(),
                       SOL_SOCKET, SO_RCVTIMEO,
                       reinterpret_cast<const char*>(&tvMs), sizeof(tvMs));

            g_socket->ConnectTCP();   // blocks until connected or error
            reachable = true;         // if ConnectTCP() returned, connection succeeded

            appendLog("TCP connected to " + ip + ":" + std::to_string(port));
        }
        else
        {
            // UDP is connectionless — probe with a Status packet and wait 2 s
            DWORD tvMs = 2000;
            setsockopt(g_socket->GetConnectionSocket(),
                       SOL_SOCKET, SO_RCVTIMEO,
                       reinterpret_cast<const char*>(&tvMs), sizeof(tvMs));

            PktDef probe;
            probe.SetCmd(RESPONSE);
            probe.SetBodyData(nullptr, 0);
            probe.SetPktCount(1);
            char* probeBuf = probe.GenPacket();
            g_socket->SendData(probeBuf, probe.GetLength());

            char recvBuf[1024] = {};
            int  bytesIn = g_socket->GetData(recvBuf);

            // Remove timeout so normal commands don't time out
            tvMs = 0;
            setsockopt(g_socket->GetConnectionSocket(),
                       SOL_SOCKET, SO_RCVTIMEO,
                       reinterpret_cast<const char*>(&tvMs), sizeof(tvMs));

            reachable = (bytesIn > 0);
            if (reachable)
                appendLog("UDP probe OK – simulator responded (" +
                          std::to_string(bytesIn) + " bytes) at " +
                          ip + ":" + std::to_string(port));
            else
                appendLog("UDP – no probe response from " + ip + ":" +
                          std::to_string(port) + " – check IP/port/VPN");
        }

        crow::json::wvalue res;
        res["status"]    = reachable ? "ok"        : "no_response";
        res["simulator"] = reachable ? "reachable" : "no_response";
        res["ip"]        = ip;
        res["port"]      = port;
        res["protocol"]  = useTCP ? "TCP" : "UDP";
        res["message"]   = reachable
                           ? "Connected via " + std::string(useTCP ? "TCP" : "UDP")
                           : "No response – check IP/port/VPN";
        return crow::response(reachable ? 200 : 202, res);
    });

    // ----------------------------------------------------------
    // PUT /telecommand/
    // Sends a Drive or Sleep packet to the robot via UDP and
    // returns the ACK/NACK parsed from the robot's response.
    //
    // Expected JSON body:
    //   Drive:  { "cmd":"DRIVE", "direction":1, "duration":500, "power":100 }
    //   Sleep:  { "cmd":"SLEEP" }
    //
    // Response JSON:
    //   { "ack": true/false, "pktCount": N }
    //
    // (Person 2 – implement the body below)
    // ----------------------------------------------------------
    CROW_ROUTE(app, "/telecommand/")
    .methods(crow::HTTPMethod::Put)
    ([](const crow::request& req)
    {
        // TODO: Person 2
        //
        // 1. Lock g_socketMtx, confirm g_socket != nullptr.
        // 2. Parse req.body as JSON to determine cmd type.
        // 3. Build a PktDef:
        //      Drive  → SetCmd(DRIVE),  SetBodyData(&driveBody, sizeof(DriveBody))
        //      Sleep  → SetCmd(SLEEP),  SetBodyData(nullptr, 0)
        // 4. Call GenPacket() to get the raw buffer + GetLength().
        // 5. Call g_socket->SendData(buf, len).
        // 6. Call g_socket->GetData(recvBuf) to read ACK.
        // 7. Parse ACK PktDef, check GetAck().
        // 8. appendLog(...) with result.
        // 9. Return { "ack": true/false, "pktCount": N }.

        crow::json::wvalue stub;
        stub["error"]   = "Not implemented – Person 2 TODO";
        stub["ack"]     = false;
        stub["pktCount"]= 0;
        return crow::response(501, stub);
    });

    // ----------------------------------------------------------
    // GET /telementry_request/
    // Sends a Status/housekeeping request to the robot and
    // returns the parsed telemetry as JSON.
    //
    // (Person 3 – implement the body below)
    // ----------------------------------------------------------
    CROW_ROUTE(app, "/telementry_request/")
    .methods(crow::HTTPMethod::Get)
    ([](const crow::request&)
    {
        // TODO: Person 3
        //
        // 1. Lock g_socketMtx, confirm g_socket != nullptr.
        // 2. Build a PktDef with SetCmd(RESPONSE) / Status flag.
        // 3. Send via g_socket->SendData().
        // 4. Await g_socket->GetData() → parse as PktDef.
        // 5. Extract housekeeping fields and return as JSON.

        crow::json::wvalue stub;
        stub["error"] = "Not implemented – Person 3 TODO";
        return crow::response(501, stub);
    });

    // ----------------------------------------------------------
    // GET /routing_table/
    // Routes commands and telemetry to another C2 GUI instance.
    //
    // (Person 3 – implement the body below)
    // ----------------------------------------------------------
    CROW_ROUTE(app, "/routing_table/")
    .methods(crow::HTTPMethod::Get)
    ([](const crow::request&)
    {
        // TODO: Person 3

        crow::json::wvalue stub;
        stub["error"] = "Not implemented – Person 3 TODO";
        return crow::response(501, stub);
    });

    // ----------------------------------------------------------
    // Launch the server
    // ----------------------------------------------------------
    std::cout << "COIL Robot C2 GUI  –  http://localhost:8080/" << std::endl;
    std::cout << "Press Ctrl+C to stop." << std::endl;

    app.port(8080)
       .multithreaded()
       .run();

    // Cleanup
    std::lock_guard<std::mutex> lk(g_socketMtx);
    delete g_socket;
    g_socket = nullptr;

    return 0;
}
