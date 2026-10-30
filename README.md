ossia-prism (PRISM, CNRS)
===

**ossia-prism** is the Max package built from this repository: [ossia](https://ossia.io) 2.1.0 for Max (every `ossia.*` object of the official package, unchanged except for the fixes listed under [Differences from ossia 2.1.0](#differences-from-ossia-210)) plus one object, **`ossia.v8connector`**. It replaces the official `ossia` package: both contain the same objects and cannot be installed together. Binaries are published on the project's [Releases](https://gitlab.prism.cnrs.fr/labo/outils/libossia/-/releases) page: macOS universal (Apple Silicon and Intel) for `2.1.0+prism.4`; Windows x64 to come, same version, added to the Release later; the French installation notes are in [docs/prism/LISEZMOI.txt](docs/prism/LISEZMOI.txt).

Julien Bayle, PRISM (CNRS), https://www.prism.cnrs.fr/

### ossia.v8connector

`[ossia.v8connector <name>]` is a node of the ossia tree whose parameters are declared by messages, typically from a `v8` or `v8ui` script, so that a script can publish its own parameters without one `ossia.parameter` object per parameter. It registers like an `ossia.model`: under the nearest `ossia.model`, `ossia.device` or `ossia.client` of the patcher hierarchy, or under the global device. Its parameters live in the same tree as the ones declared by hand, and are served by the same OSCQuery server.

| Message | Effect |
|---|---|
| `add <address> @type ... @min ... @max ...` | creates a parameter under the node, with the attribute names of `ossia.parameter` (`@type`, `@min`, `@max`, `@range`, `@unit`, `@default`, `@clip`, `@mode`, `@description`, `@tags`, `@priority`, `@rate`, `@critical`, `@invisible`, `@mute`, `@repetitions`, `@enable`, `@recall_safe`). A numeric type requires `@min` and `@max`, or `@range`. A `@type` that the `@unit` would change is refused (`color.rgb` is `vec3f`, `color.rgba` is `vec4f`). `@clip none` is accepted as the free mode. |
| `remove <address>`, `clear` | removes one parameter, or every parameter of this instance |
| `bind <address>`, `unbind <address>` | follows a parameter that exists elsewhere (an `ossia.parameter`, another instance), addressed as an `ossia.remote` would; creates nothing, follows the parameter when it disappears and comes back |
| `set <address> <values...>` | pushes a value to a declared or bound parameter |
| left outlet | `<address> <values...>` for every value changed by another source, bounded and filtered like the outlet of every ossia object; `ready` once the object has registered; `refused <address> <reason>` for every refusal of `add`, `bind`, `set`, `remove` or `unbind` |
| right outlet | `address`, `add`, `remove`, `clear`, `bind <address> <count>` and `error` messages |

Minimal `v8` or `v8ui` script, with the helper `javascript/ossia.v8connector.js`. The object has the scripting name `synth`; its left outlet is patched into the script's inlet, nothing else is wired:

```js
const OssiaConnector = require("ossia.v8connector");
const synth = new OssiaConnector("synth");

function loadbang() {
  synth.declare({
    gain:   { type: "float", min: -60, max: 6, unit: "gain.db", default: -12 },
    cutoff: { type: "float", range: [20, 20000], unit: "time.hz" },
    voices: { type: "int", min: 1, max: 16, default: 4 },
  });
  synth.on("gain", (v) => { post("gain", v, "\n"); });
}

function anything() { synth.dispatch(messagename, arrayfromargs(arguments)); }

function bang() { synth.set("gain", -6); }
```

`synth.bind("other/gain")` follows a parameter declared elsewhere instead of creating one. The helper keeps the declarations and replays them when the object sends `ready`, so the object can be deleted and recreated. It reports every `refused` message in the Max console and calls `synth.onRefused(address, reason)` when it is set.

#### Recipes

**Add and remove from a script.** The helper sends `add` and `remove` itself: it reaches the object through its scripting name (the varname set in the inspector), so declaring needs no patch cord. The left outlet is patched into the script only to receive values, `ready` and `refused`. A `v8ui` grid where a click creates `cell/x/y` and a second click removes it:

```js
const OssiaConnector = require("ossia.v8connector");
const grid = new OssiaConnector("grid");   // [ossia.v8connector grid], varname "grid"
const cells = new Set();

function toggle(x, y) {
  const address = "cell/" + x + "/" + y;
  if (cells.has(address)) {
    cells.delete(address);
    grid.remove(address);   // sends: remove cell/3/4
  } else {
    cells.add(address);
    grid.add(address, { type: "float", min: 0, max: 1, default: 0 });
    // sends: add cell/3/4 @type float @min 0 @max 1 @default 0
  }
}

function anything() { grid.dispatch(messagename, arrayfromargs(arguments)); }
```

When the object is deleted and recreated (undo, paste), it sends `ready` and the helper replays the cells still declared, not the removed ones. `tests/Network/V8ConnectorHelperTest.js` runs this example outside Max: `node tests/Network/V8ConnectorHelperTest.js`.

The package ships this recipe as a patcher, `examples/ossia.v8connector.grid.maxpat`: an 8 x 8 grid in a `v8ui` (`ossia.v8connector.grid.js`, connector `grid`), an `ossia.device` to expose on ports 7101 (OSC) and 7102 (OSCQuery), and a `jweb` (`ossia.v8connector.grid.html`) that shows the served tree live: it reads the tree over HTTP, then applies the `PATH_ADDED` and `PATH_REMOVED` messages of the server's WebSocket. Click `expose` once.

**Who owns a parameter.** An instance removes only what it declared. `remove cell/3/4` sent to another instance is refused there, with `refused cell/3/4 unknown address 'cell/3/4'` on its left outlet, and the parameter stays. Each instance replays its own declarations on every `ready`, so a removal made by another instance would not last. To act on a parameter declared elsewhere, `bind` it and `set` it; to remove it, remove it from the script that declared it.

**The name argument.** The argument is optional and chooses where the parameters go.

- `[ossia.v8connector synth]`: the argument names a node that holds the instance's parameters, like the name of an `ossia.model`. Under the enclosing `ossia.model` or `ossia.device` (or the global device), `add gain` creates `synth/gain`. It keeps the parameters of each instance apart: a second instance named `synth` in the same place becomes `synth.1`.
- `[ossia.v8connector]`: no node of its own. `add gain` creates `gain` directly under the node of the enclosing `ossia.model`, or the root of the enclosing `ossia.device` or `ossia.client`, or the root of the global device, next to the `ossia.parameter` objects. An address already taken, by another instance or by an `ossia.parameter`, is refused with `refused gain already exists` and never renamed (an `ossia.parameter` in the same case creates `gain.1`). The instance removes only its own parameters, and only the nodes they created.

`bind` does not go through the instance's node in either mode: its address is relative to the enclosing model or device, as for `ossia.remote`, so any instance follows `synth/gain` with `bind synth/gain`.

#### Bundles

With `@bundle 1` (the default), the values an instance sets during a tick leave together at the end of the tick, in OSC bundles: to a bound server through the client's OSCQuery mirror, which splits them into datagrams of at most 1400 bytes over UDP and sends one bundle over the WebSocket (`@critical`); to the clients of the device that serves the declared parameters, in bundles of at most 1400 bytes. A parameter set several times in a tick leaves once, with its last value; with `@repetitions 0`, a value equal to the current one does not leave. `@bundle 0` sends one message per value, as `2.1.0+prism.4` did, for comparison or for a peer that would refuse bundles. TouchDesigner's OSCQuery component (prism-td-oscquery) decodes bundles, as server and as client.

Load bench (`v8connector_load` in `tests/Network/V8ConnectorTest.cpp`, run by hand): a server in another process, 3000 leaves bound through a client by URL on the same Mac, the time of each value measured from the start of the tick that set it to its arrival on the server's UDP port.

| 3000 leaves, 60 ticks per second, 30 s | `@bundle 1` | `@bundle 0` | `@bundle 1`, 1 % change |
|---|---|---|---|
| values per tick | 3000 | 3000 | 30 |
| datagrams per tick | 53 | 3000 | 1 |
| bytes per tick (OSC payload) | 72 848 | 60 000 | 736 |
| largest datagram | 1384 | 20 | 736 |
| latency p50 / p99 | 1.65 / 2.30 ms | 4.19 / 9.79 ms | 0.78 / 1.39 ms |
| lost values | 0 | 0 | 0 |
| time in the tick p50 / p99 / max | 1.81 / 2.52 / 29.47 ms | 8.58 / 11.54 / 23.63 ms | 0.81 / 1.30 / 4.99 ms |

The objects of ossia-max send bundles too. What `ossia.parameter`, `ossia.remote` or `ossia.view` push during a tick of Max's scheduler leaves at the end of the tick, in OSC bundles, through the protocol of their `ossia.device` or `ossia.client`: a client's mirror splits them under 1400 bytes over UDP, a device's OSCQuery server sends them to its clients in bundles of at most 1400 bytes. Nothing is merged: two values of one address in a tick both leave, in order, impulses included; the repetition filter of a parameter applies as before. `@bundle` on `ossia.device` and `ossia.client`: -1 (the default) bundles when the device speaks OSCQuery, 0 never (one message per value, as `2.1.0+prism.4`), 1 always; plain OSC and Minuit are not bundled by default. The queue is emptied by a clock of the device object, at the end of the tick, on the scheduler thread when Overdrive is on, else on the main thread. `ossia.v8connector` keeps its own queue and calls the protocol itself: its values never enter the queue of the device, so they are not grouped twice; its `@bundle` decides for them.

Load bench, 3000 parameters pushed as `ossia.remote` pushes them, through a client by URL (`OSSIA_LOAD_MODE=remote1` and `remote0`):

| 3000 values per tick, 60 ticks per second, 30 s | client `@bundle 1` | client `@bundle 0` |
|---|---|---|
| datagrams per tick | 53 | 3000 |
| bytes per tick (OSC payload) | 72 848 | 60 000 |
| largest datagram | 1384 | 20 |
| latency p50 / p99 | 0.79 / 1.46 ms | 3.59 / 8.98 ms |
| lost values | 0 | 0 |
| time in the tick p50 / p99 / max | 0.83 / 1.47 / 4.05 ms | 6.83 / 9.69 / 30.56 ms |

`examples/ossia.v8connector.load.maxpat` shows the same load in Max: an emitter sets 3000 leaves at each frame, a client in the same Max follows them and draws them as a cloud of points, with a toggle for `@bundle`.

### Demo and help

The Extras menu of Max lists `ossia-prism examples`, which opens each example of `ossia.v8connector` (demo, grid, load) in one click. The demo patcher `examples/ossia.v8connector.demo.maxpat` mixes, in one tree, an `ossia.parameter` placed by hand, a `v8ui` that declares its parameters through the object and a second `v8ui` that binds to them, with `ossia.remote` objects listening to the same addresses.

![Demo patcher](docs/prism/demo.png)

The help patcher (right click on the object, Open Help) has three tabs: Basic, Script and Limits.

![Help, Basic tab](docs/prism/help1.png)

![Help, Script tab](docs/prism/help2.png)

![Help, Limits tab](docs/prism/help3.png)

### Differences from ossia 2.1.0

State on 2026-10-30, `2.1.0+prism.5`. "Fixed" means fixed in ossia-prism, not in ossia.

Changed in ossia-prism:

- **One `EXTENDED_TYPE` per parameter** (OSCQuery server, `src/ossia/network/oscquery/detail/json_writer_detail.cpp`, every object that publishes a parameter). libossia 2.1.0 describes a color by the `EXTENDED_TYPE` array of its components (`color.rgb.r`, `color.rgb.g`, ...), then writes the node's own extended type (`vecf`) as a second `EXTENDED_TYPE` key in the same object; a client that keeps the last duplicate key sees `vecf` and loses the color. In ossia-prism the node's extended type is not written when the parameter has a unit. Colors are still described by that array, not by `UNIT`; a parameter without a unit keeps its own extended type.
- **`ossia.client` connected by name applies the tree changes** (`src/ossia-max/src/client.cpp`, `client_mirror.hpp`). In ossia 2.1.0, `connect <name>` (zeroconf) gets the tree once; the changes the server sends afterwards (`PATH_ADDED`, `PATH_REMOVED`, `ATTRIBUTES_CHANGED`) are only applied for a client connected by URL (`connect oscquery ws://host:port`), so a parameter added on the server never appears. ossia-prism polls the mirror in both cases, and removes a path the server removes, as for a connection by URL.
- **`ossia.client` connected by URL reconnects** (`client.cpp`, `client_mirror.hpp`). In ossia 2.1.0, once the server's WebSocket goes down (the server stopped), the client receives nothing more, reports nothing and never reconnects, while its own sends still leave over UDP. ossia-prism reports the loss with `connect 0 <parameters>` on the outlet, tries again every second on the main thread until the server answers on its port, connects as the first time (tree read again, every followed parameter subscribes again with `LISTEN`), then sends `connect 1 <parameters>`. `@reconnect 0` turns the attempts off.
- **Bundles** (`2.1.0+prism.5`). In ossia 2.1.0 the OSCQuery client's `push_bundle` and `push_raw_bundle` are TODOs that send nothing (`src/ossia/network/oscquery/oscquery_mirror.cpp`), and a device exposed through a multiplex (as `ossia.device` does) pushes a bundle one value at a time (`src/ossia/network/local/local.cpp`), so a script's values left one message each. Three commits of the core, `oscquery: the client sends bundles`, `oscquery: a multiplexed device forwards bundles to its protocols` and `oscquery: bundles of values, in which a parameter may come several times`, make them send OSC bundles; `ossia.v8connector` groups the values of a tick (its `@bundle`), and `ossia.device` and `ossia.client` group what their objects push during a tick (their `@bundle`, on by default for OSCQuery).
- **`ossia.client`: the poll and `connect` never run at once** (`2.1.0+prism.5`, `client.cpp`, `client_mirror.hpp`). In ossia 2.1.0 the client applies its mirror's commands from a clock, on the scheduler thread with Overdrive, while `connect` and `disconnect` destroy the device on the main thread: a click on `connect` could crash Max. The poll, `connect`, `disconnect` and the end of a connection now enter Max's critical region, and the link waits for a running poll before the device goes.
- **`[ossia]` frees its ports when deleted** (`2.1.0+prism.5`, `ossia_object.cpp`, `device_network.hpp`). In ossia 2.1.0 the servers exposed by `[ossia]` stay on the global device after the object is deleted, their ports taken. They are now stopped with the object.
- **`ossia.client` by URL does not hold Max** (`2.1.0+prism.5`, `client.cpp`, `client_mirror.hpp`). In ossia 2.1.0, `connect oscquery <url>` builds the mirror on the main thread, which waits up to 1.5 s for the WebSocket and up to 3 s for the tree: Max freezes during each attempt, and an `ossia.device` of the same Max, whose server is polled by that same thread, can never answer. The connection now runs on a thread of its own; the outlet sends `connect 1 ...` when it is done.
- **`@clip none`**, the OSCQuery name of the free mode, is accepted by `ossia.v8connector` only; `ossia.parameter` still expects `free`.
- **An address already taken** is refused by `ossia.v8connector` without a name (`refused <address> already exists`); `ossia.parameter`, in the same case, creates a sibling with a free name (`gain.1`).
- Added: the `ossia.v8connector` object, the package name `ossia-prism`, the version `2.1.0+prism.N`.

Fixed in `ossia.v8connector`:

- **Crash when a followed device comes back** (`2.1.0+prism.4`). Max crashed twice in `ossia.v8connector`, in a patch whose `bind` addresses point to a remote device, while the object registered again. `ossia.client` clears its device before it announces its removal, so the object kept a pointer to a destroyed device and used it at the next registration; under AddressSanitizer the bench shows that use after free when a followed device is destroyed and a new one arrives under the same name. The object now notices the destruction of every device it knows by itself. The crash itself has not been reproduced in Max.

Known defects, not fixed:

- The OSCQuery server of libossia (`oscquery_asio::oscquery_server_protocol`, the one of `ossia.device`) frees its sockets before they are closed when it is destroyed while its network thread runs: the destructor (`src/ossia/protocols/oscquery/oscquery_server_asio.cpp:856`) posts the close of its UDP socket to that thread, then frees the socket before the close has run. AddressSanitizer reports this use after free in a test that stops a server and exposes another one in the same process; destroyed from its own network thread, the server blocks instead. Not observed as such in Max.
- Sending `expose` on a port already in use, including a second `expose`, crashes Max (OSCQuery server of `ossia.device`). Use explicit, free ports, and `stop` before exposing again.
- The server logs `remote_endpoint: Invalid argument` at each connection, with no effect seen on the served tree.
- The access mode is not enforced by the server: a read-only parameter can be written over OSC.
- The client cannot read some OSCQuery servers that are not ossia: it asks `?HOST_INFO=1` instead of `?HOST_INFO`, and does not read `chunked` HTTP replies.
- On Windows, the automatic discovery of OSCQuery devices (mDNS) depends on the Bonjour service being installed and running; without it, connect `ossia.client` to an explicit address.

### Known limits

- `@priority` is stored on the node but does not order the outputs of `ossia.v8connector`: ossia-max orders the outputs of `ossia.parameter` and `ossia.remote` only.
- An `ossia.model` or `ossia.view` added to the patcher after the object has registered does not take it under its node: send `address <name>` to the object, or reload the patcher.
- `ready` and `refused` are reserved addresses; inside one instance an address is either declared by `add` or followed by `bind`.
- Two instances with the same name become `name` and `name.1`, like two `ossia.model`.
- The defects of ossia that ossia-prism does not fix are listed under [Differences from ossia 2.1.0](#differences-from-ossia-210).

### Building

```
cmake -S . -B build-ossia-max -GNinja -DCMAKE_BUILD_TYPE=Release -DOSSIA_MAX_ONLY=1 -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64"
cmake --build build-ossia-max --target install
```

The package lands in `ossia-max-package/ossia-prism` under the build directory, or where `OSSIA_MAX_INSTALL_FOLDER` points. The headless test of the object's core is `tests/Network/V8ConnectorTest.cpp`, built with `OSSIA_TESTING`; the tests of the helper and of the grid example's script are `tests/Network/V8ConnectorHelperTest.js` and `V8ConnectorGridExampleTest.js`, run with Node.

The text below is the upstream libossia README, unchanged.

---

libossia
===

libossia is a modern C++, cross-environment distributed object model for creative coding.

It allows to declare the architecture of your creative coding application's functions as a **tree of OSC nodes and parameters**. These nodes/parameters can have attributes, which allow to declare many of their properties, such as their values, types, units, ranges, etc....

This OSC tree-based architecture (coined "device" in the OSSIA terminology) can then be exposed over the network under several protocols, some of which allow this architecture, and the properties and values of its nodes, to be fully explored and queried. For now, protocols available in the implementations are: plain OSC, OSCquery, and Minuit - more are part of libossia and will be made available in the future.

libossia offers bindings and implementations for several environments: PureData, Max/MSP, Python, C, C++, openframeworks, Unity3D, QML, Faust, SuperCollider.

You can find some documentation for the library and its various implementations is at https://ossia.io

Please feel free to ask questions on the [forum](http://forum.ossia.io/c/libossia), or to come and chat with us ! [![Gitter](https://badges.gitter.im/ossia/libossia.svg)](https://gitter.im/ossia/libossia?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge)

More about the general project at: https://ossia.io

Download & Build Status
=======================
* Latest release : https://github.com/ossia/libossia/releases/latest

| Binding | Build Status | Latest Build from Master Branch |
|-|-|-|
| <img src="https://upload.wikimedia.org/wikipedia/commons/thumb/9/93/Logo_Max_8_software.jpg/244px-Logo_Max_8_software.jpg" alt="Max" height="20"/> | [![Build Status](https://github.com/ossia/libossia/actions/workflows/ossia-max.yml/badge.svg)](https://github.com/ossia/libossia/actions/workflows/ossia-max.yml) | https://nightly.link/ossia/libossia/workflows/ossia-max/master/ossia-max.zip |
| <img src="https://patchstorage.com/wp-content/themes/bayside/images/logo-pd-vanilla.png" alt="Pd" height="20"/> | [![Build Status](https://github.com/ossia/libossia/actions/workflows/ossia-pd.yml/badge.svg)](https://github.com/ossia/libossia/actions/workflows/ossia-pd.yml) | |
| <img src="https://unity3d.com/profiles/unity3d/themes/unity/images/pages/branding_trademarks/unity-masterbrand-black.png" alt="Unity3D" height="20"/> | [![Build Status](https://github.com/ossia/libossia/actions/workflows/ossia-unity.yml/badge.svg)](https://github.com/ossia/libossia/actions/workflows/ossia-unity.yml) | |
| | [![Build Status](https://github.com/ossia/libossia/actions/workflows/benchmarks.yml/badge.svg)](https://github.com/ossia/libossia/actions/workflows/benchmarks.yml) | |
| | [![Build Status](https://github.com/ossia/libossia/actions/workflows/doc.yml/badge.svg)](https://github.com/ossia/libossia/actions/workflows/doc.yml) | |
| | [![Build Status](https://github.com/ossia/libossia/actions/workflows/coverage.yml/badge.svg)](https://github.com/ossia/libossia/actions/workflows/coverage.yml) | |
| Coveralls | [![Coverage Status](https://coveralls.io/repos/github/ossia/libossia/badge.svg?branch=master)](https://coveralls.io/github/ossia/libossia?branch=master) | |

Documentation
=============
* [Lib and Implementations](https://ossia.github.io/ossia-docs)
* [Examples](https://github.com/ossia/libossia/tree/master/examples)
* [Doxygen](http://ossia.github.io/libossia/html)
* [Building](https://github.com/ossia/libossia/wiki/Building)
* [Code style](https://github.com/ossia/libossia/wiki/Code-style-guide)

License
=======
Available under both LGPLv3 and CeCILL-C

[![FOSSA Status](https://app.fossa.io/api/projects/git%2Bhttps%3A%2F%2Fgithub.com%2FOSSIA%2Flibossia.svg?type=shield)](https://app.fossa.io/projects/git%2Bhttps%3A%2F%2Fgithub.com%2FOSSIA%2Flibossia?ref=badge_shield)
