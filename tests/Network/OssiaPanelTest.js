// SPDX-FileCopyrightText: 2026 CNRS
// SPDX-FileContributor: Julien Bayle, PRISM <https://www.prism.cnrs.fr/>

// Test of the logic of the auto-built control page
// (src/ossia-max/extras/ossia.panel.html) outside a browser: the widget each
// node gets, OSC encoding and decoding, and the hold of a control while the
// user moves it.
// Run with: node tests/Network/OssiaPanelTest.js

"use strict";

const test = require("node:test");
const assert = require("node:assert");
const fs = require("node:fs");
const path = require("node:path");
const vm = require("node:vm");

const page = path.join(__dirname, "..", "..", "src", "ossia-max", "extras", "ossia.panel.html");

// Objects made in the context of the page compared as plain JSON
const plain = (x) => (x === null || x === undefined ? x : JSON.parse(JSON.stringify(x)));

// The script of the page marked id="logic", run alone
function logic() {
  const html = fs.readFileSync(page, "utf8");
  const m = html.match(/<script id="logic">([\s\S]*?)<\/script>/);
  assert.ok(m, "a <script id=\"logic\"> in the page");
  // What a browser gives a page
  const context = { exports: {}, TextEncoder, TextDecoder };
  vm.createContext(context);
  vm.runInContext(m[1], context, { filename: page });
  return context.exports;
}

test("widgets follow TYPE, RANGE, VALS, ACCESS and UNIT", () => {
  const { widgetFor } = logic();
  const w = (node) => plain(widgetFor(node));

  assert.deepStrictEqual(w({ TYPE: "f", RANGE: [{ MIN: 0, MAX: 10 }] }),
    { kind: "slider", size: 1, min: [0], max: [10], step: "any", read: true, write: true });
  assert.strictEqual(w({ TYPE: "f" }).kind, "number");
  assert.strictEqual(w({ TYPE: "i", RANGE: [{ MIN: -5, MAX: 5 }] }).step, 1);
  assert.strictEqual(w({ TYPE: "T" }).kind, "toggle");
  assert.strictEqual(w({ TYPE: "F" }).kind, "toggle");
  assert.strictEqual(w({ TYPE: "I" }).kind, "button");
  assert.strictEqual(w({ TYPE: "s" }).kind, "text");
  assert.deepStrictEqual(w({ TYPE: "s", RANGE: [{ VALS: ["a", "b"] }] }).values, ["a", "b"]);
  assert.strictEqual(w({ TYPE: "i", RANGE: [{ VALS: [1, 2, 4] }] }).kind, "menu");
  assert.strictEqual(w({ TYPE: "r" }).kind, "color");
  assert.strictEqual(w({ TYPE: "ffff", UNIT: ["color.rgba"] }).kind, "color");
  assert.strictEqual(w({ TYPE: "fff", EXTENDED_TYPE: ["color.rgb.r", "color.rgb.g", "color.rgb.b"] }).kind, "color");
  const vec = w({ TYPE: "fff", RANGE: [{ MIN: 0, MAX: 1 }, { MIN: 0, MAX: 2 }, { MIN: -1, MAX: 1 }] });
  assert.strictEqual(vec.kind, "slider");
  assert.strictEqual(vec.size, 3);
  assert.deepStrictEqual(vec.max, [1, 2, 1]);
  // One RANGE for every component
  assert.deepStrictEqual(w({ TYPE: "ff", RANGE: [{ MIN: 0, MAX: 5 }] }).max, [5, 5]);
  // Read only, write only
  assert.strictEqual(w({ TYPE: "f", ACCESS: 1 }).write, false);
  assert.strictEqual(w({ TYPE: "f", ACCESS: 2 }).read, false);
  // No TYPE: a node without a parameter
  assert.strictEqual(w({ CONTENTS: {} }), null);
  // A type it does not know: shown as text, values separated by spaces
  assert.strictEqual(w({ TYPE: "[ff]" }).kind, "list");
});

test("OSC: a message encoded is decoded back", () => {
  const { encodeMessage, decodePacket } = logic();
  const cases = [
    ["/a/level", "f", [0.5]],
    ["/b", "i", [-3]],
    ["/c", "s", ["hello"]],
    ["/d", "T", [true]],
    ["/d", "F", [false]],
    ["/e", "fff", [1, 2, 3]],
    ["/f", "I", []],
  ];
  for (const [address, tags, values] of cases) {
    const bytes = encodeMessage(address, tags, values);
    assert.strictEqual(bytes.byteLength % 4, 0, address + " padded");
    const msgs = decodePacket(bytes);
    assert.strictEqual(msgs.length, 1);
    assert.strictEqual(msgs[0].address, address);
    assert.deepStrictEqual(plain(msgs[0].values), values, address);
  }
});

test("OSC: a bundle gives its messages, and a color its four bytes", () => {
  const { encodeMessage, decodePacket } = logic();
  const a = new Uint8Array(encodeMessage("/x", "f", [1]));
  const b = new Uint8Array(encodeMessage("/y", "i", [2]));
  const head = new TextEncoder().encode("#bundle\0");
  const bundle = new Uint8Array(16 + 4 + a.length + 4 + b.length);
  bundle.set(head, 0);
  const dv = new DataView(bundle.buffer);
  dv.setUint32(12, 1);
  dv.setUint32(16, a.length);
  bundle.set(a, 20);
  dv.setUint32(20 + a.length, b.length);
  bundle.set(b, 24 + a.length);
  const msgs = decodePacket(bundle.buffer);
  assert.deepStrictEqual(plain(msgs.map((m) => [m.address, m.values])), [["/x", [1]], ["/y", [2]]]);

  const color = decodePacket(encodeMessage("/c", "r", [[255, 128, 0, 255]]));
  assert.deepStrictEqual(plain(color[0].values), [[255, 128, 0, 255]]);
});

test("a control held by the user ignores the network, then takes its last value", () => {
  const { Hold } = logic();
  const h = new Hold(500);
  // Nothing held: a value from the network applies
  assert.strictEqual(h.incoming("/level", [1], 0), true);
  // The user presses the control and moves it
  h.press("/level", 100);
  assert.strictEqual(h.incoming("/level", [2], 150), false);
  assert.strictEqual(h.incoming("/other", [9], 150), true, "the other controls are free");
  h.release("/level", 200);
  // Still held for 500 ms after the release: the value that crossed the gesture
  assert.strictEqual(h.incoming("/level", [3], 400), false);
  assert.deepStrictEqual(plain(h.expired(650)), [], "held until 700");
  assert.deepStrictEqual(plain(h.expired(750)), [["/level", [3]]], "after the hold, the last value held");
  assert.strictEqual(h.incoming("/level", [4], 800), true);
  // A gesture that received nothing gives nothing back
  h.press("/level", 900);
  h.release("/level", 950);
  assert.deepStrictEqual(plain(h.expired(2000)), []);
});
