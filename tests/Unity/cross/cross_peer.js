// SPDX-FileCopyrightText: 2026 CNRS
// SPDX-FileContributor: Julien Bayle, PRISM <https://www.prism.cnrs.fr/>
//
// The Max peer of the cross bench (cross.maxpat), driven by cross_run.py
// over OSC on 127.0.0.1: the patch listens on 7493 and answers on 7496. It
// does what the runner asks and counts, on the Max side, every value that
// arrives from the network.
//
// Three roles, one subpatcher each (Max allows one ossia.client or
// ossia.device per patcher):
// - "client": ossia.client and ossia.remote probe/CrossFields/value, which
//   follow the Unity device unity_cross;
// - "device": ossia.device max_cross with ossia.parameter probe/value, which
//   a Unity client follows;
// - "observer": ossia.client and ossia.remote cube/size, cube/angle and cube/spin,
//   which follow follow_me, the device of FollowADevice.maxpat.
// An ossia.remote or an ossia.parameter outputs the value it is given at
// once, while this script is still sending it: that output is not counted.
// Every other output came from the network: it is counted and logged.
//
// From the runner:
//   /cross/hello <log file>            -> /cross/ready
//   /cross/connect <role> <words...>   "connect ..." to the ossia.client of client or observer
//                                      -> /cross/connected <role> 0|1
//   /cross/disconnect <role>
//   /cross/expose                      -> /cross/exposed 0|1
//   /cross/put <role> <leaf> <v>       Max writes one value
//   /cross/burst <role> <leaf> <n> <ms>  n values, one every ms -> /cross/burst_done <role> <leaf> <n> <last>
//   /cross/reset                       counts back to 0
//   /cross/count <role> <leaf>         -> /cross/counted <role> <leaf> <n> <current>
//   /cross/close                       closes the patch
// Every event is also appended to the log file, the epoch time first.

autowatch = 1;
inlets = 1;
outlets = 5;

var OUT = {client: 0, device: 1, observer: 2};
var LOG = "";
var lines = [];
var sending = false;
var rx = {};
var current = {};
var tasks = [];

function now() { return (new Date().getTime() / 1000).toFixed(3); }
function say(text) {
	lines.push(now() + " " + text);
	if (lines.length > 200) flush();
}
function flush() {
	if (!LOG || !lines.length) return;
	var f = new File(LOG, "readwrite");
	if (!f.isopen) return;
	f.position = f.eof;
	for (var i = 0; i < lines.length; i++) f.writeline(lines[i]);
	f.close();
	lines = [];
}
var flusher = new Task(flush, this);
flusher.interval = 250;
flusher.repeat();

function tell() { outlet(3, arrayfromargs(arguments)); }
function later(ms, fn) { var t = new Task(fn, this); tasks.push(t); t.schedule(ms); return t; }
function key(role, leaf) { return role + "/" + leaf; }

function put(role, leaf, v) {
	sending = true;
	outlet(OUT[role], leaf, v);
	sending = false;
	current[key(role, leaf)] = v;
}

function arrival(role, leaf, v) {
	if (sending) return;
	var k = key(role, leaf);
	rx[k] = (rx[k] || 0) + 1;
	current[k] = v;
	if (rx[k] <= 60) say("rx " + role + " " + leaf + " " + v);
}

function burst(role, leaf, n, ms) {
	var k = 0, last = null;
	say("burst " + role + " " + leaf + ": " + n + " values, one every " + ms + " ms");
	var t = new Task(function () {
		k++;
		last = 500 + 0.25 * k;
		put(role, leaf, last);
		if (k >= n) {
			arguments.callee.task.cancel();
			say("burst " + role + " " + leaf + " done, the last " + last);
			tell("/cross/burst_done", role, leaf, n, last);
		}
	}, this);
	tasks.push(t);
	t.interval = ms;
	t.repeat(n);
}

function anything() {
	var args = arrayfromargs(arguments);
	var kind = messagename;
	if (kind == "remote") { arrival("client", String(args[0]), args[1]); return; }
	if (kind == "param") { arrival("device", String(args[0]), args[1]); return; }
	if (kind == "oremote") { arrival("observer", String(args[0]), args[1]); return; }
	if (kind == "client" || kind == "oclient") {
		var role = kind == "client" ? "client" : "observer";
		var text = args.join(" ");
		if (args[0] == "connect" || args[0] == "disconnected") say(role + ": " + text);
		if (args[0] == "connect") {
			var ok = args[1] == 1 ? 1 : 0;
			if (ok) later(500, function () { tell("/cross/connected", role, 1); });
		}
		return;
	}
	if (kind == "device") {
		say("device: " + args.join(" "));
		if (args[0] == "expose") tell("/cross/exposed", args[1] == 1 ? 1 : 0);
		return;
	}
	if (kind == "/cross/hello") {
		if (String(args[0]) != LOG) { LOG = String(args[0]); say("ready, log in " + LOG); }
		tell("/cross/ready");
	} else if (kind == "/cross/connect") {
		var role = String(args[0]);
		say(role + " " + args.slice(1).join(" "));
		outlet(OUT[role], ["client", "connect"].concat(args.slice(1)));
	} else if (kind == "/cross/disconnect") {
		say(String(args[0]) + " disconnect");
		outlet(OUT[String(args[0])], "client", "disconnect");
	} else if (kind == "/cross/expose") {
		say("expose oscquery 9880 9881");
		outlet(1, "device", "expose", "oscquery", 9880, 9881);
	} else if (kind == "/cross/put") {
		say("put " + args.join(" "));
		put(String(args[0]), String(args[1]), args[2]);
	} else if (kind == "/cross/burst") {
		burst(String(args[0]), String(args[1]), args[2], args[3]);
	} else if (kind == "/cross/reset") {
		rx = {};
		say("counts reset");
	} else if (kind == "/cross/count") {
		var k = key(String(args[0]), String(args[1]));
		var cur = current[k];
		say("count " + k + ": " + (rx[k] || 0) + " received, current " + cur);
		tell("/cross/counted", String(args[0]), String(args[1]), rx[k] || 0, cur === undefined ? "-" : cur);
	} else if (kind == "/cross/close") {
		say("closing the patch");
		flush();
		outlet(0, "client", "disconnect");
		outlet(2, "client", "disconnect");
		later(500, function () { flush(); outlet(4, "dispose"); });
	}
}
function list() { anything.apply(this, arguments); }
