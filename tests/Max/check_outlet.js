// SPDX-FileCopyrightText: 2026 CNRS
// SPDX-FileContributor: Julien Bayle, PRISM <https://www.prism.cnrs.fr/>
// The script of check_outlet.maxpat. A script always runs on Max's main thread:
// it writes to the parameter from there, and sees the outlet of the object as
// a script does, after what it is running. Outlet 0: into the object; outlet 1:
// to the port the bench reads.
inlets = 1;
outlets = 2;
autowatch = 0;

var sending = false;

// Holds the main thread for ms, then writes value to the parameter (none if negative)
function go(ms, value) {
	outlet(1, "/js_begin");
	var end = Date.now() + ms;
	while (Date.now() < end) {}
	if (value >= 0) {
		sending = true;
		outlet(0, value);
		sending = false;
	}
	outlet(1, "/js_end");
}

// The workaround of the limit written in the README: a bang outputs what the
// device holds, on this thread
var refresh = false, refreshing = false;
function workaround(on) {
	refresh = on != 0;
}

// The outlet of the object, as a script receives it
function rx(value) {
	if (sending)
		return;
	if (refresh && !refreshing) {
		refreshing = true;
		outlet(0, "bang");
		refreshing = false;
		return;
	}
	outlet(1, "/js_rx", value);
}
