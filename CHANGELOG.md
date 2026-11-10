# Changelog

All notable changes to **ossia-prism**, the Max package built from this repository (ossia 2.1.0 for Max plus `ossia.v8connector`), and to **ossia (PRISM)**, the Unity package built from it (`io.ossia.ossia-unity`), are documented in this file. The rest of libossia has no changelog here.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/). Versions follow [Semantic Versioning](https://semver.org/) as `UPSTREAM+prism.N`: the ossia version the package is built from, then the number of the ossia-prism release on top of it.

## [Unreleased]

### Behavior changes

- OSCQuery server: `ACCESS` is applied. What a client writes to a read-only parameter (`GET`) is ignored, over UDP, over the WebSocket, as JSON and through a pattern; `?VALUE` on a write-only parameter (`SET`) answers 204. What stops working: a client that writes over the network to a parameter published as `GET`. A value set locally, by the patch or the script, is not affected.
- OSCQuery server: as in the proposal, a client gets only the addresses it sent `LISTEN` for, one address each, and `IGNORE` stops one: a `LISTEN` on a container delivers nothing below it, and a client that never sent `LISTEN` gets no value. An address counts from the moment the server has read its `LISTEN`. The libossia mirror sends `LISTEN` for every parameter that has a callback (`ossia.remote`, `bind` of `ossia.v8connector`, `Ossia.Remote`), and reading a parameter without one asks the server for its value, without waiting: the read returns the last value known, and asks at most every 100 ms for an address (one request in flight, given up after 1 s). Read every frame (an `Update` of Unity, a `metro` on `namespace` in Max), it took 0.5 to 1.2 ms and sent 60 requests per second when it waited for the answer; it now takes 0.01 to 0.03 ms and sends 9 to 10 per second; 50 parameters read in one frame took 67 ms, now 2 to 6 ms. A callback (`ossia.remote`, `Ossia.Remote`) is the way to follow a value. What stops working: a client that relied on receiving values without sending `LISTEN` for them, or on receiving the addresses it did not listen to.

### Fixed

- OSCQuery client: an HTTP error names the host, the port and the request (`HTTP Error: <host>:<port><request>: <error>`). The line `HTTP Error: Host not found (authoritative)` seen in Max's console named neither: on macOS, a device found by name is reached at the host name of its announcement, which each request of the client resolves again, and a name that does not resolve at that moment gave that line with nothing to tell which device. Test `C API: an HTTP error names the host and the request`.

## [2.1.0+prism.11] - 2026-11-09

Max no longer freezes when a Minuit device leaves the network, nor announces services without end when one comes; its connections to announced devices name the package; an `ossia.remote` gets the value of a node added while connected once; and a value written to the inlet of `ossia.parameter` is no longer lost while the object outputs a value of the network.

### Added

- Build: configuring the Max package stops when the first line of `docs/prism/LISEZMOI.txt`, which the package carries at its root, is not `ossia-prism <version of the package>`, or when the sources are built at a release tag other than `v<version>` (`src/ossia-max/check_version.cmake`).
- Tests: `tests/Max/check_minuit.py`, a Minuit device that comes makes no service of Max's own on the network, and one that leaves does not freeze Max (it freezes a Max with the defect, and asks for `--may-freeze`); `minuit_listener_device_does_not_announce`, the same without Max; `oscquery_directory_names_its_client`, the name the package gives a server; `oscquery_mirror_added_node_value_once`, the value of a node the server adds, once; `tests/Max/check_remote.py`, the same in Max, an `ossia.remote` by URL and by name, against a libossia server in another process; `tests/Max/check_outlet.py`, what `ossia.parameter` outputs when a value of the network and a value of the inlet come together; in Unity, `ClientTests.AddressAddedLaterGetsItsValueOnce` and `ClientTests.ServerListsTheClientUnderItsOwnName`, the second a guard: the name of a client of Unity does not change in this version.

### Changed

- Max: the version of the package is `2.1.0+prism.11`; `docs/prism/LISEZMOI.txt` says it (it still said `2.1.0+prism.9`).

### Fixed

- Max: a Minuit device that comes no longer makes Max announce services without end. Each client the listener of `_minuit._tcp` built announced itself, the listener built a client for that announcement, and so on: 2576 services after three runs of the test in one Max, Max at about 700 % CPU, until it quit; each announcement held the scheduler 634 to 745 ms. The clients of the listener no longer announce themselves: one service for one announcement, the metro of the test stopping at most 0.11 s, 3 runs of 3. Same code in ossia 2.1.0.

- Max: a Minuit device that leaves the network no longer freezes Max. The listener of `_minuit._tcp` took its own lock twice on a withdrawn announcement, on the scheduler thread with Overdrive, as soon as the package was loaded; same code in ossia 2.1.0 and up to `2.1.0+prism.10`. With `2.1.0+prism.10`, Max froze 1 s after the withdrawal; after, the metro of the test goes on, 6 runs of 6.
- Max: the connection the package opens to each announced OSCQuery device, which `connect <name>` reuses, gives the server the name `max_client_<process>`, not the name of that device: a map from `?PRISM_CONNECTIONS` no longer shows an edge from a device to itself.
- Max: an `ossia.remote` gets the value of a node the server adds while the client is connected once, not twice: the client read it again for each of the two `ATTRIBUTES_CHANGED` the server sends for a new parameter. Measured on the mirror without Max: 2 values before, 1 after. In Max (`tests/Max/check_remote.py`, by URL and by name): once in 47 runs of 48; twice by URL in one, seen once in 48 runs, before the server's log was kept, not reproduced. Without Max, 2 values once in a full run of the tests on a loaded machine, 0 of 60 when the test ran alone: a known defect, see the README.
- Unity: a `Remote` gets the value of a node the device adds while the Client is connected once: the same fix, in the mirror the Client uses. `ClientTests.AddressAddedLaterGetsItsValueOnce`, with the published packages: twice with `2.1.0+prism.10`, 3 runs of 3; once with `2.1.0+prism.11`, 3 runs of 3.
- Max: a value written to the inlet of `ossia.parameter` while the object outputs a value of the network on another thread is no longer lost without a message. With a consumer of about 100 ms on the outlet: 20 lost in 20 before, none after.
- Tools: `prism_map.py --html` draws an edge from a device to itself as a loop above its box; it was a line of no length, invisible. `test_prism_map.py` closes the sockets of its fake servers, and a test that leaves one open fails. The README of the tool cites the TouchDesigner OSCQuery component, whose server answers `?PRISM_CONNECTIONS` from `oscquery` 0.3.3.

## [2.1.0+prism.10] - 2026-11-05

Taking control, listening on one address, a map of who follows whom and a panel built from a device's tree, for Max and Unity. The Unity package: the crash at the end of Play is gone, an exposed object publishes its transform once, `[Ossia.Expose]` takes bounds, a unit and an access mode. The core: a client reads a value again after a crossing, a boolean is served as `true` or `false`, and the server answers `?PRISM_CONNECTIONS`. Max: the defects of `ossia.client`, `[ossia] @bundle`, `dump` and the range of a list are fixed.

### Breaking

- Unity: `ExposeAttributes` publishes the transform of its GameObject as `position`, `rotation` and `localScale` only. `localPosition`, `eulerAngles`, `localEulerAngles`, `localRotation`, `right`, `up`, `forward` and `lossyScale` are no longer in the tree: they said again what the three say, and, applied back, they disagreed with them (see Fixed). A client that follows or writes one of these addresses gets nothing from it. To migrate: `.../transform/eulerAngles`, `localEulerAngles` and `localRotation` become `.../transform/rotation`, the rotation in world space as a quaternion (x, y, z, w), not three angles; `localPosition` becomes `position`, in world space; `lossyScale` becomes `localScale`, relative to the parent; `up`, `right` and `forward` have no replacement: compute them from `rotation`.

### Added

- C API: `ossia_protocol_oscquery_server_asio_create_bound(context, osc_port, ws_port, address)`, the server of `ossia_protocol_oscquery_server_asio_create` listening on one address: `127.0.0.1` for the clients of the machine, the address of one interface for that network. Such a server is not announced over mDNS. Test `C API: a server on 127.0.0.1 only`: reached on 127.0.0.1, refused through another interface, not announced.
- OSCQuery server: `?PRISM_CONNECTIONS`, declared in the `EXTENSIONS` of `HOST_INFO`, lists the clients connected (address, port, the name an ossia-prism client sends with `PRISM_NAME`, OSC port, sender port, addresses listened to, packets) and the other senders of OSC, the last 256 (address, port, packets, last packet). The clients of the core send their device name when the server declares the extension. Tests `C API: ?PRISM_CONNECTIONS lists the clients and the senders`, `C API: ?PRISM_CONNECTIONS keeps the last 256 senders`.
- Tools: `tools/prism-map/prism_map.py` draws the map of the OSCQuery devices of a network from `?PRISM_CONNECTIONS` (text, JSON, Graphviz or HTML), browsing mDNS or given hosts; tested against fake servers (`test_prism_map.py`).
- Max: `@hold <ms>` on `ossia.parameter` and `ossia.remote`, taking control: after a value comes in by the inlet, the values of the network are held for that long, then the last one held comes out. Checked in Max by `tests/Max/check_max.py`.
- Max: `expose oscquery <OSC port> <WebSocket port> <address>` on `ossia.device` listens on that address only, and is not announced over mDNS; the outlet adds the address to `expose 1 oscquery <ports>`. Checked by `tests/Max/check_max.py`.
- Max: `extras/ossia.panel.html`, a page that builds a control for each parameter of an OSCQuery device from its tree, follows it over the WebSocket and holds a control for 500 ms after the user releases it; `examples/ossia.panel.maxpat` opens it in a `jweb` next to an `ossia.parameter @hold 500`. Logic tested outside a browser (`tests/Network/OssiaPanelTest.js`).
- Max: a PRISM tab in the help of `ossia.parameter`: `@hold`, `dump`, and `[pattr]` patched into the inlet.
- Unity: `[Ossia.Expose]` takes `Min`, `Max`, `Unit` and `Access` (`Tests/Runtime/ExposeOptionsTests.cs`).
- Unity: `Controller.listenAddress`, the address both ports listen on; on one address the device is not announced (`Tests/Runtime/ListenAddressTests.cs`).
- Unity: the events `ExposedObject.Received (name, value)` and `Remote.Received (value)`, raised for each value received from the network; the sample "Custom parameters" pauses its spin while a client sets the rotation (`ClientTests.RemoteRaisesReceivedForTheValuesOfTheDevice`).
- Tests: `tests/Max/check_max.py`, the fixes of Max checked in a running Max, with no click and without quitting it; `tests/binary_paths.py`, no path of the build machine in the binaries of a package.
- Unity: `Ossia.Controller.Hold()` and `Ossia.Controller.Let(bool playStops = false)` are public: a script that builds its own device, as `CustomDevice.cs` does, counts it with the Controllers and the Clients, so that the reset of the library, when Play stops, waits for all of them.

### Changed

- Max: `[ossia.client <name>]`, created with a name only, takes that name and connects nothing; it sent itself a `connect` with no argument every second. `connect` with no argument prints the help of the protocols once.
- Max: `ossia.client` connected by name takes the tree of the listener's mirror instead of reading it again: no node removed and created again, no `ossia.remote` registered again.
- Unity: `[Ossia.Expose]` without a name takes the name of the field or property; the public fields a script inherits are exposed; a `Vector2`, `Vector3` or `Vector4` has no unit of its own (the transform's `position` keeps `position.cart3D`); `ExposeComponents` no longer prints a line per property.

### Removed

- Max: `@trim_address` of `ossia.v8connector`, which nothing read.

### Fixed

- OSCQuery server: a boolean is served as `true` or `false`; it was `null` (`value_to_json.hpp`). Test `C API: a bool is served as true or false`.
- OSCQuery client: when a value received crosses one the client sent on the same address less than 500 ms before, the client asks the server for the value 100 ms later. Against a server that does not answer crossed writes, 20 divergences in 20 crossings before, 0 in 100 after. A value read again that did not change is not a new one: what follows the parameter does not get it twice. Tests `C API: a mirror reads the value again after a crossing`, `C API: a value read again that did not change is not a new value`.
- OSCQuery client of asio (`oscquery_mirror_asio.cpp`): it declares the port its values leave from; it declared a random one, and a server with echo sent the client's values back to it. Test `C API: ossia_device_set_echo spares an asio mirror`.
- Max: `@reconnect 0` on `ossia.client` also stops the attempts of a first connection that fails.
- Max: `[ossia]` groups what its objects push during a tick in OSC bundles (`@bundle`); the attribute did nothing.
- Max: `dump` on `ossia.parameter` and `ossia.remote` sends every attribute by the right outlet; it was written as a value.
- Max: a list parameter bounded by `@min` and `@max` carries the range on every component; it was on the first only.
- Builds: the binaries no longer hold the paths of the machine that built them (`-ffile-prefix-map`); 70 such strings in the Max package before, 0 after.
- Unity: with Send Updates, the `Controller` no longer sets back a value an exposed object published itself when the object publishes before the `Controller` reads its message queue in the frame (the same execution order, 0, for both). The guard of `2.1.0+prism.9` kept the last publication only: the value of the frame before, sent at the end of that frame, came back after the object had published the one of this frame, and was applied. Measured in the demo, a field `rotation` went back 394 times in 4.1 s, to its own value, with no value from the network; in the test, the angle went back in 60 frames of 120 and the object turned at half its speed. A binding now keeps the values it pushed until each comes back. Play Mode test `TurningObjectTests.AngleNeverGoesBack`: a script turns an exposed object while a client sets its speed over OSC, the object publishing first.
- Unity: `ExposeAttributes` published every `Vector3` and `Quaternion` property of the transform (`rotation`, `eulerAngles`, `up`, `right`, `forward`...). Applied back, they disagreed: `up` and `right` hold a part of the orientation only, and the object took another one; with the values set back each frame, the cube of the demo alternated between two fixed angles. Play Mode test `TurningObjectTests.TransformValueWrittenBackGivesTheOrientationRead`: while a script turns the object and a client sets its speed, a client writes back, one at a time, each transform value as it read it; `right` gave `(0, 202.5, 0)` for `(20, 202.5, 0)`, `up` another orientation.
- Unity: leaving Play in the editor with an `Ossia.Client` connected no longer crashes Unity. In `2.1.0+prism.9`, the `OnApplicationQuit` of the Controller reset the library (`ossia_device_reset_static`), which deleted the Client's mirror with the Controller's device; the Client, disabled afterwards, freed that mirror again in `OnDisable`: a SIGSEGV, a bus error or an abort in `ossia_device_free`, at the first Stop or later. The Controllers and the Clients now count the devices they hold, and the library is reset once the last of them has freed its own. Edit Mode test `PlayCycleTests.StopWithAClientConnected`: a Controller, a Client that follows it and a Remote, Play then Stop twice. A player quits in the same order: not measured.
- Unity: the `Controller` reads the value each message of its queue carries, not the value the parameter holds when it reads the queue: two messages of an address in one frame applied the same value twice, and a value the object published could count as received. A value that a later one replaced before the queue is read is not applied: the field keeps what the parameter and the clients hold.
- Unity: the example `CustomDevice.cs` no longer resets the library unconditionally in its `OnApplicationQuit`, which crashed Unity the same way when a Client was in the scene. It holds its device through the shared count (`Controller.Hold()` once `newDevice` is made, `Controller.Let(true)` in `OnApplicationQuit`), and the reset that frees `newDevice` and its ports waits for the Clients and the Controllers. Edit Mode test `PlayCycleTests.StopWithCustomDeviceAndAClient`: `CustomDevice` and a Client that follows it, with a Remote, Play then Stop twice, the ports free after each Stop.

## [2.1.0+prism.9] - 2026-11-03

Unity package: Send Updates on by default, no echo of a received value, a Controller that frees its ports when it is destroyed, the server of ossia-max with the changes of a frame in OSC bundles, and a client, `Ossia.Client` and `Ossia.Remote`, that follows a remote device. C API: five functions for a client and for a server that sends bundles. Max package: a PRISM tab in two helps; its objects are those of `2.1.0+prism.8`, but an `ossia.device` exposed over OSCQuery behaves differently through the core it is built on: it no longer sends a client's value back to it, and it answers a crossed write. The core: the OSCQuery server of ossia-max knows which client a value comes from, and answers the writes that cross one of its values.

### Added

- Max: the helps of `ossia.device` and `ossia.client` have a PRISM tab, with examples on ports 7411 to 7414: for `ossia.device`, `expose oscquery` and the mDNS announcement, `announced <name>`, `stop <index>`, `@bundle`; for `ossia.client`, connection by name and by address to a device of the tab, reconnection and `@reconnect` as it behaves, the loop of `[ossia.client <name>]` at its creation, the tree read again by name, `@bundle`, values in both directions.
- Unity: `Ossia.Client` follows a remote OSCQuery device, by its mDNS name or by `ws://host:port`, on a thread of its own, shows where the connection stands, applies the tree changes, and connects again every second when the server goes away, until it is back. `Ossia.Remote` binds a field or a property of a script to an address of that device, both ways, on the main thread: every change of the field leaves whatever its source, a value received does not go back, and the changes of a frame leave in one OSC bundle. Measured against a libossia server: the tree read, the server's value applied, the field sent (script, Inspector, animation), 0 echo over 5 s of concurrent writes, reconnection, connection by name, eight changes of a frame in one datagram.
- Unity: the sample "Follow a device", a scene that follows the device `follow_me` of the Max patch `FollowADevice.maxpat`: Max sets a cube's size, color and spin, Unity sends its angle back.
- C API: `ossia_device_connected` (whether a mirror's WebSocket is up), `ossia_device_run_commands` (applies the tree changes a mirror received), `ossia_device_set_echo` (a server forwards a client's value to its other clients), `ossia_device_push_bundle` (sets values and sends them as OSC bundles) and `ossia_protocol_oscquery_server_asio_create` (the OSCQuery server of ossia-max, on a network context). Each has its test (`tests/C/unit-tests/unit-tests-c-network.cpp`).
- Tests: the cross bench of the Unity package with Max and TouchDesigner (`tests/Unity/cross/cross_run.py`), one command and no click: Unity as server followed by Max, TouchDesigner and a second Unity, Unity as client of Max and TouchDesigner, by address and by name, values both ways, the echo counted on each side, 20 crossed writes per pair, and the sample "Follow a device" with its Max patch; `RESULT: PASS`. The "doc only" test of "Follow a device" in a new project (`doc_follow.py`): `RESULT: PASS`.
- Unity: `forwardToClients` on the Controller, on by default: a value one client sends reaches the other clients, never its sender (`Tests/Runtime/ForwardTests.cs`).
- Unity: Play Mode tests of a client and Unity writing the same address (a libossia OSCQuery mirror in the test, a script, the Inspector, an animation), and of a Controller destroyed then created again on the same ports; an Edit Mode test of the default of Send Updates.

### Changed

- Unity: Send Updates (`sendUpdates` of `ExposeAttributes` and `ExposeComponents`) is on by default: what changes in Unity reaches the clients without a box to tick. The box stays, to turn it off, and a scene keeps the value it saved: the sample "Exposed cube" keeps it unchecked, "Custom parameters" checked. A change of behaviour, not listed under Breaking: the binding had no user yet.

- Unity: the Controller exposes its device through the OSCQuery server of ossia-max (it was the WebSocket++ server of the C API), on a network context a thread of the Controller runs; the values received still wait for the main thread. What the exposed objects change during a frame leaves at its end, in OSC bundles: eight fields changed in one frame reach a client in one datagram, where they took eight. `bundles` on the Controller, on by default, turns it off.

- Max: the version of the package is `2.1.0+prism.9`. Its objects and helps are unchanged except the PRISM tabs; `ossia.device` behaves differently through the core (see Fixed).

### Fixed

- Unity: a value received from a client does not leave again. With Send Updates, it was applied to the field, then seen as a change and pushed back to every client that listens, its sender included. Measured with a client and a script writing the same address in turns for 5 s, 297 writes each: before, every value the client wrote came back to it (298, its 297 writes and its last one), none after; the client then receives the script's 297 values and its last one, Unity and the client end on the last value written, and nothing moves once both stop. The server forwards it to the other clients instead (`forwardToClients`, below).
- Core, Max and Unity: crossed writes no longer diverge. When the server sets a value (a change in Unity, in a Max patch) and a client writes another one to the same address in the same instant, each value crosses the other: the server applied the client's value last, the client applied the server's after sending its own, and each kept the other's. The server now answers a client's write that comes less than 100 ms after it sent that client a different value on that address with its final value, to that client only; the answer opens no window, and a write past it, or that leaves the value sent, gets nothing (`oscquery_server_protocol_base::set_crossing_window`). Measured with the C API, a server and a client, 20 crossings: 20 divergences before, 0 after; with a second client and the echo on, 20 and 0; Unity's Controller and a client, 0 in 20. Listed under Fixed, and a change of behaviour as well: in this case only, a value goes back to its sender. Within the window the server cannot tell a crossing from a write made right after receiving its value: such a write gets its own value back, once.
- Core, Max and Unity: the OSCQuery server of ossia-max sent a value it forwards back to its sender too. When its device echoes (what `expose oscquery` sets in `ossia.device`, `forwardToClients` in Unity), the server forwards a client's value to its other clients; it did not tell which client the value came from (`oscquery_server_protocol_base::on_osc_message`, `on_binary_ws_message`). It now knows the client by the UDP port its values come from, as the WebSocket++ server does, and by its connection over the WebSocket. Measured with the C API, two clients A and B: B receives what A writes, A does not, over UDP and over the WebSocket; before, A received its value back (1 value). Measured in Max 9 with the cross bench (`tests/Unity/cross/`), an `ossia.device` exposed over OSCQuery and followed by a Unity client that writes 20 values: 20 came back to the client with the library of `2.1.0+prism.8`, none with that of `2.1.0+prism.9`.
- Unity: a `Controller` destroyed without leaving Play frees its device, its ports and its mDNS announcement. The C API keeps a device by its name until it is freed: the Controller kept its server, and a new Controller of the same name got the old device back, its tree and its server, its own server failing on the ports (`Underlying Transport Error`). An exposed object stops publishing once its Controller is gone.

## [2.1.0+prism.8] - 2026-11-02

Documentation, and one fix in the Unity package: no object of the Max package behaves differently from `2.1.0+prism.7`. Published for macOS universal (Apple Silicon and Intel), as `2.1.0+prism.7`.

### Added

- Unity: the sample "Custom parameters" (Package Manager, Samples): the script `CustomParameters.cs`, whose fields `speed` (`float`), `axis` (`int`), `spin` (`bool`), `color` (`Color`) and `size` (`Vector3`) are marked `[Ossia.Expose]` and turn, scale and color a cube, exposed under `/Shape/CustomParameters`; its scene, with an `OssiaController` and `ExposeAttributes` sending updates; and the Max patch `CustomParameters.maxpat`, an `ossia.client` connected by name with one `ossia.remote` per field. The build writes a `.meta` file for each script of a sample, its GUID derived from its path, so that the sample's scene finds its script once imported.

### Changed

- Unity: the package's README is rewritten: installation from a new project, declaring your own parameters with the code of the sample, the tree a client gets (`curl` output), the values in both directions and Send Updates, the accepted types, the limits of `[Ossia.Expose]` (a name only: no range, unit, access mode), `ExposeComponents` with an example, `CustomDevice.cs` and its port conflict with the `Controller`, a reference of every component with its defaults, troubleshooting. The upstream section, whose paths and settings no longer applied, is removed. `LISEZMOI.txt` becomes a short English pointer to the README.
- Max: the README gives the installation and the update in English, declaring parameters with `ossia.v8connector` and the tree a client gets, the attributes of the object and of `add` with their defaults, the helper's functions, the limits not written before, a troubleshooting table and the cross test with Unity; the upstream README, whose downloads install the official package, is replaced by a short section on upstream. `docs/prism/LISEZMOI.txt`, copied at the root of the package, becomes a short English pointer to the README.
- Max: the reference of `ossia.v8connector` lists `get` and `namespace` with what they output, and the defaults of its attributes (`@priority` is a float); the reference of `ossia.client` documents `get devices` instead of a `getdevices` message that does not exist, and the messages of its outlet; the reference of `ossia.device` names `announced` on its outlet. The help of `ossia.v8connector`: the right outlet's messages, a sentence on models that contradicted the README, and the platforms line no longer tied to `2.1.0+prism.5`. In its Basic tab, the comments that overlapped once their text was longer are moved and resized.
- Max: `@reconnect` of `ossia.client` is documented as the code behaves: it governs only the attempts after a loss. A first connection that fails, an unreachable URL or a name not found, is tried again every second whatever `@reconnect` says, until it succeeds, a `disconnect` or a new `connect`, as in ossia 2.1.0. The notes of `2.1.0+prism.6` said that `@reconnect 0` stops the attempts without this limit; the reference of `ossia.client` said that off, nothing is tried.
- Max: the README writes `stop <index>` for `ossia.device`, `stop 0` for the first `expose`, in the known defects and the troubleshooting; it said `stop`, which the object refuses with "missing arguments for message "stop"".
- Max: the README and the reference of `ossia.client` say that `[ossia.client <name>]`, created with a name only, sends itself `connect` without argument as it is created, then, every second, `connect` with nothing after it on its outlet and the help of the protocols in the console, until a `connect` with arguments or a `disconnect`; the notes of `2.1.0+prism.6` described the attempts of `connect <name>` and not this. The README also explains why a connection by name reports every node removed then created: the client reads the tree again.
- Unity: the patch of the sample "Custom parameters" creates `[ossia.client]` without argument; with `[ossia.client custom]`, it printed the help of the protocols every second from its opening.
- Unity: the Package Manager's menu item is "Install package from tarball…" in Unity 6.3; the notes of `2.1.0+prism.7` said "Add package from tarball".

### Fixed

- Unity: with Send Updates checked on `ExposeAttributes`, the `Controller` set back a field or a transform value to what `ExposeAttributes` had published the frame before, once Unity called the script that changes it between the two (Unity chooses that order). The values a binding pushes come back to it through the `Controller`'s message queue, and were applied again: the cube of the sample "Custom parameters" turned by jerks, measured in the editor at 60 frames per second at 0.45 to 0.6 times its speed, 1.0 with the fix. A binding no longer applies the value it published while the parameter still holds it; a value from a client is applied as before. The binding is that of ossia 2.1.0 (`unity3d/impl/ParameterBindings.cs`).

## [2.1.0+prism.7] - 2026-11-02

Published for macOS universal (Apple Silicon and Intel): the Max package and the Unity package. Windows x64 to come: same version, added to the Release later.

Max package **ossia-prism**: its objects behave as in `2.1.0+prism.6`. The changes of the core below serve the Unity package; ossia-max calls none of them.

Unity package **ossia (PRISM)** (`io.ossia.ossia-unity`), for macOS universal, installed with the Package Manager's "Add package from tarball". Tested with Unity 6.3 LTS on Apple Silicon, in the editor and in macOS players, Mono and IL2CPP. Not tested: macOS Intel, Windows, Linux, iOS.

### Added

- Unity: the `Controller` has the fields `deviceName`, `oscPort` and `wsPort`, set in the inspector; their defaults are the values fixed before in the code: `unity`, 1234, 5678.
- Unity: the `Controller` field `runInBackground` (on by default) sets `Application.runInBackground` at runtime, without changing the project settings, so that the scene keeps following the values received when Unity loses focus.
- Unity: the `Controller` reads the name its device is announced under over mDNS (field `resolveAnnouncedName`, on by default), on a thread of its own that browses for two seconds once the server runs: read-only property `AnnouncedName`, a console line, and a warning when the system announced it under another name than `deviceName` (`unity (2)`, the name being taken on the network). Before, nothing said so.
- C API: `ossia_oscquery_list_devices`, the OSCQuery devices announced over mDNS (name, host, WebSocket port) within the given browsing time, freed with `ossia_oscquery_devices_free`. No effect in Max: ossia-max does not call the C API.
- `ossia::net::list_oscquery_devices(timeout)`: browses for the given time; `list_oscquery_devices()` still browses for five seconds. No effect in Max: ossia-max calls neither.
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
- C API: the strings it returns are allocated with `malloc`, as `ossia_string_free` frees them with `free`; some were allocated with `new[]`. `ossia_free_string` (presets) frees them with `free` too, no longer with `delete[]`. No effect in Max: ossia-max does not call the C API.

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
