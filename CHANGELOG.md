# Changelog

All notable changes to **ossia-prism**, the Max package built from this repository (ossia 2.1.0 for Max plus `ossia.v8connector`), and to **ossia (PRISM)**, the Unity package built from it (`io.ossia.ossia-unity`), are documented in this file. The rest of libossia has no changelog here.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/). Versions follow [Semantic Versioning](https://semver.org/) as `UPSTREAM+prism.N`: the ossia version the package is built from, then the number of the ossia-prism release on top of it.

## [Unreleased]

Unity package **ossia (PRISM)** (`io.ossia.ossia-unity`), for macOS universal, installed with the Package Manager's "Add package from tarball". Tested with Unity 6.3 LTS on Apple Silicon, Mono scripting backend. Not tested: macOS Intel, Windows, Linux, iOS, IL2CPP players.

### Added

- Unity: the `Controller` has the fields `deviceName`, `oscPort` and `wsPort`, set in the inspector; their defaults are the values fixed before in the code: `unity`, 1234, 5678.
- Unity: the `Controller` field `runInBackground` (on by default) sets `Application.runInBackground` at runtime, without changing the project settings, so that the scene keeps following the values received when Unity loses focus.
- Unity: the sample "Exposed cube" (Package Manager, Samples): an `OssiaController` and a cube whose transform is exposed.
- Unity: Edit Mode and Play Mode tests in the package (`Tests/`): a round trip of each value type, the native callbacks, a second Play, the mDNS announcement and its withdrawal, a cube moved by an OSC message from outside.
- Unity: `LISEZMOI.txt` in the package, and the README's sections on installation, the `Controller`, focus, Max and mDNS.

### Fixed

- Unity: the package the build assembles installs and compiles: it carries `ossia.asmdef`, without which Unity compiled none of its scripts, and a `.meta` file for each file and folder, without which Unity ignored them all in a package installed from a tarball (150 assets ignored). Its version is the tag without its leading `v`; its macOS library is enabled for any CPU, no longer for x86_64 only, which the Apple Silicon editor did not load.
- Unity: a player builds: `PresetController.cs` declared its inspector drawer, editor code, outside `#if UNITY_EDITOR`, and any player build, Mono or IL2CPP, failed to compile (`PropertyDrawer` not found).
- Unity: the scene follows the values received when Unity loses focus: with the project's Run In Background off, Unity stopped `Update`, where the `Controller` applies them, until its window was focused again.
- Unity: `Vector4` and `Quaternion` are sent x, y, z, w, the order they are read in and the order of ossia's `orientation.quaternion`; they were sent w, x, y, z. A peer that reordered the components it received from Unity must stop doing so.
- Unity: a `Color32` no longer throws `InvalidCastException`: it is sent and read with components from 0 to 255 (`color.rgba8`).
- Unity: `Value.GetList()` reads the list it is given; it read it as a list of floats, past its end, and a list of ints aborted Unity.
- Unity: every native function the binding imports exists in the library with that signature: `ossia_value_to_char`, `ossia_devices_read_json`, `ossia_preset_set_debug_logger`, `ossia_preset_free_string` and `ossia_devices_get_node_parameter` did not exist (`EntryPointNotFoundException` when called), eight others had a wrong signature.
- Unity: the native callbacks (node and parameter deletion, debug log) are static methods marked `[MonoPInvokeCallback]`, which an IL2CPP player requires; they were lambdas.
- C API: the strings it returns are allocated with `malloc`, as `ossia_string_free` and `ossia_free_string` free them with `free`; some were allocated with `new[]`.

## [2.1.0+prism.6] - 2026-11-01

Published for macOS universal (Apple Silicon and Intel). Windows x64 to come: same version, added to the Release later.

### Added

- `ossia.device` exposed over OSCQuery: `announced <name>` on the outlet, about a second after `expose`, with the name the device is announced under over mDNS; when that name is already taken on the network, the system announces the device as `name (2)`, and the console warns. Before, nothing said so.

### Changed

- `ossia.client` connected by name (`connect <name>`): when the device's WebSocket goes down while it is still announced, the outlet sends `disconnected` then `connect 0 <name>` within about a second, and the client tries again every second until the device answers; before, it stayed connected to a dead device until the announcement went (60 s after a TouchDesigner killed, never for a server stopped but still announced). `@reconnect 0` stops the attempts, as for a connection by URL, and also after the device is no longer announced.
- An OSCQuery server withdraws its mDNS announcement when it stops (`stop` on an `ossia.device`), before it closes its connections; before, only when it was destroyed.

### Fixed

- `stop` sent to an `ossia.device` exposed over OSCQuery quit Max (`stop 0` after `expose oscquery`, three times out of three): the server was destroyed while the closing of its UDP socket, posted to its network context, still pointed to it, and while the WebSocket of a connected client (the zeroconf listener's own mirror of the device, in the same Max) was closing: the next poll threw out of the scheduler, `close: Bad file descriptor` for the first, `mutex lock failed` for the second. The defect existed in `2.1.0+prism.5`, `2.1.0+prism.4` and ossia 2.1.0.
- `ossia.client` finds by name a device of the same Max (`connect <name>` to an `ossia.device` exposed in that Max): the mirrors of the announced devices are built on a thread of their own. Before, they were built on the Max clock that also polls the device's server, which could never answer: the device was never found (`devices 0`), and each attempt held that clock for 1.5 s.
- A device announced before its server answers, or whose build failed, is tried again every second while it is announced; before, it was never tried again until it was announced anew.
- `ossia.v8connector`: when its `ossia.client` goes from one device to another while the first stays alive (connected by name to the zeroconf listener's shared mirror, then by URL, as the fallback of a demo does), the bindings on the first device are released. Before, each bound address followed both (`bind <address> 2`), and every value set left twice, once through the listener's mirror.

## [2.1.0+prism.5] - 2026-10-30

Published for macOS universal (Apple Silicon and Intel). Windows x64 to come: same version, added to the Release later.

### Added

- `ossia.device` and `ossia.client`: attribute `@bundle` (default -1: on when the device speaks OSCQuery, off for plain OSC and Minuit). What `ossia.parameter`, `ossia.remote` and `ossia.view` push during a tick of the scheduler leaves at its end, in OSC bundles of at most 1400 bytes over UDP; nothing is merged, two values of one address both leave, in order.
- `ossia.v8connector`: attribute `@bundle` (default 1). The values an instance sets during a tick leave together at the end of the tick, in OSC bundles of at most 1400 bytes over UDP; a parameter set several times leaves once with its last value, and with `@repetitions 0` a value equal to the current one does not leave. `@bundle 0` sends one message per value, as before.
- `examples/ossia.v8connector.load.maxpat`: 3000 parameters set at each frame and followed by a client in the same Max, drawn as a cloud of points, with a toggle for `@bundle`.
- Extras menu: `ossia-prism examples` opens each example of `ossia.v8connector` (demo, grid, load) in one click.
- A load bench (`v8connector_load`), whose results are in the README.

### Changed

- Frames on the network, by default with OSCQuery: the values of `ossia.parameter`, `ossia.remote` and `ossia.view` leave at the end of the scheduler's tick in OSC bundles, no longer one message each as soon as they are pushed. A peer that decodes OSC bundles sees the same values in the same order; `@bundle 0` on the `ossia.device` or `ossia.client` restores the previous frames.
- `ossia.client` connected by URL: `connect 1 ...` comes out a little after the `connect` message, when the connection made on its own thread is done (tens of milliseconds on a local network), no longer before `connect` returns.
- With `@bundle 1`, the default, the values of `ossia.v8connector` reach the network at the end of the tick, in bundles, instead of one message as soon as each is set. A peer that decodes OSC bundles sees the same values; `@bundle 0` restores the previous frames.

### Fixed

- Clicking `connect` on an `ossia.client` connected by URL could crash Max: its poll, on the scheduler thread, applied the tree changes of the mirror that `connect`, on the main thread, was destroying. The poll and `connect`, `disconnect` and the end of a connection no longer run at once. The race was there in `2.1.0+prism.4` and in ossia 2.1.0.
- Deleting the global `[ossia]` object stops the servers it exposed and frees their ports: exposing again on the same ports works. Before, they stayed on the global device, unpolled.
- `ossia.v8connector`: a parameter created resolves again only the bindings it can satisfy. With 3000 binds waiting and 3000 parameters arriving one by one, every creation resolved every binding: 15 s in one poll of the client, Max frozen, the client's HTTP requests timing out.
- The load example connects its client once the emitter has declared its leaves (a `declared` message), instead of 500 ms after load; `[print load_view]`, which flooded the console, is gone.
- `expose` sent to the global `[ossia]` object crashed Max (null network context), for instance on loading `examples/model_view_example.maxpat` or clicking `expose oscquery` in `examples/pattern-matching_examples.maxpat`. The defect existed in `2.1.0+prism.4` and in ossia 2.1.0. `[ossia]` now creates and polls its network context, as `ossia.device` does.
- `ossia.client` connected by URL builds its device and reads the tree on a thread of its own. On the main thread, each attempt froze Max for up to 4.5 s, and a client could never connect to an `ossia.device` of the same Max, whose server that thread polls.
- The OSCQuery client sends bundles: `push_bundle` and `push_raw_bundle` of its mirror were TODOs that sent nothing.
- A device exposed through a multiplex (`ossia.device`) forwards a bundle to its protocols, instead of pushing its values one by one.
- The OSCQuery client and server send bundles of values in which a parameter may come several times (`push_bundle` of `bundle_element`), instead of setting and pushing each value again.
- `ossia.v8connector` finds an address through an index: setting 3000 bound addresses took 9.1 ms per tick with bundles, 17.6 ms without, now 1.8 ms and 8.6 ms.

## [2.1.0+prism.4] - 2026-10-30

Published for macOS universal (Apple Silicon and Intel). Windows x64 to come: same version, added to the Release later.

### Added

- `ossia.client`: attribute `@reconnect` (default 1), see Fixed.
- `[ossia.v8connector]` without an argument: the parameters sit directly under the node of the enclosing `ossia.model`, or the root of the enclosing `ossia.device` or `ossia.client`, or the root of the global device, next to the `ossia.parameter` objects. An address already taken, by another instance or by an `ossia.parameter`, is refused as `refused <address> already exists`, never renamed. The instance removes only its own parameters and the nodes they created. With an argument, nothing changes.

### Fixed

- `ossia.client` connected by URL (`connect oscquery ws://host:port`) received nothing more once the server's WebSocket went down (the server stopped, for instance a TouchDesigner component turned off), reported nothing and never reconnected, while its own sends still left over UDP. It now reports the loss with `connect 0 <parameters>` on its outlet, tries again every second from the main thread until the server answers on its port, then connects as the first time: the tree is read again, every followed parameter (`bind` of `ossia.v8connector`, `ossia.remote`) subscribes again with `LISTEN`, and the outlet sends `connect 1 <parameters>`. A `disconnect`, a new `connect` or the deletion of the object stop the attempts. New attribute `@reconnect` (default 1) to turn this off.
- Max could crash in `ossia.v8connector` when a device it follows with `bind` went away and came back, for instance a server found by name whose announcement disappears and reappears. The object kept a pointer to the destroyed device, because `ossia.client` clears its device before it announces its removal; it now notices the destruction of every device it knows by itself.
- `ossia.client` connected by name (`connect <name>`) applies the tree changes the server sends after the connection (`PATH_ADDED`, `PATH_REMOVED`, `ATTRIBUTES_CHANGED`), as a client connected by URL does: a parameter added on the server appears, and a `bind` to it resolves.

## [2.1.0+prism.3] - 2026-10-30

Published for macOS universal (Apple Silicon and Intel). Windows x64 to come: same version, added to the Release later.

### Breaking

- `add` refuses a `@type` that its `@unit` would change, with the reason (`color.rgb` is `vec3f`, `color.rgba` is `vec4f`, `gain.db` is `float`). Such a declaration was accepted before, and the parameter silently took the type of the unit.
- Every refusal of `add`, `bind`, `set`, `remove` or `unbind` also comes out of the left outlet as `refused <address> <reason>`. `refused` is now a reserved address, like `ready`; a script that forwards every message of the left outlet without the helper receives these messages too.

### Added

- `@clip none`, the OSCQuery name of the free mode, is accepted by `ossia.v8connector` (`ossia.parameter` still expects `free`).
- The JavaScript helper reports `refused` messages in the Max console and calls `onRefused(address, reason)` when it is set.
- Recipes in the README, the help and `docs/prism/LISEZMOI.txt`: adding and removing parameters from a script, which instance owns a parameter, the role of the name argument.
- The list of differences from ossia 2.1.0, fixed and known defects, in the README.
- A test of the JavaScript helper outside Max, `tests/Network/V8ConnectorHelperTest.js`, run with Node.
- The ossia-prism section of the README and this changelog.

### Fixed

- The OSCQuery server writes a single `EXTENDED_TYPE` key per parameter. For a parameter with a unit, libossia 2.1.0 wrote the unit's `EXTENDED_TYPE` (the components of a color) then the node's own extended type (`vecf`) under the same key, so a client keeping the last duplicate key lost the color. This applies to every object that publishes a parameter, not only `ossia.v8connector`.

## [2.1.0+prism.2] - 2026-10-23

### Changed

- The Max package is named `ossia-prism`: install folder, name in `package-info.json`, and the line `ossia-prism 2.1.0+prism.2, PRISM CNRS` posted in the Max console when the object loads. The objects keep their `ossia.*` names and existing patchers open unchanged; remove the `ossia` folder installed by `2.1.0+prism.1` or by the official package before installing this one.
- Help patcher: new Limits tab; layout of the help and demo patchers.
- The help, the demo and the reference page name the author, Julien Bayle, PRISM (CNRS); the reference page shows a minimal script using the JavaScript helper.

## [2.1.0+prism.1] - 2026-10-23

### Added

- `ossia.v8connector`, a node of the ossia tree whose parameters are declared by messages from a `v8` or `v8ui` script: `add` (attribute names of `ossia.parameter`; a numeric type requires `@min` and `@max`, or `@range`), `remove`, `clear`, `set`. It registers like `ossia.model`, and creates its node on the first `add`.
- `bind` and `unbind`: follow a parameter that exists elsewhere, addressed as `ossia.remote` does, without creating anything.
- Left outlet: values changed by another source, bounded to the range then filtered (mute, enable, repetitions) like the outlet of every ossia object, a value corrected by the parameter sent back to its sender, and `ready` once the object has registered. Initialization follows `ossia.parameter`: the default is stored, then output.
- The JavaScript helper `javascript/ossia.v8connector.js` (`declare`, `add`, `remove`, `set`, `bind`, `on`, `dispatch`), which reaches the object by its scripting name and replays its declarations on `ready`, so the object can be deleted and recreated.
- Help and demo patchers, reference page, and the headless test of the object's core, `tests/Network/V8ConnectorTest.cpp`.
- Package version `2.1.0+prism.1`, installed in a folder named `ossia`.

[2.1.0+prism.2]: https://gitlab.prism.cnrs.fr/labo/outils/libossia/-/tags/v2.1.0%2Bprism.2
[2.1.0+prism.1]: https://gitlab.prism.cnrs.fr/labo/outils/libossia/-/tags/v2.1.0%2Bprism.1
