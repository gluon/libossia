# ossia (PRISM) for Unity

The Unity package `io.ossia.ossia-unity`, displayed as "ossia (PRISM)": it exposes the objects of a Unity scene, and the fields of your own scripts, as an OSCQuery device that Max (package `ossia-prism`), TouchDesigner or any OSCQuery client can read and drive. It is built from this repository, with the ossia library and its C# binding; its version is the tag it is built from, `2.1.0+prism.N`.

Prerequisites: Unity 6.3 LTS on macOS (the library is universal, arm64 and x86_64). Tested on macOS 15 Apple Silicon with Unity 6000.3.27f2, in the editor and in players built with Mono and with IL2CPP, in projects on the Built-in and on the Universal render pipeline. Not tested: macOS Intel, Windows, Linux, iOS; the package holds no library for Windows, Linux or iOS (see [Not tested](#not-tested)).

## Installation

1. Download `io.ossia.ossia-unity-2.1.0+prism.N.tgz` from the [Releases](https://gitlab.prism.cnrs.fr/labo/outils/libossia/-/releases) page. Keep the file where it is: the project refers to it by its path.
2. Unity Hub > New project, template "Universal 3D" or "3D (Built-In Render Pipeline)", Create project.
3. In the editor: Window > Package Management > Package Manager, `+` (top left) > Install package from tarball…, choose the `.tgz`. The package appears as "ossia (PRISM)".
4. Select it in the Package Manager, Samples tab, Import "Custom parameters". The sample is copied into `Assets/Samples/ossia (PRISM)/<version>/Custom parameters`.
5. Open the scene `CustomParameters` from that folder, press Play. The console shows `OSSIA: Starting`, then `OSSIA: announced "unity" over mDNS (_oscjson._tcp)`; the cube turns.
6. From a terminal:

   ```
   curl -s http://localhost:5678/Shape/CustomParameters
   ```

   returns the five parameters of the sample (shown under [What a client sees](#what-a-client-sees)).
7. Drive it from Max: open `CustomParameters.maxpat`, in the same folder, with the package `ossia-prism` installed in Max, and click `connect unity` (see [Driving from Max](#driving-from-max)).

## Samples

| Sample | Content |
|---|---|
| Custom parameters | The scene `CustomParameters`: an `OssiaController` and a cube named `Shape` with the script `CustomParameters.cs`, whose five fields (`float`, `int`, `bool`, `Color`, `Vector3`) are exposed under `/Shape/CustomParameters`; `ExposeAttributes` with Send Updates checked. The Max patch `CustomParameters.maxpat` drives each field and shows what Unity sends back. |
| Exposed cube | The scene `ExposedCube`: an `OssiaController` and a cube whose transform is exposed (`/Cube/transform/position`, `rotation`, `localScale`, ...), with no script of its own. |

Both run on ports 1234 (OSC) and 5678 (HTTP and WebSocket), under the device name `unity`.

## Declaring your own parameters

Three things in the scene:

1. A GameObject named exactly `OssiaController`, with the component `Ossia.Controller` (Add Component > search "Controller"). One per scene; the exposed objects find it by that name.
2. Your script, on any GameObject, with the fields to expose marked `[Ossia.Expose("<name>")]`.
3. The component `Ossia.ExposeAttributes` on that same GameObject. It reads the marked fields when the scene starts and publishes each one under `/<GameObject name>/<script class>/<name>`. It also always publishes the GameObject's transform, under `/<GameObject name>/transform/...`.

The script of the sample, `CustomParameters.cs`, copied as is into `Assets` (the file name must be the class name), on a cube named `Shape`:

```csharp
using UnityEngine;
using UnityEngine.Rendering;

// Five fields of a script, exposed over OSCQuery. The ExposeAttributes
// component of the same GameObject finds the fields marked [Ossia.Expose]
// and publishes each one under /<GameObject name>/<script class>/<name>,
// here /Shape/CustomParameters/speed, /Shape/CustomParameters/axis, ...
// A value sent by a client changes the field; with Send Updates checked on
// ExposeAttributes, a change made in Unity is sent to the clients.
public class CustomParameters : MonoBehaviour
{
  // Rotation speed, in degrees per second
  [Ossia.Expose("speed")]
  public float speed = 45;

  // Rotation axis: 0 x, 1 y, 2 z (any other value: y)
  [Ossia.Expose("axis")]
  public int axis = 1;

  // Rotation on or off
  [Ossia.Expose("spin")]
  public bool spin = true;

  // Color of the shape, r g b a from 0 to 1
  [Ossia.Expose("color")]
  public Color color = new Color(1f, 0.5f, 0f, 1f);

  // Scale of the shape, x y z
  [Ossia.Expose("size")]
  public Vector3 size = Vector3.one;

  Material material;

  void Start()
  {
    var r = GetComponent<Renderer>();
    // The cube of the scene has Unity's built-in default material, which
    // the Universal and High Definition render pipelines cannot draw.
    if (GraphicsSettings.currentRenderPipeline != null) {
      var lit = Shader.Find("Universal Render Pipeline/Lit");
      if (lit == null)
        lit = Shader.Find("HDRP/Lit");
      if (lit != null)
        r.sharedMaterial = new Material(lit);
    }
    material = r.material;
  }

  void Update()
  {
    if (spin) {
      var a = axis == 0 ? Vector3.right : axis == 2 ? Vector3.forward : Vector3.up;
      transform.Rotate(a, speed * Time.deltaTime);
    }
    transform.localScale = size;
    material.color = color;
  }
}
```

The `Start` method only matters for a cube made by the sample's scene file; a cube created in your own project with GameObject > 3D Object > Cube already has the material of its render pipeline, and `material = GetComponent<Renderer>().material;` is enough.

### What a client sees

Play, then `curl -s http://localhost:5678/Shape/CustomParameters` (output of the sample, unchanged):

```
{"FULL_PATH":"/Shape/CustomParameters","CONTENTS":{"speed":{"FULL_PATH":"/Shape/CustomParameters/speed","TYPE":"f","VALUE":45.0,"ACCESS":3,"CLIPMODE":"none"},"axis":{"FULL_PATH":"/Shape/CustomParameters/axis","TYPE":"i","VALUE":1,"ACCESS":3,"CLIPMODE":"none"},"spin":{"FULL_PATH":"/Shape/CustomParameters/spin","TYPE":"T","VALUE":null,"ACCESS":3,"CLIPMODE":"none"},"color":{"FULL_PATH":"/Shape/CustomParameters/color","TYPE":"ffff","VALUE":[1.0,0.5,0.0,1.0],"ACCESS":3,"CLIPMODE":"none","EXTENDED_TYPE":["color.rgb.r","color.rgb.g","color.rgb.b","color.rgb.a"]},"size":{"FULL_PATH":"/Shape/CustomParameters/size","TYPE":"fff","VALUE":[1.0,1.0,1.0],"ACCESS":3,"CLIPMODE":"none","EXTENDED_TYPE":["position.cartesian.x","position.cartesian.y","position.cartesian.z"]}}}
```

Pipe it into `python3 -m json.tool` to read it indented. In short:

| Address | `TYPE` | `VALUE` at start | `EXTENDED_TYPE` |
|---|---|---|---|
| `/Shape/CustomParameters/speed` | `f` | `45.0` | |
| `/Shape/CustomParameters/axis` | `i` | `1` | |
| `/Shape/CustomParameters/spin` | `T` | `null` (see [Limits](#limits)) | |
| `/Shape/CustomParameters/color` | `ffff` | `[1.0, 0.5, 0.0, 1.0]` | `color.rgb.r` ... `color.rgb.a` |
| `/Shape/CustomParameters/size` | `fff` | `[1.0, 1.0, 1.0]` | `position.cartesian.x` ... `z` |

`ACCESS` is 3 (read and write) for every parameter, `CLIPMODE` is `none`, and there is no `RANGE`. One value: `curl -s "http://localhost:5678/Shape/CustomParameters/speed?VALUE"` returns `{"VALUE":45.0}`.

### Values in both directions

- **Client to Unity, always.** A value sent by a client (an `ossia.remote` in Max, an OSC message to UDP port 1234, a WebSocket message) changes the field, in the `Update` of the Controller, on the main thread, at the next frame. Measured on the sample: after `/Shape/CustomParameters/speed 90.` (`,f`), `axis 0` (`,i`), `spin` false (`,F`), `color 0. 1. 0. 1.` and `size 2. 1. 0.5`, the fields read `speed=90 axis=0 spin=False color=RGBA(0, 1, 0, 1) size=(2, 1, 0.5)`, and the cube's scale is `(2, 1, 0.5)`.
- **Unity to client, with Send Updates checked** on `ExposeAttributes` (field `sendUpdates`, off by default, on in the sample's scene). Each frame, a field whose value changed, in the Inspector or by a script, is pushed to the clients that listen to it (an `ossia.remote` shows it on its outlet). Measured on the sample: after setting in Unity `speed=12.5 axis=2 color=(0, 0, 1, 1) size=(0.5, 0.5, 0.5)`, `?VALUE` returns `12.5`, `2`, `[0.0,0.0,1.0,1.0]` and `[0.5,0.5,0.5]`. With Send Updates unchecked, `speed` set to 7 in Unity stays `12.5` for the client.

The initial values of the fields are published when the scene starts, whatever Send Updates says. When a script sets a field every frame (as the sample does with `transform.localScale = size`), what a client sends to that field is overwritten at the next frame.

### Accepted types

What `[Ossia.Expose]` accepts, with what a client sees (measured):

| C# type | `TYPE` | `EXTENDED_TYPE` (unit) | Note |
|---|---|---|---|
| `float`, `double` | `f` | | `double` is sent as a 32-bit float |
| `int` | `i` | | |
| `bool` | `T` | | `VALUE` reads `null` over HTTP |
| `string` | `s` | | |
| `Vector2`, `Vector3`, `Vector4` | `ff`, `fff`, `ffff` | `position.cartesian.*` | the unit is a position, whatever the field means |
| `Quaternion` | `ffff` | `orientation.quaternion` | x, y, z, w |
| `Color` | `ffff` | `color.rgb.r` ... `color.rgb.a` | r, g, b, a from 0 to 1 |
| `Color32` | `r` | | sent as an OSC color, `#RRGGBBAA` over HTTP |
| `float[]`, `int[]` | `ff...`, `ii...` | `list` | the length is that of the array at start |

A field of another type (`long`, `Texture2D`, an enum, a class) is skipped without a message: it does not appear in the tree.

What is read: the public fields declared in the script's own class, and its public properties (inherited ones included). A property needs a `get` to be published and a `set` to receive values. Not read: private fields, and public fields declared in a parent class.

### Limits

- `[Ossia.Expose]` takes a name and nothing else: no bounds (`RANGE`), no unit of your choice, no access mode, no clip mode, no description. Every parameter is read-write, unbounded, with the unit its type gives it (table above). Clamp the received values in your script, as the sample does for `axis`. `ossia.parameter` in Max offers all of these; `[Ossia.Expose]` does not, in this version.
- The name is required: `[Ossia.Expose]` without one creates a parameter with an empty name, at `/<GameObject>/<class>/`, that a client can hardly address.
- The node of a script is its full class name: a class `Params` in the namespace `MyGame` gives `/<GameObject>/MyGame.Params/...`.
- The GameObject's name is read once, when the scene starts: renaming it in Play does not move its parameters.
- `bool` parameters read `"VALUE":null` over HTTP (`curl`, the tree at connection), while their OSC and WebSocket values are right.

### ExposeComponents

`Ossia.ExposeComponents` publishes every public field and property of the components put in its list, with no attribute to write. Example, a `Light` on a GameObject named `Lamp`: add `ExposeComponents`, set its Node Name to `Lamp`, drag the `Light` component into its Components list, Play. Measured:

```
curl -s "http://localhost:5678/Lamp/Light/intensity?VALUE"
{"VALUE":1.0}
```

The tree holds `/Lamp/Light/color`, `/Lamp/Light/intensity`, `/Lamp/Light/range`, `/Lamp/Light/spotAngle`, `/Lamp/Light/enabled` and some forty other properties of `Light`. An OSC message `/Lamp/Light/intensity 3.5` sets the light's intensity to 3.5. The node of a component is its class name without namespace (`Light`), under Node Name (default `Object`). Its fields: `NodeName` (`Object`), `components` (empty), `sendUpdates` (off). It writes one `Prop: <name> => <type>` line in the console for each property it reads.

### Low-level API: CustomDevice.cs

`Scripts/unity3d/examples/CustomDevice.cs`, in the package, builds a device with the C# API alone (`Ossia.Device`, `Ossia.Node.CreateNode`, `CreateParameter`, `PushValue`), with no Controller: it is the way to build a tree that does not follow the scene. It creates its own device, `newDevice`, on ports 1234 (OSC) and 5678 (WebSocket), the ports of the Controller.

**Do not put it in a scene that has an `OssiaController` with the default ports.** Nothing is reported: both servers start, and a client reaching port 5678 gets the tree of `CustomDevice` (`/foo/blu`, `/my_string`, ...) instead of the scene's. Change the ports in `CustomDevice.cs` (copy it into `Assets` under another class name) or on the Controller.

## Driving from Max

With the package `ossia-prism` in Max, the sample's `CustomParameters.maxpat`:

- `[ossia.client custom]`, message `connect unity`: connects by name (mDNS); the console shows `client: connect 1 unity` once the device is found. Or `connect oscquery ws://127.0.0.1:5678`, by address.
- one `[ossia.remote /Shape/CustomParameters/<name>]` per field: a number box for `speed`, a number box from 0 to 2 for `axis`, a toggle for `spin`, four number boxes into `[pak 1. 0.5 0. 1.]` for `color`, three into `[pak 1. 1. 1.]` for `size`;
- right of each remote, a message box shows what Unity sends back (Send Updates is checked in the sample's scene).

For your own script: one `[ossia.remote /<GameObject>/<class>/<name>]` per field, the address read with `curl` as above. Connected by name, the client follows Stop and Play: `disconnected` once the device is no longer announced, `connect 1 unity` when it is back.

## Reference

### Ossia.Controller

On the GameObject named `OssiaController`.

| Field | Default | Role |
|---|---|---|
| `deviceName` | `unity` | Name of the device, under which it is announced over mDNS (`_oscjson._tcp`) |
| `oscPort` | `1234` | UDP port on which the device receives OSC |
| `wsPort` | `5678` | TCP port of the OSCQuery server: HTTP (the tree) and WebSocket |
| `runInBackground` | on | Keeps the scene following the values received when Unity loses focus (see [Troubleshooting](#troubleshooting)) |
| `resolveAnnouncedName` | on | Reads the name the device is announced under over mDNS (see [mDNS](#mdns)) |
| `appName` | `Unity` | Name used by `PresetController` and `Logger` |
| `AnnouncedName` (read only) | null | The name the device is announced under, once found |

The device starts in `Awake` and stops when Play stops (`OnApplicationQuit`), which frees its ports and withdraws its mDNS announcement: a second Play in the same editor gets them back. The values received are applied in the Controller's `Update`.

### Ossia.ExposeAttributes

| Field | Default | Role |
|---|---|---|
| `sendUpdates` | off | Pushes to the clients, each frame, the fields and transform values changed in Unity |

Publishes the transform (`position`, `localPosition`, `eulerAngles`, `localEulerAngles`, `right`, `up`, `forward`, `rotation`, `localRotation`, `localScale`, `lossyScale`) and the fields and properties marked `[Ossia.Expose]` of every component of its GameObject.

### Ossia.Expose

`[Ossia.Expose("<name>")]` on a public field or property: the name of the parameter under the script's node. See [Accepted types](#accepted-types) and [Limits](#limits).

### Ossia.ExposeComponents

See [ExposeComponents](#exposecomponents).

### Inherited from ossia, not tested

- `PresetController`: fields `loadPresetOnStart` (off), `presetPath` (`Assets/preset.json`), `devicePath` (`Assets/device.json`), and three Inspector buttons, Write Device, Read Preset, Write Preset, which save the tree or the values to JSON and read them back.
- `Ossia.Logger`: fields `host` (`ws://127.0.0.1:1337`) and `cmdLine` (`/path/to/Unity`); sends the logs of the library to an ossia log server.

## mDNS

In Play, the device is announced over mDNS (`_oscjson._tcp` on the WebSocket port, `_osc._udp` on the OSC port) under `deviceName`; Stop withdraws the announcement, about a second later. When another device of the network already has that name, the system announces this one as `unity (2)`.

When `resolveAnnouncedName` is on, the default, the Controller reads the name really announced: once its server runs, a thread of its own browses the network for two seconds and picks the device announced on this machine with the Controller's WebSocket port. On the main thread, the console then shows `OSSIA: announced "unity" over mDNS (_oscjson._tcp)`, the property `AnnouncedName` gets the name, and when it differs from `deviceName`, a warning: `OSSIA: announced as "unity (2)": the name "unity" is taken on the network`.

In the editor, a Play started less than two seconds after the previous one waits until that browsing ends, up to two seconds: the domain reload waits for its thread, which cannot leave the library before. Turn `resolveAnnouncedName` off to do without it.

`dns-sd -B _oscjson._tcp` (macOS) lists the names announced on the network. mDNS works within one subnet: multicast 224.0.0.251 on UDP 5353 must pass, Wi-Fi client isolation must be off; elsewhere, connect by address.

## Troubleshooting

| Symptom | Cause | Action |
|---|---|---|
| `DllNotFoundException: ossia` in the console | The package has no library for this platform: it holds only the macOS one (`Plugins/macos/ossia.dylib`). Or the package was added from a folder of the sources, which has no library. | Use a Mac; install the `.tgz` of the Release, not a folder. |
| `error CS0246: The type or namespace name 'Ossia' could not be found` | Your script is in an assembly definition (`.asmdef`) that does not reference `ossia`. | Inspector of the `.asmdef` > Assembly Definition References > `+` > `ossia`, Apply. |
| The script cannot be added to the GameObject, or its fields do not appear | The file name differs from the class name, or the project has compile errors (Play is refused). | Name the file after the class; fix the errors in the console. |
| `Controller GameObject not found` | No GameObject named exactly `OssiaController` in the scene. | Create it, with `Ossia.Controller`. |
| `OSSIA : ossia_protocol_multiplex_expose_to: Underlying Transport Error`; `curl` returns another program's answer | Port 5678 (or 1234) is held by another program. | `lsof -nP -iTCP:5678 -sTCP:LISTEN` and `lsof -nP -iUDP:1234` name it; quit it, or change `wsPort` and `oscPort` on the Controller. |
| `curl` returns a tree that is not the scene's, nothing in the console | Another ossia server on the same ports: a second Unity, a `CustomDevice`, an `ossia.device` exposed in Max on 5678. The last one started answers. | Give each its own ports. |
| Max connected by name finds `unity (2)`, or `connect unity` reaches another device | Another device of the network is already announced as `unity`. | Read the warning in the console; connect with `connect unity (2)`, or rename `deviceName`. |
| The scene follows the values only when Unity has the focus | `runInBackground` is off on the Controller, and the project's Run In Background (Project Settings > Player > Resolution and Presentation) is off: Unity stops `Update` when it loses focus, the values wait. | Turn `runInBackground` on (the default): the Controller then sets `Application.runInBackground` at runtime, without changing the project settings. |
| A value sent from Max does not move anything | The address is wrong (check with `curl`), or the script overwrites the field every frame. | Copy the `FULL_PATH` from `curl`; set the visible property from the field, not the other way round. |
| Max does not show what changes in Unity | Send Updates is off on `ExposeAttributes`. | Check it. |
| The cube of a sample is pink | The Exposed cube scene uses Unity's built-in default material, which the Universal and HD render pipelines cannot draw. | Cosmetic: the values still pass. Give the cube a material of your pipeline. |

## Tests

The package carries Edit Mode and Play Mode tests (`Tests/`). To run them, list the package in the `testables` of the project's `Packages/manifest.json`:

```
"testables": [ "io.ossia.ossia-unity" ]
```

then Window > General > Test Runner, or in batch mode: `Unity -batchmode -projectPath <project> -runTests -testPlatform EditMode -testResults results.xml`. The second Play and mDNS tests use ports 1234, 5678, 21234 and 25678, and `dns-sd` (macOS).

## Not tested

macOS Intel (the library is universal, its x86_64 part has not run); Windows, Linux, iOS (no library in the package); render pipeline HDRP; `PresetController` and `Logger`.
