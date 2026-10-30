// SPDX-FileCopyrightText: 2026 CNRS
// SPDX-FileContributor: Julien Bayle, PRISM <https://www.prism.cnrs.fr/>

// Emitter of the load example of ossia.v8connector: declares N leaves
// (load/p0 to load/p<N-1>, float 0 to 1) and sets them all at each bang.
// The connector sends the values set during a tick together, in OSC bundles
// (@bundle 1), or one message per value (@bundle 0).
//
// Patch: [ossia.v8connector load] with the scripting name "load_emitter",
// its left outlet into [v8 ossia.v8connector.load.emitter.js load_emitter 3000].
// Arguments, after the script name:
//   1. scripting name of the connector (default "load_emitter")
//   2. number of leaves (default 3000)

let OssiaConnector = null;
try {
  OssiaConnector = require("ossia.v8connector");
} catch (e) {
  error("ossia.v8connector.js not found: it lives in the javascript folder of the ossia package\n");
}

const CONNECTOR = jsarguments.length > 1 ? String(jsarguments[1]) : "load_emitter";
const N = jsarguments.length > 2 ? Math.max(1, Math.floor(Number(jsarguments[2]))) : 3000;
const conn = OssiaConnector ? new OssiaConnector(CONNECTOR) : null;
const addresses = [];
for (let i = 0; i < N; i++) addresses.push("p" + i);

let phase = 0;

function declareAll() {
  if (!conn) return;
  const spec = {};
  for (const a of addresses) spec[a] = { type: "float", min: 0, max: 1, default: 0.5 };
  conn.declare(spec);
}

function loadbang() { declareAll(); }

// One frame: every leaf moves, a sine wave travelling along the leaves.
function bang() {
  if (!conn) return;
  phase += 0.05;
  for (let i = 0; i < N; i++) {
    conn.set(addresses[i], 0.5 + 0.45 * Math.sin(phase + (i * 6 * Math.PI) / N));
  }
}

// "ready", "refused" and the values changed elsewhere come back here.
function anything() {
  if (conn) conn.dispatch(messagename, arrayfromargs(arguments));
}
