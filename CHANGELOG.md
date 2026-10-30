# Changelog

All notable changes to **ossia-prism**, the Max package built from this repository (ossia 2.1.0 for Max plus `ossia.v8connector`), are documented in this file. The rest of libossia has no changelog here.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/). Versions follow [Semantic Versioning](https://semver.org/) as `UPSTREAM+prism.N`: the ossia version the package is built from, then the number of the ossia-prism release on top of it.

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
