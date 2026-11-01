README
======

# ossia (PRISM) for Unity

The Unity package `io.ossia.ossia-unity`, displayed as "ossia (PRISM)", built from this repository: the ossia library and its C# binding, which expose objects of a Unity scene over OSCQuery. Its version is the tag it is built from, `2.1.0+prism.N`. Tested on macOS Apple Silicon, Unity 6.3 LTS, in the editor and in players built with Mono and with IL2CPP; the macOS library is universal (arm64 and x86_64).

## Installation

1. Unity: Window > Package Management > Package Manager, `+` > Add package from tarball, choose `io.ossia.ossia-unity-2.1.0+prism.N.tgz`.
2. In the Package Manager, the package's Samples tab: Import "Exposed cube". The scene `ExposedCube` holds an `OssiaController` and a cube exposed with `ExposeAttributes`.
3. Play. Check from a terminal: `curl http://localhost:5678` returns the OSCQuery tree, with `/Cube/transform/position`.

## Controller

One GameObject named `OssiaController` holds the `Ossia.Controller` component: the exposed objects find it by that name. Its fields:

| Field | Default | Role |
|---|---|---|
| `deviceName` | `unity` | Name of the device, under which it is announced over mDNS (`_oscjson._tcp`) |
| `oscPort` | `1234` | UDP port on which the device receives OSC |
| `wsPort` | `5678` | TCP port of the OSCQuery server: HTTP (the tree) and WebSocket |
| `runInBackground` | on | Keeps the scene following the values received when Unity loses focus (see below) |
| `appName` | `Unity` | Name used by the presets and the logger |

The device starts in `Awake` and stops when Play stops (`OnApplicationQuit`), which frees its ports and withdraws its mDNS announcement: a second Play in the same editor gets them back.

## Exposing objects

- `Ossia.ExposeAttributes` on a GameObject: its transform (`position`, `rotation`, `localPosition`, ...) and the fields and properties marked `[Ossia.Expose]` in its components, under `/<GameObject name>/transform/...` and `/<GameObject name>/<component type>/...`.
- `Ossia.ExposeComponents`: every public field and property of the components put in its list.

Types: `float`, `int`, `bool`, `string`, `Vector2`, `Vector3`, `Vector4`, `Quaternion` (sent x, y, z, w, unit `orientation.quaternion`), `Color` (`color.rgba`, 0 to 1), `Color32` (`color.rgba8`, 0 to 255), `float[]`, `int[]`. The values received are applied in the `Update` of the Controller, on the main thread.

## Focus

Unity stops the player loop when the application loses focus and the project's Run In Background (Project Settings > Player > Resolution and Presentation) is off, in the editor as in a player. `Update` then no longer runs, and the scene no longer follows the values received until Unity is focused again: the values wait in the queue. The Controller sets `Application.runInBackground` at runtime when its `runInBackground` field is on, the default, without changing the project settings. Turn the field off to get back the pause on focus loss.

## Test from Max

With the package `ossia-prism` in Max:

- by name: `[ossia.client]`, message `connect unity`; the outlet sends `connect 1 unity` once the device is found over mDNS;
- or by URL: `connect oscquery ws://127.0.0.1:5678`.

Then `[ossia.remote /Cube/transform/position]` fed by `[pak 0. 0. 0.]` moves the cube. Connected by name, the client follows Stop and Play as `ossia.client`'s reference describes: `disconnected` once the device is no longer announced, `connect 1 unity` when it is back.

## mDNS

In Play, the device is announced over mDNS (`_oscjson._tcp` on the WebSocket port, `_osc._udp` on the OSC port) under `deviceName`; Stop withdraws the announcement, about a second later. When another device of the network already has that name, the system announces this one as `unity (2)`.

The Controller reads the name really announced: a thread of its own browses the network for five seconds after `Awake` and picks the device announced on this machine with the Controller's WebSocket port. On the main thread, the console then shows `OSSIA: announced "unity" over mDNS (_oscjson._tcp)`, the read-only property `AnnouncedName` gets the name (null until then), and when it differs from `deviceName`, a warning: `OSSIA: announced as "unity (2)": the name "unity" is taken on the network`. A client then connects with `connect unity (2)`, or by URL.

In the editor, a Play started less than five seconds after the previous one stopped waits until that browsing ends: the domain reload waits for its thread, which cannot leave the library before.

`dns-sd -B _oscjson._tcp` (macOS) lists the names announced on the network.

## Tests

The package carries Edit Mode and Play Mode tests (`Tests/`). To run them, list the package in the `testables` of the project's `Packages/manifest.json`:

```
"testables": [ "io.ossia.ossia-unity" ]
```

then Window > General > Test Runner, or in batch mode: `Unity -batchmode -projectPath <project> -runTests -testPlatform EditMode -testResults results.xml`. The second Play and mDNS tests use ports 1234, 5678, 21234 and 25678, and `dns-sd` (macOS).

## Not tested

macOS Intel; Windows, Linux, iOS.

# Basic usage: 

## High-level API: 

### Setup
* Create an empty GameObject, named "OssiaController". 
* Add the script `ossia/Controller.cs` to it.
* Optionally, `ossia/PresetController.cs` can be added for preset and device loading / saving.
* Optionally, `ossia/Logger.cs` can be added for a logging object.

OssiaController should look like this: 

![OssiaController](https://github.com/ossia/ossia.github.io/blob/dev/source/images/unity/OssiaController.png)

Then once this is done, set the script execution order so that the objects are loaded in the correct order: 

![ScriptOrder](https://github.com/ossia/ossia.github.io/blob/dev/source/images/unity/ScriptOrder.png)

### Exposing objects 

To expose objects to the API, either : 

* Add the `ossia/ExposeAttributes.cs` to an object. 
  Then, in your own scripts, mark fields and properties with `[Ossia.Expose]` like in the example `ExampleAttribute.cs`: 

![ExposeAttributes](https://github.com/ossia/ossia.github.io/blob/dev/source/images/unity/ExposeCube.png)

* Or add the `ossia/ExposeComponents.cs` to an object.
  Then, add all the components that you wish to expose to the list, by dragging and dropping them.

![ExposeAttributes](https://github.com/ossia/ossia.github.io/blob/dev/source/images/unity/ExposeCylinder.png)

## Low-level C# API:
* An example is given in `ossia/examples/CustomDevice.cs`

# TODO

* Materials
* Instance support when loading presets

# possible issues

* under some circumptance on windows, you might have a DLL not found exception. Check first that the dll are in Plugins folder at root directory of your project. If you still have a DLL not found exception, you might need to install [vc_redist](https://github.com/ossia/libossia/issues/530)
