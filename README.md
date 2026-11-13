ossia-prism (PRISM, CNRS)
===

PRISM's fork of [libossia](https://ossia.io), which exposes the parameters of an application over OSCQuery. It builds two packages under one version: **ossia-prism** for Max and **ossia (PRISM)** for Unity.

Julien Bayle, PRISM (CNRS), https://www.prism.cnrs.fr/

| Package | Latest version | Documentation | Changes | Binaries |
|---|---|---|---|---|
| ossia-prism (Max) | `2.1.0+prism.14` | [this README](#ossia-prism-for-max) | [CHANGELOG](CHANGELOG.md) | [Release `v2.1.0+prism.14`](https://gitlab.prism.cnrs.fr/labo/outils/libossia/-/releases/v2.1.0%2Bprism.14) |
| ossia (PRISM), `io.ossia.ossia-unity` (Unity) | `2.1.0+prism.14` | [README](src/ossia-unity3d/README.md) | [CHANGELOG](CHANGELOG.md) | [Release `v2.1.0+prism.14`](https://gitlab.prism.cnrs.fr/labo/outils/libossia/-/releases/v2.1.0%2Bprism.14) |

Both are published for macOS universal (Apple Silicon and Intel); Windows x64 to come, same version, added to the Release later. Every version is on the [Releases](https://gitlab.prism.cnrs.fr/labo/outils/libossia/-/releases) page. Questions and defects: the [issues](https://gitlab.prism.cnrs.fr/labo/outils/libossia/-/issues) of this repository.

### ossia-prism for Max

**ossia-prism** is the Max package built from this repository: [ossia](https://ossia.io) 2.1.0 for Max (every `ossia.*` object of the official package, unchanged except for the changes and fixes listed under [Differences from ossia 2.1.0](#differences-from-ossia-210)) plus one object, **`ossia.v8connector`**, which lets a `v8` or `v8ui` script declare its own parameters. It replaces the official `ossia` package: both contain the same objects and cannot be installed together.

Prerequisites: Max 9 (`v8` and `v8ui` are objects of Max 9), macOS. Tested on macOS 15 Apple Silicon with Max 9.2.0. Not tested: Intel Macs (the package is universal, its x86_64 part has not run), Windows (not built yet), Max 8 and earlier.

#### Installation

1. Download `ossia-prism-2.1.0+prism.N-macos.zip` from the [Releases](https://gitlab.prism.cnrs.fr/labo/outils/libossia/-/releases) page.
2. Quit Max.
3. Remove the official `ossia` package if it is installed: Max's Package Manager, `ossia`, Uninstall, or delete the folder `~/Documents/Max 9/Packages/ossia`.
4. Unzip the archive into `~/Documents/Max 9/Packages/`, which gives `~/Documents/Max 9/Packages/ossia-prism`.
5. The package is not signed by Apple: in the Terminal, run

   ```
   xattr -dr com.apple.quarantine ~/Documents/Max\ 9/Packages/ossia-prism
   ```

6. Start Max, create an `ossia.v8connector` object: the Max console shows `ossia-prism 2.1.0+prism.N, PRISM CNRS`.
7. Extras menu > `ossia-prism examples` opens the examples; right click on `ossia.v8connector` > Open Help opens its help.

The package root also holds `LISEZMOI.txt`, which points to this README.

#### Update

Never update `ossia` from Max's Package Manager: it would install the official package again, next to this one. For a new version of ossia-prism, quit Max and replace the folder `~/Documents/Max 9/Packages/ossia-prism` with the one of the new archive, then run the `xattr` command again. Max's documentation on packages: https://docs.cycling74.com/userguide/packages/

### Unity quick start

Tested with Unity 6.3 LTS on macOS Apple Silicon, in the editor and in players built with Mono and IL2CPP; the rest is in the package's [README](src/ossia-unity3d/README.md).

1. Download `io.ossia.ossia-unity-2.1.0+prism.N.tgz` from the [Releases](https://gitlab.prism.cnrs.fr/labo/outils/libossia/-/releases) page.
2. Unity: Window > Package Management > Package Manager, `+` > Install package from tarball…, choose the file.
3. In the package's Samples tab, Import "Custom parameters", open the scene `CustomParameters`, Play: `curl http://localhost:5678/Shape/CustomParameters` returns the five parameters of the sample's script, and `CustomParameters.maxpat`, in the same folder, drives them from Max.

### ossia.v8connector

`[ossia.v8connector <name>]` is a node of the ossia tree whose parameters are declared by messages, typically from a `v8` or `v8ui` script, so that a script can publish its own parameters without one `ossia.parameter` object per parameter. It registers like an `ossia.model`: under the nearest `ossia.model`, `ossia.device` or `ossia.client` of the patcher hierarchy, or under the global device. Its parameters live in the same tree as the ones declared by hand, and are served by the same OSCQuery server.

#### Declaring your own parameters

The patch, three objects and one cord:

1. `[ossia.device mydevice]`, with a message box `[expose oscquery 1234 5678(` patched into it: the device's OSCQuery server, on UDP 1234 (OSC) and TCP 5678 (HTTP and WebSocket).
2. `[ossia.v8connector synth]`; in its Inspector, Scripting Name `synth`.
3. `[v8 synthparams.js]`, the script below saved as `synthparams.js` next to the patch; the left outlet of `ossia.v8connector` patched into the inlet of `v8`.

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

`require` finds `ossia.v8connector.js` in the package's `javascript` folder. The helper reaches the object through its Scripting Name, so declaring needs no cord; the cord into the script brings back the values changed elsewhere, `ready` and `refused`. Click `expose oscquery 1234 5678`, then from a terminal:

```
curl -s http://localhost:5678/synth
{"FULL_PATH":"/synth","CONTENTS":{"gain":{"FULL_PATH":"/synth/gain","TYPE":"f","VALUE":-12.0,"RANGE":[{"MIN":-60.0,"MAX":6.0}],"ACCESS":3,"CLIPMODE":"none","UNIT":["gain.db"],"DEFAULT_VALUE":-12.0},"cutoff":{"FULL_PATH":"/synth/cutoff","TYPE":"f","VALUE":0.0,"RANGE":[{"MIN":20.0,"MAX":20000.0}],"ACCESS":3,"CLIPMODE":"none","UNIT":["time.Hz"]},"voices":{"FULL_PATH":"/synth/voices","TYPE":"i","VALUE":4,"RANGE":[{"MIN":1,"MAX":16}],"ACCESS":3,"CLIPMODE":"none","DEFAULT_VALUE":4}}}
```

This output is the one of the object's core declaring these three parameters on a device exposed on the same ports (`tests/Network/V8ConnectorTest.cpp` builds the same core). A client of the device sees:

| Address | `TYPE` | `RANGE` | `UNIT` | `VALUE` at start |
|---|---|---|---|---|
| `/synth/gain` | `f` | -60 to 6 | `gain.db` | `-12.0`, the default |
| `/synth/cutoff` | `f` | 20 to 20000 | `time.Hz` | `0.0`: no default declared |
| `/synth/voices` | `i` | 1 to 16 | | `4`, the default |

Every value a client sends to `/synth/gain` comes out of the left outlet of `ossia.v8connector` as `gain <value>`, and the script's handler posts it; `bang` on the script sets `gain` to -6, which the clients that listen receive. From another Max: `[ossia.client]`, `connect mydevice` (or `connect oscquery ws://127.0.0.1:5678`), `[ossia.remote /synth/gain]`.

#### Messages

| Message | Effect |
|---|---|
| `add <address> @type ... @min ... @max ...` | creates a parameter under the node, with the attribute names of `ossia.parameter`; see [Attributes of add](#attributes-of-add). A numeric type requires `@min` and `@max`, or `@range`. A `@type` that the `@unit` would change is refused (`color.rgb` is `vec3f`, `color.rgba` is `vec4f`). `@clip none` is accepted as the free mode. Declaring the same address again with the same attributes does nothing; with other attributes, it is refused. |
| `remove <address>`, `clear` | removes one parameter, or every parameter of this instance |
| `bind <address>`, `unbind <address>` | follows a parameter that exists elsewhere (an `ossia.parameter`, another instance), addressed as an `ossia.remote` would; creates nothing, follows the parameter when it disappears and comes back |
| `set <address> <values...>` | pushes a value to a declared or bound parameter |
| `address <name>` | renames the instance's node and registers it again with its parameters |
| `namespace` | right outlet: `namespace_size <n>`, then one `namespace <address> <value>` per parameter of the node |
| `get <attribute>` | right outlet: the node attribute named (`global_address`, `description`, `tags`, `invisible`, `priority`, `recall_safe`, `zombie`) |
| left outlet | `<address> <values...>` for every value changed by another source, bounded and filtered like the outlet of every ossia object; `ready` once the object has registered; `refused <address> <reason>` for every refusal of `add`, `bind`, `set`, `remove` or `unbind` |
| right outlet | `address`, `add`, `remove`, `clear`, `bind <address> <count>`, `unbind`, `error`, and the replies to `namespace` and `get` |

#### Attributes of the object

| Attribute | Default | Role |
|---|---|---|
| `@bundle` | 1 | sends the values set during a tick together, in OSC bundles; 0, one message per value (see [Bundles](#bundles)) |
| `@priority` | 0 | stored on the node; does not order the outputs (see [Known limits](#known-limits)) |
| `@description` | none | description of the node |
| `@tags` | none | tags of the node |
| `@invisible` | 0 | hides the node from remote clients |
| `@recall_safe` | 0 | keeps the node's parameters out of preset recalls |

The node attributes (`@priority` to `@recall_safe`) apply to the node of a named instance; an instance without a name has no node of its own and ignores them.

#### Attributes of add

| Attribute | Values | Default |
|---|---|---|
| `@type` (required) | `float`, `int`, `bool`, `impulse`, `string`, `list`, `vec2f`, `vec3f`, `vec4f` | |
| `@min`, `@max` | one number, or one per component for a vector | required for `float`, `int`, `vec2f`, `vec3f`, `vec4f` |
| `@range` | `<min> <max>`, or `<min> <max>` per component; for a `string`, the list of accepted symbols | |
| `@unit` | an ossia unit (`gain.db`, `time.hz`, `color.rgb`, `position.cart3D`, ...) | none |
| `@default` | a value of the type | none |
| `@clip` | `free` (or `none`), `both`, `low`, `high`, `wrap`, `fold` | `free` |
| `@mode` | `bi` (or `rw`), `get` (or `r`), `set` (or `w`) | `bi` |
| `@description` | one symbol | none |
| `@tags` | symbols | none |
| `@priority`, `@rate` | a number | none |
| `@critical`, `@invisible`, `@mute`, `@recall_safe` | 0 or 1 | 0 |
| `@repetitions`, `@enable` | 0 or 1 | 1 |

The access mode is published but not enforced by the server (see [Differences from ossia 2.1.0](#differences-from-ossia-210)).

#### The helper, javascript/ossia.v8connector.js

| Call | Effect |
|---|---|
| `new OssiaConnector("<scripting name>")` | the connector for the object with that Scripting Name, searched in the script's patcher and the patchers above it; or `new OssiaConnector(<Maxobj>)` |
| `declare({ <address>: { type, min, max, range, unit, default, ... } })` | sends `add` for each address, with the attributes of [Attributes of add](#attributes-of-add) without `@` |
| `add(address, attrs)`, `remove(address)`, `clear()` | one declaration, one removal, all of them |
| `bind(address)`, `unbind(address)` | follows, or stops following, a parameter declared elsewhere |
| `set(address, ...values)` | pushes a value |
| `on(address, function)`, `off(address)` | calls the function with the values received for that address, or stops |
| `dispatch(messagename, args)` | to call from the script's `anything()`: routes what the object's left outlet sends to the handlers, replays the declarations on `ready` |
| `replay()` | sends the declarations and bindings again, as on `ready` |
| `onRefused = function (address, reason)` | called for every `refused`; each refusal is also posted to the Max console |

`synth.bind("other/gain")` follows a parameter declared elsewhere instead of creating one. The helper keeps the declarations and replays them when the object sends `ready`, so the object can be deleted and recreated.

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

With `@bundle 1` (the default), the values an instance sets during a tick leave together at the end of the tick, in OSC bundles: to a bound server through the client's OSCQuery mirror, which splits them into datagrams of at most 1400 bytes over UDP and sends one bundle over the WebSocket (`@critical`); to the clients of the device that serves the declared parameters, in bundles of at most 1400 bytes. A parameter set several times in a tick leaves once, with its last value; with `@repetitions 0`, a value equal to the current one does not leave. That is right for a state (a level, a position), where the last value is the one that counts, and wrong for events or triggers, where each value counts: for them, `@bundle 0` is the choice. Measured with TouchDesigner's OSCQuery component, `2.1.0+prism.10`, a script setting 128 leaves at 60 Hz, three runs of 60 s each way: with `@bundle 1`, 4, 34 and 107 batches of about 3600 never left Max, and an independent WebSocket client saw exactly what TouchDesigner received; with `@bundle 0`, none. The merge was not observed inside Max: it follows from the code (a parameter is queued once in a tick and sent with the value it holds at the end of the tick) and from those values missing on the socket. `@bundle 0` sends one message per value, as `2.1.0+prism.4` did, for comparison or for a peer that would refuse bundles. TouchDesigner's OSCQuery component (prism-td-oscquery) decodes bundles, as server and as client.

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

The help patcher (right click on the object, Open Help) has three tabs: Basic, Script and Limits. The grid example exposes its device on ports 7101 and 7102 and the load example on 7301 and 7302: click `expose` once, and close one before opening it again (see [Known limits](#known-limits)).

![Help, Basic tab](docs/prism/help1.png)

![Help, Script tab](docs/prism/help2.png)

![Help, Limits tab](docs/prism/help3.png)

### What PRISM adds to ossia 2.1.0

Everything `2.1.0+prism.1` to `2.1.0+prism.14` add to ossia 2.1.0, one line per function: the fixes are here too, since a function that now works is what they add. **Since** is the version that brought it, from the [CHANGELOG](CHANGELOG.md) (`.N` for `2.1.0+prism.N`). `.N (unreleased)` marks what the sources hold and no Release has yet; its section of the CHANGELOG is `[Unreleased]`. **Documentation**: a section of this README or of the [Unity README](src/ossia-unity3d/README.md), or the help or the reference page of the object in Max (`src/ossia-max/help`, `src/ossia-max/docs/refpages`). **Proven by**: a test of this repository: a name in `tests/C/unit-tests/unit-tests-c-network.cpp` (`C API: ...`), `tests/Network/V8ConnectorTest.cpp`, `tests/Network/OSCQueryTest.cpp` or `tests/Network/OSCQueryColorTest.cpp`, a line of `tests/Max/check_max.py`, or `Class.Method` under `src/ossia-unity3d/Tests`. "No automated test" says so where a function has none.

#### Max package

| Function | Object, attribute or API | Since | Documentation | Proven by |
|---|---|---|---|---|
| A node of the tree whose parameters a `v8` or `v8ui` script declares | `ossia.v8connector`: `add`, `remove`, `clear`, `set` | .1 | [Declaring your own parameters](#declaring-your-own-parameters), [Messages](#messages), [Attributes of the object](#attributes-of-the-object), [Attributes of add](#attributes-of-add); reference of `ossia.v8connector`; help of `ossia.v8connector`, Basic | `v8connector_declarations`, `v8connector_tree`, `v8connector_node_on_first_add` |
| Follow a parameter that exists elsewhere, without creating anything | `bind`, `unbind` | .1 | [Messages](#messages); reference of `ossia.v8connector` | `v8connector_bind` |
| Values changed by another source come out bounded and filtered; a corrected value goes back to its sender; `ready` once registered | left outlet of `ossia.v8connector`, `ready` | .1 | [Messages](#messages); reference of `ossia.v8connector` | `v8connector_declarations`, `v8connector_device_return`, `tests/Network/V8ConnectorHelperTest.js` |
| A helper for scripts, which replays its declarations on `ready` | `javascript/ossia.v8connector.js`: `declare`, `add`, `remove`, `set`, `bind`, `on`, `dispatch` | .1 | [The helper](#the-helper-javascriptossiav8connectorjs); help of `ossia.v8connector`, Script | `tests/Network/V8ConnectorHelperTest.js` |
| Help, demo and reference of the object | `help/ossia.v8connector.maxhelp`, `examples/ossia.v8connector.demo.maxpat`, `docs/refpages/ossia.v8connector.maxref.xml` | .1 | [Demo and help](#demo-and-help) | no automated test (the headless core: `tests/Network/V8ConnectorTest.cpp`) |
| The package has its own name and version, next to none of the official package | package `ossia-prism`, the line `ossia-prism 2.1.0+prism.N, PRISM CNRS` in the console | .2 | [ossia-prism for Max](#ossia-prism-for-max), [Installation](#installation) | no automated test (the console line, read in Max) |
| A tab of the limits in the help | `ossia.v8connector` help, Limits | .2 | help of `ossia.v8connector`, Limits | no automated test |
| `add` refuses a `@type` that its `@unit` would change, with the reason | `add` | .3 | [Attributes of add](#attributes-of-add) | `v8connector_units_clip_refusals` |
| Every refusal also comes out as a message | `refused <address> <reason>`, left outlet | .3 | [Messages](#messages), [Known limits](#known-limits) | `v8connector_units_clip_refusals`, `tests/Network/V8ConnectorHelperTest.js` |
| The OSCQuery name of the free mode | `@clip none` on `add` | .3 | [Attributes of add](#attributes-of-add), [Differences from ossia 2.1.0](#differences-from-ossia-210) | `v8connector_units_clip_refusals` |
| The helper reports refusals | `onRefused(address, reason)` | .3 | [The helper](#the-helper-javascriptossiav8connectorjs) | `tests/Network/V8ConnectorHelperTest.js` |
| Adding and removing from a script, which instance owns a parameter, the role of the name | recipes | .3 | [Recipes](#recipes); `docs/prism/LISEZMOI.txt` | no automated test (documentation) |
| Parameters directly under the enclosing node, an address taken refused and never renamed | `[ossia.v8connector]` without an argument | .4 | [Recipes](#recipes); reference of `ossia.v8connector` | `v8connector_without_name`, `v8connector_without_name_device_root` |
| A client by URL reports the loss of its server and connects again, every second; off with `@reconnect 0` | `ossia.client`: `@reconnect`, `connect 0 <parameters>` | .4 | [Differences from ossia 2.1.0](#differences-from-ossia-210); reference of `ossia.client`; help of `ossia.client`, PRISM | `v8connector_client_reconnects`, `@reconnect 0: a failed first connection is not tried again` |
| The object survives a device it follows going away and coming back | `bind` of `ossia.v8connector` | .4 | [Differences from ossia 2.1.0](#differences-from-ossia-210) | `v8connector_device_return`, `v8connector_parameter_recreated` |
| A client by name applies the tree changes of the server | `ossia.client`, `connect <name>` | .4 | [Differences from ossia 2.1.0](#differences-from-ossia-210) | `v8connector_client_by_name` |
| Values pushed during a tick leave at its end in OSC bundles of at most 1400 bytes | `@bundle` of `ossia.device` and `ossia.client` (default -1: on over OSCQuery) | .5 | [Bundles](#bundles); reference of `ossia.device`; reference of `ossia.client`; help of `ossia.device`, PRISM; help of `ossia.client`, PRISM | `device_bundles`, `device_bundles_default` |
| The values of an instance leave together; a parameter set several times leaves once | `@bundle` of `ossia.v8connector` (default 1) | .5 | [Bundles](#bundles); reference of `ossia.v8connector` | `v8connector_bundles`, `v8connector_bundles_served_leaves` |
| 3000 parameters set at each frame, drawn as a cloud of points | `examples/ossia.v8connector.load.maxpat` | .5 | [Bundles](#bundles), [Demo and help](#demo-and-help) | `tests/Network/V8ConnectorLoadExampleTest.js` |
| Opens each example in one click | Extras menu, `ossia-prism examples` | .5 | [Demo and help](#demo-and-help) | no automated test |
| The load bench | `v8connector_load`, run by hand | .5 | [Bundles](#bundles) | `v8connector_load` |
| A client by URL does not hold Max, and reaches a device of the same Max | `ossia.client`, `connect oscquery <url>` | .5 | [Differences from ossia 2.1.0](#differences-from-ossia-210); reference of `ossia.client` | `v8connector_client_connects_to_same_process_server`, `oscquery_mirror_on_the_polling_thread_fails` |
| The poll of a client and `connect` never run at once | `ossia.client` | .5 | [Differences from ossia 2.1.0](#differences-from-ossia-210) | `oscquery_link_unwatch_waits_for_tick` |
| Deleting `[ossia]` frees its ports | `[ossia]` | .5 | [Differences from ossia 2.1.0](#differences-from-ossia-210) | `ossia_object_delete_frees_its_ports` |
| `expose` on `[ossia]` no longer crashes Max | `[ossia]`, `expose` | .5 | CHANGELOG, `2.1.0+prism.5` | `ossia_object_expose_oscquery` |
| Thousands of binds and of set addresses, without freezing Max | `ossia.v8connector`, `bind`, `set` | .5 | CHANGELOG, `2.1.0+prism.5` | `v8connector_binds_follow_a_burst_of_leaves`, `v8connector_load` |
| The name a device is announced under; a warning when it is taken | `ossia.device`, `announced <name>` on the outlet | .6 | [Differences from ossia 2.1.0](#differences-from-ossia-210); reference of `ossia.device`; help of `ossia.device`, PRISM | `oscquery_directory_announced_name` |
| A client by name sees the WebSocket of its device go down, and tries again | `ossia.client`, `connect <name>`, `disconnected`, `connect 0 <name>` | .6 | [Discovery by name](#discovery-by-name); reference of `ossia.client`; help of `ossia.client`, PRISM | `oscquery_directory_follows_the_websocket` |
| A client by name finds a device of the same Max, and retries a failed build | `ossia.client`, `connect <name>` | .6 | [Differences from ossia 2.1.0](#differences-from-ossia-210), [Discovery by name](#discovery-by-name) | `oscquery_directory_finds_its_own_device`, `oscquery_directory_retries_a_failed_build` |
| `stop <index>` no longer quits Max | `ossia.device`, `stop <index>` | .6 | [Differences from ossia 2.1.0](#differences-from-ossia-210); help of `ossia.device`, PRISM | `oscquery_server_stop_then_poll`, `oscquery_server_stopped_while_polled_with_a_client` |
| A bound address no longer follows two devices when its client changes device | `ossia.v8connector`, `bind` | .6 | [Differences from ossia 2.1.0](#differences-from-ossia-210) | `v8connector_context_change_leaves_a_shared_device` |
| A second `expose` on busy ports reports it, instead of crashing Max | `ossia.device`, `expose 0 oscquery <ports>` on the outlet | .9 | [Differences from ossia 2.1.0](#differences-from-ossia-210), [Troubleshooting](#troubleshooting) | `second server on the same ports reports it` |
| A PRISM tab in the helps of the device and the client, with examples | help of `ossia.device`, help of `ossia.client` | .9 | help of `ossia.device`, PRISM; help of `ossia.client`, PRISM | no automated test (patchers) |
| Taking control: the values of the network wait after a value comes in by the inlet | `@hold <ms>` of `ossia.parameter` and `ossia.remote` | .10 | [Differences from ossia 2.1.0](#differences-from-ossia-210); reference of `ossia.parameter`; reference of `ossia.remote`; help of `ossia.parameter`, PRISM | `@hold 500: held, then out` |
| Listen on one address, not announced | `ossia.device`, `expose oscquery <OSC port> <WebSocket port> <address>` | .10 | [Differences from ossia 2.1.0](#differences-from-ossia-210); reference of `ossia.device` | `expose on 127.0.0.1: reached there`, `expose on 127.0.0.1: refused through another interface`, `expose on 127.0.0.1: not announced, the others are` |
| A page that builds a control for each parameter of a device from its tree | `extras/ossia.panel.html`, `examples/ossia.panel.maxpat` | .10 | [Differences from ossia 2.1.0](#differences-from-ossia-210) | `tests/Network/OssiaPanelTest.js` |
| A PRISM tab in the help of the parameter | help of `ossia.parameter` | .10 | help of `ossia.parameter`, PRISM | no automated test (patcher) |
| A name given at creation connects nothing | `[ossia.client <name>]` | .10 | [Discovery by name](#discovery-by-name); reference of `ossia.client`; help of `ossia.client`, PRISM | `[ossia.client <name>] does not loop` |
| A client by name keeps the tree of the listener: nothing removed and created again | `ossia.client`, `connect <name>` | .10 | [Discovery by name](#discovery-by-name); reference of `ossia.client`; help of `ossia.client`, PRISM | `connect <name>: the tree is not read again` |
| `@reconnect 0` also stops the attempts of a first connection that fails | `ossia.client`, `@reconnect` | .10 | [Differences from ossia 2.1.0](#differences-from-ossia-210); reference of `ossia.client`; help of `ossia.client`, PRISM | `@reconnect 0: a failed first connection is not tried again` |
| What the objects of `[ossia]` push during a tick leaves in bundles | `[ossia]`, `@bundle` | .10 | [Differences from ossia 2.1.0](#differences-from-ossia-210) | `[ossia] @bundle 1: one frame for a tick` |
| `dump` sends every attribute by the right outlet | `dump` of `ossia.parameter` and `ossia.remote` | .10 | [Differences from ossia 2.1.0](#differences-from-ossia-210); reference of `ossia.parameter`; reference of `ossia.remote`; help of `ossia.parameter`, PRISM | `dump on ossia.remote writes no value` |
| A bounded list carries the range on every component | `@min`, `@max` on a list parameter | .10 | [Differences from ossia 2.1.0](#differences-from-ossia-210) | `vec3f @min 0 @max 1: a range per component`, `test_oscquery_one_bound_for_each_component` |
| A Minuit device that leaves the network does not freeze Max | the listener of `_minuit._tcp`, `ZeroconfMinuitListener.cpp` | .11 | [Differences from ossia 2.1.0](#differences-from-ossia-210) | `tests/Max/check_minuit.py` |
| A Minuit device that comes does not make Max announce services without end | the listener of `_minuit._tcp`, `client_mirror.cpp`, `minuit.cpp` | .11 | [Differences from ossia 2.1.0](#differences-from-ossia-210) | `minuit_listener_device_does_not_announce`, `tests/Max/check_minuit.py` |
| The connections the package opens to announced devices name the package (`max_client_<process>`), not the device | the listener of `_oscjson._tcp`, `client_mirror.cpp`, `oscquery_mirror.cpp` | .11 | [Differences from ossia 2.1.0](#differences-from-ossia-210); [Discovery by name](#discovery-by-name); [`tools/prism-map/README.md`](tools/prism-map/README.md) | `oscquery_directory_names_its_client` |
| An `ossia.remote` gets the value of a node the server adds once | `ossia.remote`, `ossia.client`, `oscquery_mirror.cpp` | .11 | [Differences from ossia 2.1.0](#differences-from-ossia-210) | `oscquery_mirror_added_node_value_once`, `tests/Max/check_remote.py` |
| A value written to the inlet while the object outputs a value of the network is not lost | `ossia.parameter`, `object_base.cpp`, `matcher.cpp` | .11 | [Differences from ossia 2.1.0](#differences-from-ossia-210) | `tests/Max/check_outlet.py` |
| A client of Max gets only the addresses its `ossia.remote` listen to | `ossia.client`, `ossia.remote`, `oscquery_mirror.cpp` | .12 | CHANGELOG, Behavior changes | `tests/Max/check_remote.py` (`... listens to /bench/late only`, `a value no remote follows is not sent`) |
| One connection of the package to a device announced again at its address | the listener of `_oscjson._tcp`, `client_mirror.cpp` | .12 | CHANGELOG, `[2.1.0+prism.12]` | `oscquery_directory_keeps_its_build`; bench of TouchDesigner 0.3.5, 2 runs |
| `namespace` and the explorer read the value of a parameter nothing follows from the server, without waiting | `node_base.cpp`, `search_filter.cpp`, `read_followed_value` | .12 | CHANGELOG, Behavior changes | `C API: reading a mirrored parameter nothing follows asks the server` |
| A client that leaves as soon as it connects to an `ossia.device` puts no red line in the console | `ossia.device`, `expose oscquery`, `websocket_server_beast.cpp` | .12 | CHANGELOG, `[2.1.0+prism.12]` and `[2.1.0+prism.14]` | `C API: asio server: a client that leaves as it connects is no error`, `tests/Max/check_console.py` |
| An `ossia.client` whose server is closing the connection disconnects at once, with no red line | `ossia.client`, `disconnect`, `websocket_client_beast.cpp` | .12 | CHANGELOG, `[2.1.0+prism.12]` | `C API: a mirror stopped as its server closes is no error`, `tests/Max/check_console.py` |
| `get protocols` lists the OSCQuery server of an `ossia.device`, and `get oscquery_clients` its clients | `ossia.device`, `device_protocols.cpp` | .14 | CHANGELOG, `[Unreleased]` | `tests/Network/DeviceProtocolsTest.cpp`; not run in Max |

#### Unity package

| Function | Object, attribute or API | Since | Documentation | Proven by |
|---|---|---|---|---|
| The package installs and compiles from a tarball, on Apple Silicon | `io.ossia.ossia-unity`: `ossia.asmdef`, a `.meta` per file, version from the tag | .7 | [Unity README, Installation](src/ossia-unity3d/README.md#installation) | `tests/Unity/cross/doc_follow.py` |
| A player builds, Mono and IL2CPP | `PresetController.cs` | .7 | [Unity README](src/ossia-unity3d/README.md), prerequisites | `tests/Unity/cross/cross_run.py` (builds its player) |
| Name and ports of the device in the Inspector | `Controller.deviceName`, `oscPort`, `wsPort` | .7 | [Unity README, Ossia.Controller](src/ossia-unity3d/README.md#ossiacontroller) | `ControllerTests.DefaultConfiguration` |
| The scene follows the values when Unity loses focus | `Controller.runInBackground` | .7 | [Unity README, Ossia.Controller](src/ossia-unity3d/README.md#ossiacontroller), [Unity README, Troubleshooting](src/ossia-unity3d/README.md#troubleshooting) | `ControllerTests.DefaultConfiguration` (the default); the focus measured by hand |
| The name the device is announced under; a warning when it is taken | `Controller.resolveAnnouncedName`, `AnnouncedName` | .7 | [Unity README, mDNS](src/ossia-unity3d/README.md#mdns) | `AnnouncedNameTests.AnnouncedUnderItsName`, `AnnouncedNameTests.SecondDeviceOfTheSameNameReadsItsNewName` |
| A sample: a cube whose transform is exposed | sample "Exposed cube" | .7 | [Unity README, Samples](src/ossia-unity3d/README.md#samples) | `ExposedCubeTests.OscMessageMovesTheCube` |
| Tests in the package | `Tests/` | .7 | [Unity README, Tests](src/ossia-unity3d/README.md#tests) | `ValueTests.Float`, `CallbackTests.DeletedNodeAndParameterAreReleased`, `PlayCycleTests.SecondPlayGetsThePortsBack`, `MdnsTests.PlayAnnouncesStopWithdraws` |
| `Vector4` and `Quaternion` sent x, y, z, w | `Ossia.Value` | .7 | [Unity README, Accepted types](src/ossia-unity3d/README.md#accepted-types) | `ValueTests.Vector4`, `ValueTests.Quaternion`, `ValueTests.QuaternionOrderOnTheWire` |
| `Color32` is sent and read | `Ossia.Value` | .7 | [Unity README, Accepted types](src/ossia-unity3d/README.md#accepted-types) | `ValueTests.Color32` |
| A list is read as it is | `Value.GetList()` | .7 | [Unity README, Accepted types](src/ossia-unity3d/README.md#accepted-types) | `ValueTests.FloatList`, `ValueTests.IntList`, `ValueTests.ValueList`, `ValueTests.LargeValueList` |
| Every native function imported exists with that signature | `Interop.cs` | .7 | CHANGELOG, `2.1.0+prism.7` | no automated test (compared by hand with `nm`) |
| The native callbacks work in an IL2CPP player | `[MonoPInvokeCallback]` | .7 | [Unity README](src/ossia-unity3d/README.md), prerequisites | `CallbackTests.EveryMarkedCallbackIsStatic` |
| A sample: five fields of a script, with its Max patch | sample "Custom parameters", `CustomParameters.cs`, `CustomParameters.maxpat` | .8 | [Unity README, Installation](src/ossia-unity3d/README.md#installation), [Unity README, Declaring your own parameters](src/ossia-unity3d/README.md#declaring-your-own-parameters), [Unity README, Driving from Max](src/ossia-unity3d/README.md#driving-from-max) | the bench of the TouchDesigner component, `bidir_run.py`, Unity (a) |
| The value an object published is not applied back | `unity3d/impl/ParameterBindings.cs` | .8 | [Unity README, Values in both directions](src/ossia-unity3d/README.md#values-in-both-directions) | `SendUpdatesTests.FieldChangedAfterThePublicationIsKept`, `SendUpdatesTests.RotationAfterThePublicationIsKept`, `TurningObjectTests.AngleNeverGoesBack` |
| Follow a remote device, by name or by address | `Ossia.Client`: `server`, `reconnect`, `bundles`, `browseMs`, `Status` | .9 | [Unity README, Following a device](src/ossia-unity3d/README.md#following-a-device), [Unity README, Ossia.Client](src/ossia-unity3d/README.md#ossiaclient) | `ClientTests.TreeReadAndValuesBothWays`, `ClientTests.ConnectsByName`, `ClientTests.ReconnectsWhenTheServerComesBack`, `ClientTests.AddressAddedLaterBinds` |
| Bind a field or a property to an address of that device, both ways | `Ossia.Remote`: `address`, `target`, `member`, `sendUpdates` | .9 | [Unity README, Your own script](src/ossia-unity3d/README.md#your-own-script), [Unity README, Ossia.Remote](src/ossia-unity3d/README.md#ossiaremote) | `ClientTests.InspectorAndAnimationAreSent`, `ClientTests.NoEchoWhenBothWriteFor5Seconds`, `ClientTests.ChangesOfAFrameLeaveInOneBundle` |
| A sample: a scene that follows a device of Max | sample "Follow a device", `FollowADevice.maxpat` | .9 | [Unity README, The sample](src/ossia-unity3d/README.md#the-sample) | `tests/Unity/cross/doc_follow.py` |
| The clients of a Controller see each other | `Controller.forwardToClients` | .9 | [Unity README, Values in both directions](src/ossia-unity3d/README.md#values-in-both-directions), [Unity README, Ossia.Controller](src/ossia-unity3d/README.md#ossiacontroller) | `ForwardTests.TwoClientsSeeEachOther`, `ForwardTests.ForwardingOff` |
| What changes in Unity reaches the clients with no box to tick | `sendUpdates` of `ExposeAttributes` and `ExposeComponents`, on by default | .9 | [Unity README, Ossia.ExposeAttributes](src/ossia-unity3d/README.md#ossiaexposeattributes) | `ExposedObjectTests.SendUpdatesIsOnByDefault`, `ConcurrentWriteTests.InspectorAndAnimationPublish` |
| The server of ossia-max; the changes of a frame leave in bundles | `Controller.bundles` | .9 | [Unity README, Values in both directions](src/ossia-unity3d/README.md#values-in-both-directions), [Unity README, Ossia.Controller](src/ossia-unity3d/README.md#ossiacontroller) | `ServerBundleTests.ChangesOfAFrameLeaveInOneBundle` |
| A value received from a client does not leave again | `ExposeAttributes` | .9 | [Unity README, Values in both directions](src/ossia-unity3d/README.md#values-in-both-directions) | `ConcurrentWriteTests.ValuesOfTheClientAloneDoNotComeBack`, `ConcurrentWriteTests.ClientAndScriptWriteTheSameAddressFor5Seconds` |
| A Controller destroyed frees its device, its ports and its announcement | `Controller` | .9 | [Unity README, Ossia.Controller](src/ossia-unity3d/README.md#ossiacontroller) | `ControllerLifetimeTests.RecreatedControllerServesOnTheSamePorts`, `ControllerLifetimeTests.ExposedObjectOutlivesItsController` |
| The transform is published once: no two addresses that disagree | `ExposeAttributes`: `position`, `rotation`, `localScale` | .10 | [Unity README, Declaring your own parameters](src/ossia-unity3d/README.md#declaring-your-own-parameters), [Unity README, Ossia.ExposeAttributes](src/ossia-unity3d/README.md#ossiaexposeattributes); CHANGELOG, Breaking | `TurningObjectTests.TransformValueWrittenBackGivesTheOrientationRead` |
| Bounds, a unit and an access mode on a member | `[Ossia.Expose]`: `Min`, `Max`, `Unit`, `Access` | .10 | [Unity README, Limits](src/ossia-unity3d/README.md#limits), [Unity README, Ossia.Expose](src/ossia-unity3d/README.md#ossiaexpose) | `ExposeOptionsTests.BoundsUnitAndAccess` |
| The name of the member by default; inherited public fields; a vector has no unit of its own | `[Ossia.Expose]` | .10 | [Unity README, Accepted types](src/ossia-unity3d/README.md#accepted-types), [Unity README, Ossia.Expose](src/ossia-unity3d/README.md#ossiaexpose) | `ExposeOptionsTests.NameDefaultsToTheField`, `ExposeOptionsTests.FieldsOfABaseClassArePublished`, `ExposeOptionsTests.AVectorIsNotAPosition`, `ExposeOptionsTests.ExposeComponentsLogsNoLinePerProperty` |
| Listen on one address, not announced | `Controller.listenAddress` | .10 | [Unity README, Ossia.Controller](src/ossia-unity3d/README.md#ossiacontroller) | `ListenAddressTests.OnTheLoopbackOnly` |
| Taking control: an event for each value received from the network | `ExposedObject.Received`, `Remote.Received` | .10 | [Unity README, Ossia.ExposeAttributes](src/ossia-unity3d/README.md#ossiaexposeattributes), [Unity README, Ossia.Remote](src/ossia-unity3d/README.md#ossiaremote) | `ExposeOptionsTests.ReceivedForTheValuesOfAClientOnly`, `ClientTests.RemoteRaisesReceivedForTheValuesOfTheDevice` |
| A script that builds its own device counts it with the Controllers and the Clients | `Controller.Hold()`, `Controller.Let(bool playStops)` | .10 | [Unity README, Low-level API: CustomDevice.cs](src/ossia-unity3d/README.md#low-level-api-customdevicecs) | `PlayCycleTests.StopWithCustomDeviceAndAClient` |
| Leaving Play with a Client connected no longer crashes Unity | `Controller`, `Client` | .10 | [Unity README, Ossia.Client](src/ossia-unity3d/README.md#ossiaclient) | `PlayCycleTests.StopWithAClientConnected` |
| The Controller applies the value each message carries, unless a later one replaced it | `Controller`, `unity3d/impl/ParameterBindings.cs` | .10 | CHANGELOG, `2.1.0+prism.10` | `ExposeOptionsTests.AValueReplacedInTheSameFrameIsNotApplied` |
| A Remote gets the value of a node the device adds once, then once for each change | `Ossia.Remote`, `oscquery_mirror.cpp` | .11 | [Unity README, Following a device](src/ossia-unity3d/README.md#following-a-device); [Known defects](#differences-from-ossia-210) | `ClientTests.AddressAddedLaterGetsItsValueOnce` |
| A Client gets only the addresses its Remotes listen to | `Ossia.Client`, `Ossia.Remote` | .12 | [Unity README, What happens](src/ossia-unity3d/README.md#what-happens) | `ClientTests.ClientGetsOnlyWhatItsRemotesListenTo`; `tests/Unity/cross/cross_run.py` (TouchDesigner server) |
| `Parameter.GetValue()` of a parameter no Remote follows does not wait for the device | `Ossia.Parameter`, `ossia_parameter_get_value` | .12 | [Unity README, What happens](src/ossia-unity3d/README.md#what-happens) | `C API: reading a mirrored parameter nothing follows asks the server` |

#### The core, the C API and the tools

| Function | Object, attribute or API | Since | Documentation | Proven by |
|---|---|---|---|---|
| One `EXTENDED_TYPE` per parameter in the JSON of the server | `json_writer_detail.cpp` | .3 | [Differences from ossia 2.1.0](#differences-from-ossia-210) | `test_json_rgb_extended_type` |
| The OSCQuery client sends bundles | `oscquery_mirror_protocol::push_bundle`, `push_raw_bundle` | .5 | [Differences from ossia 2.1.0](#differences-from-ossia-210) | `oscquery_mirror_push_bundle` |
| A multiplexed device forwards a bundle to its protocols | `local.cpp` | .5 | [Differences from ossia 2.1.0](#differences-from-ossia-210) | `oscquery_multiplex_push_bundle` |
| Bundles of values in which a parameter may come several times | `push_bundle` of `bundle_element` | .5 | [Differences from ossia 2.1.0](#differences-from-ossia-210) | `oscquery_value_bundles` |
| A server withdraws its announcement when it stops | `oscquery_server_asio.cpp` | .6 | [Differences from ossia 2.1.0](#differences-from-ossia-210) | `oscquery_server_withdraws_on_stop` |
| List the devices announced, for a given browsing time | `ossia_oscquery_list_devices`, `ossia_oscquery_devices_free` | .7 | `src/ossia-c/ossia-c.h` | `C API: a server on 127.0.0.1 only`, `ListenAddressTests.OnTheLoopbackOnly` |
| The same, in C++ | `ossia::net::list_oscquery_devices(timeout)` | .7 | `src/ossia/network/zeroconf/zeroconf.hpp` | through `C API: a server on 127.0.0.1 only` |
| The strings the C API returns are freed as they are allocated | `ossia_string_free`, `ossia_free_string` | .7 | `src/ossia-c/ossia-c.h` | `ValueTests.String` |
| Whether the WebSocket of a mirror is up | `ossia_device_connected` | .9 | `src/ossia-c/ossia-c.h` | `C API: ossia_device_connected follows the WebSocket` |
| Applies the tree changes a mirror received | `ossia_device_run_commands` | .9 | `src/ossia-c/ossia-c.h` | `C API: ossia_device_run_commands applies the nodes added later`, `C API: a parameter added later comes with its value` |
| A server forwards the value of a client to its other clients | `ossia_device_set_echo` | .9 | `src/ossia-c/ossia-c.h` | `C API: ossia_device_set_echo` |
| Sets values and sends them as OSC bundles | `ossia_device_push_bundle` | .9 | `src/ossia-c/ossia-c.h` | `C API: ossia_device_push_bundle from a server`, `C API: ossia_device_push_bundle from a mirror`, `C API: a parameter of another device is not sent` |
| The OSCQuery server of ossia-max, on a network context | `ossia_protocol_oscquery_server_asio_create` | .9 | `src/ossia-c/ossia-c.h` | `C API: an OSCQuery server on a network context` |
| A server knows the client a value comes from, and does not send it back | `oscquery_server_protocol_base::on_osc_message`, `on_binary_ws_message` | .9 | [Differences from ossia 2.1.0](#differences-from-ossia-210) | `C API: ossia_device_set_echo spares the sender`, `C API: ossia_device_set_echo spares the sender over the WebSocket` |
| Crossed writes: the server answers a write that crosses its value | `oscquery_server_protocol_base::set_crossing_window` | .9 | [Differences from ossia 2.1.0](#differences-from-ossia-210); [Unity README, Values in both directions](src/ossia-unity3d/README.md#values-in-both-directions) | `C API: crossed writes converge`, `C API: crossed writes converge while the server forwards`, `C API: the answer to a crossing opens no window`, `C API: a write that crossed nothing is not sent back`, `ForwardTests.CrossedWritesConverge` |
| The cross bench of Unity, Max and TouchDesigner, and the doc test of a sample | `tests/Unity/cross/cross_run.py`, `doc_follow.py` | .9 | [Unity README, With Max and TouchDesigner](src/ossia-unity3d/README.md#with-max-and-touchdesigner) | `tests/Unity/cross/cross_run.py`, `tests/Unity/cross/doc_follow.py` |
| Crossed writes: the client reads the value again; the same value is not a new one | `oscquery_mirror_protocol` | .10 | [Differences from ossia 2.1.0](#differences-from-ossia-210) | `C API: a mirror reads the value again after a crossing`, `C API: a value read again that did not change is not a new value` |
| A server on one address, not announced | `ossia_protocol_oscquery_server_asio_create_bound` | .10 | `src/ossia-c/ossia-c.h` | `C API: a server on 127.0.0.1 only` |
| Who follows: the clients, what they listen to, and the last 256 other senders | `?PRISM_CONNECTIONS`, declared in the `EXTENSIONS` of `HOST_INFO`; `PRISM_NAME` | .10 | [Differences from ossia 2.1.0](#differences-from-ossia-210); [`tools/prism-map/README.md`](tools/prism-map/README.md) | `C API: ?PRISM_CONNECTIONS lists the clients and the senders`, `C API: ?PRISM_CONNECTIONS keeps the last 256 senders`, `?PRISM_CONNECTIONS from ossia.device` |
| A boolean is served as `true` or `false` | `value_to_json.hpp` | .10 | [Differences from ossia 2.1.0](#differences-from-ossia-210) | `C API: a bool is served as true or false`, `bool served as true or false` |
| The asio client declares the port its values leave from | `oscquery_mirror_asio.cpp` | .10 | CHANGELOG, `2.1.0+prism.10` | `C API: ossia_device_set_echo spares an asio mirror` |
| Draws the map of the devices of a network | `tools/prism-map/prism_map.py` | .10 | [`tools/prism-map/README.md`](tools/prism-map/README.md) | `tools/prism-map/test_prism_map.py` |
| The binaries hold no path of the machine that built them | `-ffile-prefix-map` | .10 | [Differences from ossia 2.1.0](#differences-from-ossia-210) | `tests/binary_paths.py` |
| The fixes of Max checked in a running Max, with no click | `tests/Max/check_max.py` | .10 | its own header | `tests/Max/check_max.py` |
| Who follows, under neutral names: the clients, what they listen to, the other senders | `?PEERS` and `CLIENT_NAME`, declared in the `EXTENSIONS` of `HOST_INFO`; `?PRISM_CONNECTIONS` and `PRISM_NAME` kept for the transition, to be removed later | .12 | CHANGELOG, `[2.1.0+prism.12]`; `propositions/oscquery-peers-extension.md` of PRISM | `C API: PEERS and CLIENT_NAME are declared`, `C API: ?PEERS lists the clients and the senders`, `C API: CLIENT_NAME names a client`, `tools/prism-map/test_prism_map.py` |
| A client gets only the addresses it sent `LISTEN` for; `IGNORE` is applied | `oscquery_server_asio.cpp`, `oscquery_server.cpp`, `listens_to` | .12 | CHANGELOG, Behavior changes | `C API: asio server filters by LISTEN`, `C API: C API server filters by LISTEN`, `C API: a bundle follows what each client listens to`, `C API: a mirror listens again after a reconnection` |
| The server applies `ACCESS`: no network write to `GET`, 204 to `?VALUE` of `SET` | `osc_client_message_applier`, `get_query_parser.hpp` | .12 | CHANGELOG, Behavior changes | `C API: asio server applies ACCESS`, `C API: C API server applies ACCESS` |
| A read of a mirrored parameter nothing follows asks the server, without waiting, at most every 100 ms | `read_followed_value`, `refresh_unfollowed` | .12 | `ossia_parameter_get_value` in `src/ossia-c/ossia-c.h`; CHANGELOG | `C API: reading a mirrored parameter nothing follows asks the server`, `C API: an added parameter read then followed gets its value once` |
| The mirror reads the address of a `PATH_CHANGED` again | `oscquery_mirror.cpp` | .12 | CHANGELOG, `[2.1.0+prism.12]` | `C API: a mirror reads again the address of a PATH_CHANGED` |
| The server follows the proposal where it did not: `DATA` of `LISTEN`, 404 and 400, valid JSON, `ECHO` and `EXTENDED_TYPE` declared, UDP senders by address and port, one `ATTRIBUTES_CHANGED` per new parameter, attribute queries, URL fragments | `json_query_parser.hpp`, `websocket_server_beast.cpp`, `json.hpp`, `json_writer_detail.cpp`, `get_query_parser.hpp`, `node.cpp` | .12 | CHANGELOG, `[2.1.0+prism.12]` | the tests named in each commit of `prism/consolidation-12` |
| An HTTP error names the host, the port and the request | `http_client.hpp` | .12 | CHANGELOG, `[2.1.0+prism.12]` | `C API: an HTTP error names the host and the request` |


### Differences from ossia 2.1.0

State on 2026-11-13, `2.1.0+prism.14`; the changes of the Unity package and of the C API are in the [CHANGELOG](CHANGELOG.md). "Fixed" means fixed in ossia-prism, not in ossia.

Since `2.1.0+prism.14` the base is not ossia 2.1.0 as released but its pull request [ossia#876](https://github.com/ossia/libossia/pull/876): the WebSocket server and client are Boost.Beast, `websocket_server.hpp` and `websocket_client.hpp` are gone, and the sockets speak IPv6. Every difference below is a difference from ossia, not from that branch, and each one is on it with its test; the CHANGELOG, `[2.1.0+prism.14]`, says which of them had to be written again for it.

Changed in `2.1.0+prism.12` (details, measures and what stops working: the CHANGELOG, `[2.1.0+prism.12]`; tests: the table above):

- **A client gets only the addresses it listens to** (`oscquery_server_asio.cpp`, `oscquery_server.cpp`). An `ossia.device` sends a client only the addresses it sent `LISTEN` for, as the OSCQuery proposal says; an `ossia.client` listens to what its `ossia.remote` follow. A client that never sends `LISTEN` gets no value.
- **An `ossia.device` applies `ACCESS`**: a value written over the network to a parameter published as `GET` is ignored.
- **`?PEERS` and `CLIENT_NAME`**: a device lists its clients, what each listens to and the other senders, under neutral names; `?PRISM_CONNECTIONS` and `PRISM_NAME` stay during the transition.
- **Reading a parameter nothing follows no longer waits for the server** (`namespace`, the explorer): the read returns the last value known and asks the server again, at most every 100 ms. An `ossia.remote` is the way to follow a value.
- **One connection of the package to a device announced again at its address** (`client_mirror.cpp`): a server that restarted got two.
- **No red line in the console for a client that leaves as soon as it connects, nor for an `ossia.client` stopped while its server closes the connection**; that `disconnect` held Max up to 5 s, it now returns at once. An HTTP error names the host, the port and the request.

Changed in `2.1.0+prism.11`:

- **A Minuit device that leaves the network no longer freezes Max** (`ZeroconfMinuitListener.cpp`). The listener of `_minuit._tcp`, which the package starts with its first object and runs every 100 ms on a clock of Max (on the scheduler thread with Overdrive), handled a withdrawn announcement under its lock and called a function that takes the same lock: the scheduler waited on itself for good, and the main thread too as soon as it entered Max's critical region. Nothing has to connect to the device. Same code since 2020, in ossia 2.1.0 and in every ossia-prism up to `2.1.0+prism.10`. The listener now handles the announcements out of its lock. `tests/Max/check_minuit.py`: with `2.1.0+prism.10`, Max froze 1 s after the withdrawal, on the frames of the hang reports macOS wrote for freezes seen in use; after, the metro goes on, 6 runs of 6. Candidate for upstream.
- **A Minuit device that comes no longer makes Max announce services without end** (`client_mirror.cpp`, `minuit.cpp`). The listener of `_minuit._tcp` builds a client for each device announced on the network, and every Minuit protocol of libossia announced itself (`<name> Minuit server`): the listener saw that announcement as a new device, built a client for it, which announced itself in turn, and so on, the names growing to the length limit of mDNS, then numbered by the system. Measured on the network: 2576 services after three runs of `tests/Max/check_minuit.py` in one Max, Max at about 700 % CPU, the names later numbered up to 32306, until Max quit; a load bench run on that Max saw its script stop about 1 s, 11 times in 60 s. Each announcement also held Max's scheduler: building the client took 634 to 745 ms (6 runs). The clients of the listener no longer announce themselves (`minuit_protocol` takes `announce`, true by default): built in 0 ms (6 runs), and `tests/Max/check_minuit.py` sees one service for one announcement, the metro stopping at most 0.11 s when the device comes and when it leaves (3 runs of 3). Same code in ossia 2.1.0. Candidate for upstream.
- **The connections the package opens to announced devices name the package, not the device** (`client_mirror.cpp`, `oscquery_mirror.cpp`). The listener of `_oscjson._tcp` opens one connection per announced device, which `connect <name>` reuses, and gave the server the name of that device: `?PRISM_CONNECTIONS` of `unity_demo` listed a client `unity_demo`, and `tools/prism-map` drew an edge from each device to itself. It now gives `max_client_<process>`. `oscquery_directory_names_its_client`: 3 runs of 3 without that name before, 6 of 6 with it after. A connection by URL still gives the name of its `ossia.client`.
- **An `ossia.remote` gets the value of a node the server adds once, not twice** (`oscquery_mirror.cpp`). For one new parameter the server sends two `ATTRIBUTES_CHANGED` with a `TYPE` (the type, then the parameter), and the client read the value again for each: what follows the parameter got it twice, then once for each change, as `2.1.0+prism.10` wrote in its limits. The client now reads it once at a time for an address. `oscquery_mirror_added_node_value_once`, a libossia server and the mirror the package uses, a callback set when the parameter appears: 2 values in 3 runs of 3 before, 1 in 6 of 6 after, for a value of 0.5 and for a value equal to the default; one change, one value. In Max, `tests/Max/check_remote.py` (an `ossia.client` by URL and one by name, each with an `ossia.remote`, the server a libossia process): the value once, then one change once, by URL and by name, in 47 runs of 48; see the known defects for the other. The mirror is the one of libossia, which the Unity client uses too. Candidate for upstream (a TODO of ossia notes the double read).
- **A value sent to the inlet while the network's value goes out is not lost** (`object_base.cpp`, `matcher.cpp`). With Overdrive on, `ossia.parameter` outputs a value of the network on the scheduler thread. Two guards meant to stop an object's own loop, a flag of the object and the list of the parameters being pushed, were shared by every thread: a value written to the inlet on the main thread meanwhile was taken for that loop and dropped, without a message. The guards now hold for their own thread. `tests/Max/check_outlet.py`, a consumer of about 100 ms on the outlet: 20 values lost in 20 with `2.1.0+prism.10`, none after. Same guards in ossia 2.1.0.

Changed in `2.1.0+prism.10`:

- **`ossia.client`** (`client.cpp`): `[ossia.client <name>]` no longer connects by itself every second; a connection by name takes the tree of the listener's mirror instead of reading it again; `@reconnect 0` also stops the attempts of a first connection that fails.
- **`[ossia]` bundles** what its objects push during a tick, as `ossia.device` does (`@bundle`, `ossia_object.cpp`); in ossia 2.1.0 the attribute did nothing.
- **`dump`** on `ossia.parameter` and `ossia.remote` sends every attribute by the right outlet, its name then its value; in ossia 2.1.0 it was written as a value.
- **A list parameter bounded by `@min` and `@max`** (a `vec3f`, for instance) carries the range on every component; in ossia 2.1.0, on the first only.
- **`@hold <ms>`** on `ossia.parameter` and `ossia.remote`: taking control. After a value comes in by the inlet, the values of the network are held for that long, then the last one held comes out. The example `examples/ossia.panel.maxpat` shows it.
- **`expose oscquery <OSC port> <WebSocket port> <address>`** on `ossia.device`: both ports listen on that address only, `127.0.0.1` for the clients of this machine, the address of one interface for that network; such a device is not announced over mDNS, clients connect with `ws://<address>:<port>`. The outlet adds the address to `expose 1 oscquery <ports>`.
- **The OSCQuery server** serves a boolean as `true` or `false` (it was `null`), and answers `?PRISM_CONNECTIONS`, declared in the `EXTENSIONS` of `HOST_INFO`: the clients connected (address, port, the name an ossia-prism client sends, its OSC port, the addresses it listens to, its packet count) and the last 256 other senders of OSC. `tools/prism-map` draws the map of a network from it.
- **The OSCQuery client reads a value again after a crossing** (`oscquery_mirror.cpp`): when a value received crosses one it sent on the same address less than 500 ms before, it asks the server for the value 100 ms later. Against a server that does not answer crossed writes, 20 divergences in 20 crossings before, 0 in 100 after.
- **`extras/ossia.panel.html`**: a page that builds a control for each parameter of an OSCQuery device, from its tree, and follows it over the WebSocket; `examples/ossia.panel.maxpat` opens it in a `jweb`.
- **`@trim_address`** is removed from `ossia.v8connector`, which never read it.
- **The binaries** no longer hold the paths of the machine that built them (`tests/binary_paths.py`).

Changed in ossia-prism:

- **One `EXTENDED_TYPE` per parameter** (OSCQuery server, `src/ossia/network/oscquery/detail/json_writer_detail.cpp`, every object that publishes a parameter). libossia 2.1.0 describes a color by the `EXTENDED_TYPE` array of its components (`color.rgb.r`, `color.rgb.g`, ...), then writes the node's own extended type (`vecf`) as a second `EXTENDED_TYPE` key in the same object; a client that keeps the last duplicate key sees `vecf` and loses the color. In ossia-prism the node's extended type is not written when the parameter has a unit. Colors are still described by that array, not by `UNIT`; a parameter without a unit keeps its own extended type.
- **`ossia.client` connected by name applies the tree changes** (`src/ossia-max/src/client.cpp`, `client_mirror.hpp`). In ossia 2.1.0, `connect <name>` (zeroconf) gets the tree once; the changes the server sends afterwards (`PATH_ADDED`, `PATH_REMOVED`, `ATTRIBUTES_CHANGED`) are only applied for a client connected by URL (`connect oscquery ws://host:port`), so a parameter added on the server never appears. ossia-prism polls the mirror in both cases, and removes a path the server removes, as for a connection by URL.
- **`ossia.client` connected by URL reconnects** (`client.cpp`, `client_mirror.hpp`). In ossia 2.1.0, once the server's WebSocket goes down (the server stopped), the client receives nothing more, reports nothing and never reconnects, while its own sends still leave over UDP. ossia-prism reports the loss with `connect 0 <parameters>` on the outlet, tries again every second on the main thread until the server answers on its port, connects as the first time (tree read again, every followed parameter subscribes again with `LISTEN`), then sends `connect 1 <parameters>`. `@reconnect 0` turns off the attempts after a loss; a first connection that fails is tried again every second whatever `@reconnect` says (see [Known limits](#known-limits)).
- **The OSCQuery server no longer sends a client's value back to it** (`2.1.0+prism.9`, `src/ossia/protocols/oscquery/oscquery_server_asio.cpp`). `expose oscquery` turns the device's echo on (`src/ossia-max/src/device.cpp`): the server forwards a value one client sends to the other clients. In ossia 2.1.0 it does not tell which client a value comes from (two TODOs) and sends it back to its sender too. ossia-prism knows the client by the UDP port its values come from, as the server of the C API does, and by its connection over the WebSocket. Measured outside Max, with the same server and the echo on, through the C API (`tests/C/unit-tests/unit-tests-c-network.cpp`): the other client receives the value, its sender does not, over UDP and over the WebSocket. Measured in Max 9 with the cross bench (`tests/Unity/cross/`), an `ossia.device` followed by a Unity client that writes 20 values: 20 came back to the client with the library of `2.1.0+prism.8`, none with that of `2.1.0+prism.9`.
- **The OSCQuery server answers a crossed write** (`2.1.0+prism.9`, `oscquery_server_asio.cpp`). When `ossia.device` sets a value and a client writes another one to the same address in the same instant, each value crosses the other; in ossia 2.1.0 each side keeps the other's. ossia-prism answers a client's write that comes less than 100 ms after the server sent it a different value on that address with the server's final value, to that client only. Measured outside Max, with the same server, through the C API: 20 divergences in 20 crossings before, 0 after. Within the window the server cannot tell a crossing from a write made right after receiving its value: such a write gets its own value back, once.
- **Bundles** (`2.1.0+prism.5`). In ossia 2.1.0 the OSCQuery client's `push_bundle` and `push_raw_bundle` are TODOs that send nothing (`src/ossia/network/oscquery/oscquery_mirror.cpp`), and a device exposed through a multiplex (as `ossia.device` does) pushes a bundle one value at a time (`src/ossia/network/local/local.cpp`), so a script's values left one message each. Three commits of the core, `oscquery: the client sends bundles`, `oscquery: a multiplexed device forwards bundles to its protocols` and `oscquery: bundles of values, in which a parameter may come several times`, make them send OSC bundles; `ossia.v8connector` groups the values of a tick (its `@bundle`), and `ossia.device` and `ossia.client` group what their objects push during a tick (their `@bundle`, on by default for OSCQuery).
- **`ossia.client`: the poll and `connect` never run at once** (`2.1.0+prism.5`, `client.cpp`, `client_mirror.hpp`). In ossia 2.1.0 the client applies its mirror's commands from a clock, on the scheduler thread with Overdrive, while `connect` and `disconnect` destroy the device on the main thread: a click on `connect` could crash Max. The poll, `connect`, `disconnect` and the end of a connection now enter Max's critical region, and the link waits for a running poll before the device goes.
- **`[ossia]` frees its ports when deleted** (`2.1.0+prism.5`, `ossia_object.cpp`, `device_network.hpp`). In ossia 2.1.0 the servers exposed by `[ossia]` stay on the global device after the object is deleted, their ports taken. They are now stopped with the object.
- **`ossia.client` by URL does not hold Max** (`2.1.0+prism.5`, `client.cpp`, `client_mirror.hpp`). In ossia 2.1.0, `connect oscquery <url>` builds the mirror on the main thread, which waits up to 1.5 s for the WebSocket and up to 3 s for the tree: Max freezes during each attempt, and an `ossia.device` of the same Max, whose server is polled by that same thread, can never answer. The connection now runs on a thread of its own; the outlet sends `connect 1 ...` when it is done.
- **`stop <index>` on an `ossia.device` exposed over OSCQuery no longer quits Max** (`2.1.0+prism.6`, core commit `3b065120b`, `src/ossia/network/sockets/udp_socket.hpp`). In ossia 2.1.0, `stop <index>` destroys the OSCQuery server at once, while the closing of its UDP socket, posted to the network context, still captures the socket: the next poll of the context, on Max's scheduler thread, reads the freed socket and `close()` throws `boost::system::system_error` (Bad file descriptor), which nothing catches. A posted closing now does nothing once its socket is destroyed, and neither `shutdown()` nor `close()` throws out of the context. The same holds for the WebSocket server (core commit `1788af7bf`, `src/ossia/network/sockets/websocket_server.hpp`): websocketpp keeps its handlers in each connection, which outlives the server, and a client whose closing handshake completes after the stop (in Max, the zeroconf listener's mirror of the device itself) called `on_connectionClosed` on the destroyed server, whose mutex then threw. The handlers now do nothing once the server is stopped or destroyed. Candidates for upstream.
- **The OSCQuery server withdraws its announcement when it stops** (`2.1.0+prism.6`, core commit `5e8816497`, `src/ossia/protocols/oscquery/oscquery_server_asio.cpp`). In ossia 2.1.0 the mDNS announcement goes only with the destruction of the server, after its connections are closed. Candidate for upstream.
- **`ossia.client` connected by name follows the device** (`2.1.0+prism.6`, `ZeroconfOscqueryListener.cpp`, `client_mirror.cpp`). In ossia 2.1.0 the zeroconf listener builds the mirror of each announced device on a Max clock, under a lock, once: a build that fails is never tried again, a device of the same Max is never found (the clock that builds is the one that polls its server), and a mirror whose WebSocket went down stays until the announcement goes. ossia-prism builds each mirror on a thread of its own, tries again every second, drops a mirror whose WebSocket is down and builds it again; the client connected by name is told (`disconnected`, `connect 0 <name>`) and reconnects, as `@reconnect` allows. The mirrors are still built for every announced device, so that `ossia.fuzzysearch` keeps searching them.
- **`ossia.device` says the name it is announced under** (`2.1.0+prism.6`, `ZeroconfOscqueryListener.cpp`, `client_mirror.cpp`). In ossia 2.1.0, when the name of a device exposed over OSCQuery is already announced on the network, the system announces it as `name (2)` and Max says nothing: Servus, the mDNS library of libossia, does not report the name it registered. ossia-prism finds the device's own announcement among those it browses (its WebSocket port, this machine), sends `announced <name>` on the outlet of the `ossia.device`, and warns in the console when the name differs. Servus is unchanged.
- **`@clip none`**, the OSCQuery name of the free mode, is accepted by `ossia.v8connector` only; `ossia.parameter` still expects `free`.
- **An address already taken** is refused by `ossia.v8connector` without a name (`refused <address> already exists`); `ossia.parameter`, in the same case, creates a sibling with a free name (`gain.1`).
- Added: the `ossia.v8connector` object, the package name `ossia-prism`, the version `2.1.0+prism.N`.

Fixed in `ossia.v8connector`:

- **A binding left on a device the client no longer uses** (`2.1.0+prism.6`, `v8connector_branch.cpp`). An `ossia.client` that leaves a device still alive elsewhere, the zeroconf listener's mirror it shared when connected by name, for another one (a connection by URL) does not tell the object: it clears its device before announcing the removal, and the shared mirror is not destroyed. The object then followed each bound address on both devices (`bind <address> 2`) and sent every value through both. A change of resolution context now releases the bindings on the devices that left it.
- **Crash when a followed device comes back** (`2.1.0+prism.4`). Max crashed twice in `ossia.v8connector`, in a patch whose `bind` addresses point to a remote device, while the object registered again. `ossia.client` clears its device before it announces its removal, so the object kept a pointer to a destroyed device and used it at the next registration; under AddressSanitizer the bench shows that use after free when a followed device is destroyed and a new one arrives under the same name. The object now notices the destruction of every device it knows by itself. The crash itself has not been reproduced in Max.

Known defects, not fixed:

- Sending `expose` on a port already in use, including a second `expose`, crashed Max up to `2.1.0+prism.8`. Measured with `2.1.0+prism.10`: the outlet sends `expose 0 oscquery <ports>` and the console says the port might be in use. Use explicit, free ports, and `stop <index>` before exposing again (`stop 0` for the first `expose`; `stop` alone is refused with "missing arguments for message "stop"").
- The server logs `remote_endpoint: Invalid argument` at each connection, with no effect seen on the served tree.
- The access mode is not enforced by the server: a read-only parameter can be written over OSC.
- The client cannot read some OSCQuery servers that are not ossia: it asks `?HOST_INFO=1` instead of `?HOST_INFO`, and does not read `chunked` HTTP replies.
- On Windows, the automatic discovery of OSCQuery devices (mDNS) depends on the Bonjour service being installed and running; without it, connect `ossia.client` to an explicit address.
- The OSCQuery server of libossia (the one of `ossia.device`), destroyed while its network thread runs, frees its sockets before they are closed (destructor, `src/ossia/protocols/oscquery/oscquery_server_asio.cpp`): the closing of its UDP socket, posted to that thread, runs after the release. AddressSanitizer sees it in a test that stops a server and exposes another in the same process; not seen as such in Max.
- The listener of `_minuit._tcp` resolves each device newly announced on the network on Max's scheduler: Servus waits up to 500 ms for the answer (from the code). Measured on one machine, the metro of `tests/Max/check_minuit.py` stops at most 0.11 s (3 runs).
- The Pd package (`ossia-pd`, not built by this repository's releases) has the same listener of `_minuit._tcp`, whose clients announce themselves: one Minuit device on the network would make Pd announce services without end, as Max did before `2.1.0+prism.11` (from the code, not run).
- The value of a node the server adds can still come twice, rarely (`2.1.0+prism.11`). An `ossia.remote` connected by URL output it twice in one run of `tests/Max/check_remote.py`: seen once in 48 runs, before the server's log was kept, not reproduced; the 46 runs with the log show one read of the value for each client. Without Max, `oscquery_mirror_added_node_value_once` got 2 values for a value of 0: seen once in a full run of `ossia_V8ConnectorTest` on a loaded machine (load average about 11), 0 of 60 when the test ran alone. Probable cause, read in the code and not proven: the guard of the read is cleared by the answer to the first read, so a second `ATTRIBUTES_CHANGED` processed after that answer reads the value again. No test reproduces it yet.
- `connect <name>` on an `ossia.client`, to a Minuit device that does not answer, holds Max's scheduler while the protocol waits for the tree (`minuit_protocol::update`: 5 s, then up to 100 times 250 ms, from the code). Seen once: the metro stopped for the 6 s the test watched.

### Discovery by name

`expose oscquery` on an `ossia.device` announces it over mDNS as `_oscjson._tcp`, under the device's name, with the WebSocket port (also the HTTP port) in the SRV record and the host name of the machine as its target; no `_osc._udp`. `connect <name>` on an `ossia.client` connects to it, from the same Max, another Max or a TouchDesigner component of the same network. The client keeps the mirror the zeroconf listener built for that device, follows its WebSocket and tries again every second (`@reconnect`). The listener builds a mirror for every announced device, on a thread of its own, and tries a failed build again every second while the device is announced. To a server that lists its clients (`?PRISM_CONNECTIONS`), that mirror gives the name `max_client_<process>`, since `2.1.0+prism.11`.

Send `connect <name>` to the client. Since `2.1.0+prism.10`, `[ossia.client <name>]`, created with a name only, takes that name and connects nothing; in ossia 2.1.0 it sent itself a `connect` with no argument every second, the console printing the help of the protocols each time. `connect` with no argument prints that help once and tries nothing. `[ossia.client <name> 0]`, an integer, still connects by name to `<name>` itself.

When it connects by name, the client takes the tree the listener's mirror holds (`client.cpp`, `finish_connect`). Up to `2.1.0+prism.9` it read the tree again: the mirror emptied its root and built it again, the outlet sent `removed` then `created` for every node, the `ossia.remote` objects registered again and the values that arrived in between came out as `osc <address> <values>`. A connection by URL reads the tree once, on its own thread.

mDNS works within one subnet: the machines must share a VLAN, multicast 224.0.0.251 on UDP 5353 must pass between them, Wi-Fi client isolation must be off. On Windows, mDNS in Max needs the Bonjour service. Where discovery cannot work, connect by address: `connect oscquery ws://<ip>:<port>`.

### Known limits

- `@priority` is stored on the node but does not order the outputs of `ossia.v8connector`: ossia-max orders the outputs of `ossia.parameter` and `ossia.remote` only.
- An `ossia.model` or `ossia.view` added to the patcher after the object has registered does not take it under its node: send `address <name>` to the object, or reload the patcher.
- `ready` and `refused` are reserved addresses; inside one instance an address is either declared by `add` or followed by `bind`.
- Two instances with the same name become `name` and `name.1`, like two `ossia.model`.
- The node attributes of the object (`@priority`, `@description`, `@tags`, `@invisible`, `@recall_safe`), `namespace` and `get` act on the node of a named instance: an instance without a name, or that only binds, has none, and they do nothing (`namespace` outputs `namespace_size 0`).
- One `ossia.device` or `ossia.client` per patcher (as in ossia 2.1.0): a second one in the same patcher is not created, and the console says "You can put only one [ossia.device] or [ossia.client] per patcher". Put each in its own subpatcher or abstraction.
- `[pattr @bindto]` cannot bind to an `ossia.parameter`, which has no `getvalueof` and `setvalueof`: what the `pattr` recalls or receives is lost, without a message. Connect the outlet of a `[pattr]` to the inlet of the `ossia.parameter` instead: a recall of a `pattrstorage` then leaves on the network (measured, see below). The PRISM tab of the help of `ossia.parameter` shows it.
- `examples/ossia.v8connector.load.maxpat` exposes its device on ports 7301 and 7302 when it loads: opening it twice is a second `expose` on busy ports (see [Differences from ossia 2.1.0](#differences-from-ossia-210), known defects).
- A script can end on another value than the device. With Overdrive on, `ossia.parameter` outputs a value of the network on Max's scheduler, while a script runs on the main thread: Max hands the script that value after what it is running. A script that writes to the parameter at that moment then receives the network's value after its own, though the device holds its own, and the outlet output them in that order. Measured, `tests/Max/check_outlet.py`: 40 times out of 40 when the two values come together, over UDP and over the WebSocket. An object that reads the outlet on the scheduler is not in this case. To end on what the device holds, send `bang` to the object when a value comes, on the same thread: it outputs the value the device holds (the same test: 0 times out of 20, over UDP).
- A change leaves whatever its source, measured in Max 9 with `2.1.0+prism.10` and a WebSocket client: one value received for each change made by a message box, `set <value>`, a number box or a `live.dial` patched into the inlet, and a `pattrstorage` recall through a `[pattr]` patched into the inlet or bound to a `live.dial`; none through `[pattr @bindto]` on the parameter (see above). A gesture with the mouse is not measured: it takes the path of the number box's output. The benches change the values from a script. That a value an `ossia.device` or `ossia.client` receives does not leave again, and that crossed writes converge, is measured with Max 9 by the cross bench of the Unity package (`tests/Unity/cross/`, against Unity and TouchDesigner: nothing back for 20 values, 0 divergence in 20 crossed writes) and by the bench of prism-td-components (`bidir_run.py`: 0 copy sent back by `ossia.device` for 50 values written by TouchDesigner, 50 with `2.1.0+prism.8`).
- Not tested: Intel Macs, Windows (not built), Max 8 and earlier.
- The defects of ossia that ossia-prism does not fix are listed under [Differences from ossia 2.1.0](#differences-from-ossia-210).

### Functional audit

What the Max package does for each requirement of the PRISM platform, and what proves it, for `2.1.0+prism.10`. The requirements: values travel in OSC bundles, and only those that change (12); both directions, without echo (13): a value received is applied (a), every local change leaves whatever its source (b), a value received and applied does not leave again, and two sides writing at once agree (c).

| Requirement | Server (`ossia.device`) | Client (`ossia.client`, `ossia.remote`) | Proven by |
|---|---|---|---|
| 12, bundles | what its objects push during a tick leaves at its end, in bundles of at most 1400 bytes | the same through the mirror: under 1400 bytes over UDP, one bundle over the WebSocket | load bench of [Bundles](#bundles); `tests/Max/check_max.py` (`[ossia] @bundle 1`: one frame for a tick) |
| 12, only what changes | a repeated value leaves by default; `@repetitions 0` filters it | the same | not checked by a bench |
| 13a, received values applied | at once, on the outlet of `ossia.parameter`, bounded as its `@clip` says | the same on `ossia.remote` | `tests/Unity/cross/cross_run.py`; the bench of the TouchDesigner component (`bidir_run.py`) |
| 13b, every source leaves | a message box, `set`, a number box, a `live.dial`, a `pattrstorage` recall through a `[pattr]` patched into the inlet: one value per change | the same path | measured with a WebSocket client (see [Known limits](#known-limits)); `bidir_run.py`, echo |
| 13c, no echo | a client's value goes to the others, never back to it | nothing it receives leaves again | `cross_run.py` (0 of 20 back on each side); `bidir_run.py` (0 of 50); `C API` tests of `tests/C/unit-tests/unit-tests-c-network.cpp` |
| 13c, two writers agree | a write that crosses a value sent less than 100 ms before is answered with the final value | reads the value again 100 ms after a crossing; a value that did not change is not a new one | `cross_run.py` (0 divergence out of 40 crossings with Max); `bidir_run.py` (0 out of 100 with Max); `C API: crossed writes converge`, `C API: a mirror reads the value again after a crossing` |
| Discovery by name | announces `_oscjson._tcp`; `announced <name>` says the name it got | `connect <name>` | `cross_run.py` (by name); `check_max.py` (the tree is not read again) |
| Reconnection | a client that comes back is served; `expose` after a stop serves again | tries again every second; `@reconnect 0` stops it, first connection included | `bidir_run.py`, reconnection; `check_max.py` (`@reconnect 0`) |
| Taking control | `@hold <ms>` on `ossia.parameter` | `@hold <ms>` on `ossia.remote` | `check_max.py` (`@hold 500`); `examples/ossia.panel.maxpat`, the PRISM tab of the help of `ossia.parameter` |
| Where it listens | `expose oscquery <OSC port> <WebSocket port> <address>` listens there, and is not announced | no choice of interface | `check_max.py` (reached on `127.0.0.1`, refused through another interface, not announced) |
| Who follows (map) | `?PRISM_CONNECTIONS`, the last 256 UDP senders | sends `PRISM_NAME` and what it listens to | `C API: ?PRISM_CONNECTIONS lists the clients and the senders`, `C API: ?PRISM_CONNECTIONS keeps the last 256 senders`; `check_max.py`; `tools/prism-map/test_prism_map.py` |

What it does not do, also in the limits: a repeated value is not filtered by default; `[pattr @bindto]` cannot bind to an `ossia.parameter`; a gesture of the mouse on an interface object is measured by what the object sends, not with a mouse; `@hold` ends by time, not with the mouse button; a source that animates in Max is not paused by a value from the network (with `@hold`, the values it sends win over those of the network); a client does not choose the interface it follows from.

### Cross test with Unity and TouchDesigner

- **Unity**: the Unity package's sample "Custom parameters" ships `CustomParameters.maxpat`: `[ossia.client]`, `connect unity`, one `ossia.remote` per field of a Unity script (`float`, `int`, `bool`, `Color`, `Vector3`), with what Unity sends back shown next to each. Steps in the Unity package's [README](src/ossia-unity3d/README.md#driving-from-max).
- **TouchDesigner**: the OSCQuery component of PRISM (prism-td-oscquery) decodes the bundles of ossia-prism, as server and as client, and an `ossia.client` connects to it by name or by address (see [Discovery by name](#discovery-by-name)); its procedure is in that component's documentation.

### Troubleshooting

| Symptom | Cause | Action |
|---|---|---|
| `ossia.v8connector` cannot be created, or no `ossia-prism 2.1.0+prism.N, PRISM CNRS` line in the console when it is | The official `ossia` package is still installed next to ossia-prism, or ossia-prism is not in the Packages folder. | Remove `~/Documents/Max 9/Packages/ossia`; check that `~/Documents/Max 9/Packages/ossia-prism` exists; restart Max. |
| Max cannot load the externals of the package | macOS quarantine on an unsigned package. | `xattr -dr com.apple.quarantine ~/Documents/Max\ 9/Packages/ossia-prism`, restart Max. |
| The helper posts `no object named 'synth' in this patcher or above` | The object has no Scripting Name, or another one. | Inspector of `ossia.v8connector` > Scripting Name, the name given to `new OssiaConnector(...)`. |
| The script receives nothing (no `ready`, no values) | The left outlet of `ossia.v8connector` is not patched into the script's inlet, or the script has no `anything()` calling `dispatch`. | Patch it; copy `anything()` from the example. |
| `refused <address> ... requires both @min and @max (or @range)` | A numeric type declared without bounds. | Add `min` and `max`, or `range`. |
| `refused <address> ... already declared with other attributes` | The same address declared again with other attributes. | `remove` it first, or declare it once. |
| `refused <address> already exists` | Without a name, the address is taken by another instance or an `ossia.parameter`. | Give the instance a name, or another address. |
| `expose 0 oscquery ...` on the outlet and "can't connect, port might be already in use" in the console | The port is already in use, by a second `expose` or another program. | `stop <index>` before exposing again (`stop 0` for the first `expose`); `lsof -nP -iTCP:<port> -sTCP:LISTEN` names the program holding it; choose free ports. |
| `ossia.client` stays on `connect 0 <name>` | The device is not announced on this network: mDNS blocked (another subnet, VLAN, Wi-Fi isolation, multicast 224.0.0.251 UDP 5353 filtered), or on Windows, no Bonjour service. | `dns-sd -B _oscjson._tcp` lists what is announced; connect by address, `connect oscquery ws://<ip>:<port>`. |
| The device is announced as `name (2)` | Another device of the network already has the name; `ossia.device` sends `announced name (2)` and warns in the console. | Rename the device, or connect with `connect name (2)`. |
| The console shows `remote_endpoint: Invalid argument` at each connection | Logged by the server, without effect on the served tree (known defect). | Nothing to do. |

### Building

```
cmake -S . -B build-ossia-max -GNinja -DCMAKE_BUILD_TYPE=Release -DOSSIA_MAX_ONLY=1 -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64"
cmake --build build-ossia-max --target install
```

The package lands in `ossia-max-package/ossia-prism` under the build directory, or where `OSSIA_MAX_INSTALL_FOLDER` points. The headless test of the object's core is `tests/Network/V8ConnectorTest.cpp`, built with `OSSIA_TESTING`; the tests of the helper and of the grid example's script are `tests/Network/V8ConnectorHelperTest.js` and `V8ConnectorGridExampleTest.js`, run with Node.

### Upstream

ossia-prism follows [libossia](https://github.com/ossia/libossia) 2.1.0, by the ossia project (https://ossia.io), and since `2.1.0+prism.14` its WebSocket server and client are those of the pull request [ossia#876](https://github.com/ossia/libossia/pull/876) (Boost.Beast in place of WebSocket++, IPv6, socket.io), not yet merged upstream: the sources are that branch plus what this repository adds. The upstream documentation, https://ossia.io/ossia-docs/, describes the objects of the official package; it does not cover `ossia.v8connector` nor the changes listed above (bundles, `@reconnect`, connection by name that follows the device, `announced`). The upstream downloads and nightly builds install the official `ossia` package, which cannot be installed next to ossia-prism. This repository builds only the Max and Unity packages; the other bindings of libossia (Pure Data, Python, C, QML, ...) are not built nor tested here.

### License

libossia: LGPLv3 or CeCILL-C, at your choice (`LICENSE`, `LICENSE.cecill`). Files of `ossia.v8connector` (object, helper, examples, help): copyright CNRS, as their SPDX headers say.
