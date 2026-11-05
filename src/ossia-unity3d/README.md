# ossia (PRISM) for Unity

The Unity package `io.ossia.ossia-unity`, displayed as "ossia (PRISM)": it exposes the objects of a Unity scene, and the fields of your own scripts, as an OSCQuery device that Max (package `ossia-prism`), TouchDesigner or any OSCQuery client can read and drive; and it follows a remote OSCQuery device, a Max patch or a TouchDesigner project, with `Ossia.Client` and `Ossia.Remote` (see [Following a device](#following-a-device)). It is built from this repository, with the ossia library and its C# binding; its version is the tag it is built from, `2.1.0+prism.N`.

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
| Follow a device | The scene `FollowADevice`: an `OssiaClient` that follows the device `follow_me` by its name, and a cube whose four `Ossia.Remote` bind the fields of `FollowADevice.cs` to `/cube/size`, `/cube/color`, `/cube/spin` and `/cube/angle`. The Max patch `FollowADevice.maxpat` exposes `follow_me` as it opens: Max sets the size, the color and the spin, Unity sends the angle back. See [Following a device](#following-a-device). |
| Exposed cube | The scene `ExposedCube`: an `OssiaController` and a cube whose transform is exposed (`/Cube/transform/position`, `rotation`, `localScale`), with no script of its own. Its `ExposeAttributes` keeps Send Updates unchecked, as the scene saved it: the cube follows the clients and sends nothing back. |

"Custom parameters" and "Exposed cube" run on ports 1234 (OSC) and 5678 (HTTP and WebSocket), under the device name `unity`; "Follow a device" has no server, and its patch exposes `follow_me` on 9234 and 9678.

## Declaring your own parameters

Three things in the scene:

1. A GameObject named exactly `OssiaController`, with the component `Ossia.Controller` (Add Component > search "Controller"). One per scene; the exposed objects find it by that name.
2. Your script, on any GameObject, with the fields to expose marked `[Ossia.Expose("<name>")]`.
3. The component `Ossia.ExposeAttributes` on that same GameObject. It reads the marked fields when the scene starts and publishes each one under `/<GameObject name>/<script class>/<name>`. It also always publishes the GameObject's transform, under `/<GameObject name>/transform/`: `position`, `rotation` and `localScale`.

The script of the sample, `CustomParameters.cs`, copied as is into `Assets` (the file name must be the class name), on a cube named `Shape`. Do this in a project without the imported sample, or rename the class and the file: two classes `CustomParameters` stop the project compiling (`error CS0101`).

```csharp
using UnityEngine;
using UnityEngine.Rendering;

// Five fields of a script, exposed over OSCQuery. The ExposeAttributes
// component of the same GameObject finds the fields marked [Ossia.Expose]
// and publishes each one under /<GameObject name>/<script class>/<name>,
// here /Shape/CustomParameters/speed, /Shape/CustomParameters/axis, ...
// A value sent by a client changes the field; with Send Updates on (the
// default) on ExposeAttributes, a change made in Unity is sent to the clients.
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

  // Taking control: when a client writes the orientation of the shape
  // (/Shape/transform/rotation), the spin pauses this long, so that the
  // client's value is not turned away at the next frame. Not exposed.
  public float holdAfterClient = 1f;

  Material material;
  float pausedUntil;

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

    // Received: a value of a client applied, never what Unity publishes
    GetComponent<Ossia.ExposeAttributes>().Received += (name, value) => {
      if (name == "rotation")
        pausedUntil = Time.time + holdAfterClient;
    };
  }

  void Update()
  {
    if (spin && Time.time >= pausedUntil) {
      var a = axis == 0 ? Vector3.right : axis == 2 ? Vector3.forward : Vector3.up;
      transform.Rotate(a, speed * Time.deltaTime);
    }
    transform.localScale = size;
    material.color = color;
  }
}
```

The `Start` method: the render pipeline part only matters for a cube made by the sample's scene file; a cube created in your own project with GameObject > 3D Object > Cube already has the material of its render pipeline, and `material = GetComponent<Renderer>().material;` is enough. `Received` and `holdAfterClient` are taking control: leave them out if the spin may go on while a client writes the rotation.

### What a client sees

Play, then `curl -s http://localhost:5678/Shape/CustomParameters` (output of the sample with `2.1.0+prism.10`):

```
{"FULL_PATH":"/Shape/CustomParameters","CONTENTS":{"speed":{"FULL_PATH":"/Shape/CustomParameters/speed","TYPE":"f","VALUE":45.0,"ACCESS":3,"CLIPMODE":"none"},"axis":{"FULL_PATH":"/Shape/CustomParameters/axis","TYPE":"i","VALUE":1,"ACCESS":3,"CLIPMODE":"none"},"spin":{"FULL_PATH":"/Shape/CustomParameters/spin","TYPE":"T","VALUE":true,"ACCESS":3,"CLIPMODE":"none"},"color":{"FULL_PATH":"/Shape/CustomParameters/color","TYPE":"ffff","VALUE":[1.0,0.5,0.0,1.0],"ACCESS":3,"CLIPMODE":"none","EXTENDED_TYPE":["color.rgb.r","color.rgb.g","color.rgb.b","color.rgb.a"]},"size":{"FULL_PATH":"/Shape/CustomParameters/size","TYPE":"fff","VALUE":[1.0,1.0,1.0],"ACCESS":3,"CLIPMODE":"none","EXTENDED_TYPE":"vecf"}}}
```

Pipe it into `python3 -m json.tool` to read it indented. In short:

| Address | `TYPE` | `VALUE` at start | `EXTENDED_TYPE` |
|---|---|---|---|
| `/Shape/CustomParameters/speed` | `f` | `45.0` | |
| `/Shape/CustomParameters/axis` | `i` | `1` | |
| `/Shape/CustomParameters/spin` | `T` | `true` | |
| `/Shape/CustomParameters/color` | `ffff` | `[1.0, 0.5, 0.0, 1.0]` | `color.rgb.r` ... `color.rgb.a` |
| `/Shape/CustomParameters/size` | `fff` | `[1.0, 1.0, 1.0]` | `vecf` |

`ACCESS` is 3 (read and write) for every parameter, `CLIPMODE` is `none`, and there is no `RANGE`: the script gives none (see below for `Min`, `Max`, `Unit` and `Access`). One value: `curl -s "http://localhost:5678/Shape/CustomParameters/speed?VALUE"` returns `{"VALUE":45.0}`.

### Values in both directions

- **Client to Unity, always.** A value sent by a client (an `ossia.remote` in Max, an OSC message to UDP port 1234, a WebSocket message) changes the field, in the `Update` of the Controller, on the main thread, at the next frame. Measured on the sample: after `/Shape/CustomParameters/speed 90.` (`,f`), `axis 0` (`,i`), `spin` false (`,F`), `color 0. 1. 0. 1.` and `size 2. 1. 0.5`, the fields read `speed=90 axis=0 spin=False color=RGBA(0, 1, 0, 1) size=(2, 1, 0.5)`, and the cube's scale is `(2, 1, 0.5)`.
- **Unity to client, with Send Updates** on `ExposeAttributes` (field `sendUpdates`, on by default; a scene keeps the value it saved, unchecked in the sample "Exposed cube"). Each frame, a field whose value changed, whatever changed it (the Inspector, a script, an animation), is pushed to the clients that listen to it (an `ossia.remote` shows it on its outlet). Measured on the sample: after setting in Unity `speed=12.5 axis=2 color=(0, 0, 1, 1) size=(0.5, 0.5, 0.5)`, `?VALUE` returns `12.5`, `2`, `[0.0,0.0,1.0,1.0]` and `[0.5,0.5,0.5]`. With Send Updates unchecked, `speed` set to 7 in Unity stays `12.5` for the client.

- **The changes of a frame leave together.** What the exposed objects change during a frame leaves at its end, after every `Update`, in OSC bundles (`bundles` on the Controller, on by default): measured, eight fields changed in one frame reach a client in one datagram of eight messages; with `bundles` off, in eight datagrams (`Tests/Runtime/ServerBundleTests.cs`). A bundle is split when it would exceed a datagram (1400 bytes).
- **Its own values are not applied again.** What an exposed object publishes comes back through the Controller's message queue, and is not applied: when the object publishes before the Controller reads its queue in the frame, the value of the frame before comes back after the one of this frame left, and both are recognised. Measured, a script that turns an object while a client sets its speed, the object publishing first: in `2.1.0+prism.9`, the Controller set the angle back in 60 frames of 120, and the object turned at half its speed; now in none (`Tests/Runtime/TurningObjectTests.cs`).
- **No echo.** A value received from a client and applied to the field does not leave again from Unity. Measured with a libossia OSCQuery client (the core of `ossia.remote`) writing 20 values, 150 ms apart, away from any change in Unity: the field took the last, and none came back to the client (`Tests/Runtime/ConcurrentWriteTests.cs`).
- **Crossed writes.** When Unity and a client write the same address in the same instant, each value crosses the other: Unity's leaves before the client's arrives, and the client applies Unity's after sending its own. The server then answers the client with its final value, the client's, and both end on it. Measured, a script and a libossia client, 20 crossings: 0 divergence (`Tests/Runtime/ForwardTests.cs`, `CrossedWritesConverge`); without the answer, every crossing diverged. A write is taken for a crossing when it comes less than 100 ms after the server sent the client a different value on that address; the answer goes to that client only, and opens no window. When a script and a client write in turns every frame for 5 s (297 and 296 writes), every write of the client follows one of Unity's within that window: each is answered with its own value (296 answers), both end on the last value written, and nothing moves once both stop.
- **Clients see each other.** The server forwards a value one client sends to the other clients, never back to its sender, as `ossia.device` does in Max (`forwardToClients` on the Controller, on by default). Measured with two libossia clients A and B: A writes 0.25, Unity applies it, B receives it once, A receives nothing; B writes 0.5 and Unity then sets 0.75, A receives 0.5 then 0.75, B only 0.75. With `forwardToClients` off, B receives nothing of what A writes (`Tests/Runtime/ForwardTests.cs`).

The initial values of the fields are published when the scene starts, whatever Send Updates says. When a script sets a field every frame (as the sample does with `transform.localScale = size`), what a client sends to that field is overwritten at the next frame.

### Accepted types

What `[Ossia.Expose]` accepts, with what a client sees (measured):

| C# type | `TYPE` | `EXTENDED_TYPE` (unit) | Note |
|---|---|---|---|
| `float`, `double` | `f` | | `double` is sent as a 32-bit float |
| `int` | `i` | | |
| `bool` | `T` | | `VALUE` reads `true` or `false` over HTTP |
| `string` | `s` | | |
| `Vector2`, `Vector3`, `Vector4` | `ff`, `fff`, `ffff` | `vecf` | no unit of its own; give one with `Unit` |
| `Quaternion` | `ffff` | `orientation.quaternion` | x, y, z, w |
| `Color` | `ffff` | `color.rgb.r` ... `color.rgb.a` | r, g, b, a from 0 to 1 |
| `Color32` | `r` | | sent as an OSC color, `#RRGGBBAA` over HTTP |
| `float[]`, `int[]` | `ff...`, `ii...` | `list` | the length is that of the array at start |

A field of another type (`long`, `Texture2D`, an enum, a class) is skipped without a message: it does not appear in the tree.

What is read: the public fields declared in the script's own class, and its public properties (inherited ones included). A property needs a `get` to be published and a `set` to receive values. Not read: private fields. Public fields declared in a parent class are read since `2.1.0+prism.10`.

### Limits

- The server cannot tell a write that crossed its value from a write the client made right after receiving it: a client that writes less than 100 ms after a change in Unity gets its own value back, once, unchanged. Writes further apart get nothing.
- `[Ossia.Expose]` takes a name, `Min`, `Max`, `Unit` and `Access` (`2.1.0+prism.10`), for instance `[Ossia.Expose ("speed", Min = 0, Max = 360, Unit = "angle.degree", Access = Ossia.ossia_access_mode.GET)]`; without a name, the parameter takes the name of the field or property. No clip mode and no description: the bounds are published in `RANGE` and not enforced, so clamp the received values in your script, as the sample does for `axis`. A vector has the same bounds on each component.
- The node of a script is its full class name: a class `Params` in the namespace `MyGame` gives `/<GameObject>/MyGame.Params/...`.
- The GameObject's name is read once, when the scene starts: renaming it in Play does not move its parameters.

### ExposeComponents

`Ossia.ExposeComponents` publishes every public field and property of the components put in its list, with no attribute to write. Example, a `Light` on a GameObject named `Lamp`: add `ExposeComponents`, set its Node Name to `Lamp`, drag the `Light` component into its Components list, Play. Measured:

```
curl -s "http://localhost:5678/Lamp/Light/intensity?VALUE"
{"VALUE":1.0}
```

The tree holds `/Lamp/Light/color`, `/Lamp/Light/intensity`, `/Lamp/Light/range`, `/Lamp/Light/spotAngle`, `/Lamp/Light/enabled` and some forty other properties of `Light`. An OSC message `/Lamp/Light/intensity 3.5` sets the light's intensity to 3.5. The node of a component is its class name without namespace (`Light`), under Node Name (default `Object`). Its fields: `NodeName` (`Object`), `components` (empty), `sendUpdates` (on).

### Low-level API: CustomDevice.cs

`Scripts/unity3d/examples/CustomDevice.cs`, in the package, builds a device with the C# API alone (`Ossia.Device`, `Ossia.Node.CreateNode`, `CreateParameter`, `PushValue`), with no Controller: it is the way to build a tree that does not follow the scene. It creates its own device, `newDevice`, on ports 1234 (OSC) and 5678 (WebSocket), the ports of the Controller.

**Do not put it in a scene that has an `OssiaController` with the default ports.** Nothing is reported: both servers start, and a client reaching port 5678 gets the tree of `CustomDevice` (`/foo/blu`, `/my_string`, ...) instead of the scene's. Change the ports in `CustomDevice.cs` (copy it into `Assets` under another class name) or on the Controller.

It leaves `newDevice` to the reset of the library (`ossia_device_reset_static`), which frees its ports when Play stops, through the count the Controllers and the Clients share: `Ossia.Controller.Hold()` once its device is made, `Ossia.Controller.Let(true)` in its `OnApplicationQuit`. The reset waits for the last of them, so that a Client of the same scene frees its mirror first. A script of your own that builds a device does the same, or frees its device itself. Measured in the editor, `CustomDevice` and a Client that follows it, Play then Stop twice: no crash, the ports free after each Stop (`Tests/Editor/PlayCycleTests.cs`); with an unconditional reset, Unity crashed at the first Stop.

## Driving from Max

With the package `ossia-prism` in Max, the sample's `CustomParameters.maxpat`:

- `[ossia.client]`, without argument, message `connect unity`: connects by name (mDNS); the console shows `client: connect 1 unity` once the device is found, `client: connect 0 unity` every second until then. Created with a name, `[ossia.client <name>]` takes that name and connects nothing (see the README of ossia-prism, Discovery by name). Or `connect oscquery ws://127.0.0.1:5678`, by address.
- one `[ossia.remote /Shape/CustomParameters/<name>]` per field: a number box for `speed`, a number box from 0 to 2 for `axis`, a toggle for `spin`, four number boxes into `[pak 1. 0.5 0. 1.]` for `color`, three into `[pak 1. 1. 1.]` for `size`;
- right of each remote, a message box shows what Unity sends back (Send Updates is checked in the sample's scene).

For your own script: one `[ossia.remote /<GameObject>/<class>/<name>]` per field, the address read with `curl` as above. Connected by name, the client follows Stop and Play: `disconnected` once the device is no longer announced, `connect 1 unity` when it is back.

## Following a device

`Ossia.Client` follows a remote OSCQuery device, as `ossia.client` does in Max: an `ossia.device` exposed in Max, a TouchDesigner project, another Unity. `Ossia.Remote` binds a field or a property of one of your scripts to an address of that device, both ways, as `ossia.remote` does. A scene can have a Client without a Controller, and both.

### The sample

1. In the Package Manager, Samples tab, Import "Follow a device".
2. In Max, with the package `ossia-prism`, open `FollowADevice.maxpat` from the sample's folder: it exposes `follow_me` as it opens, on ports 9234 (OSC) and 9678 (WebSocket).
3. In Unity, open the scene `FollowADevice` and press Play. The console shows `OSSIA client: connected to ws://<machine>.local:9678 (follow_me)`; the Inspector of `OssiaClient` reads `Connected to ws://...`.
4. In Max, change size, color and spin: the cube follows. The number under angle follows the cube as it turns.

Measured with Max 9 and ossia-prism `2.1.0+prism.10`, in a new project, these steps followed with no click (`tests/Unity/cross/doc_follow.py`, see [Tests](#tests)): the console line `OSSIA client: connected to ws://<machine>.local:9678 (follow_me)`, the Inspector `Connected to ws://<machine>.local:9678`; after `size 2.` and `spin 180.` in Max, the cube's scale reads `(2.00, 2.00, 2.00)` and its spin 180; the angle Max receives moves.

### Your own script

A script whose field follows `/level` of a device, and sends it back when Unity changes it:

```csharp
using UnityEngine;

public class FollowLevel : MonoBehaviour
{
  public float level;   // follows /level

  void Update ()
  {
    transform.localScale = Vector3.one * (0.5f + level);
  }
}
```

1. Save it as `Assets/FollowLevel.cs` (the file name is the class name).
2. Create an empty GameObject, add `Ossia.Client`, set Server to the device's name (`mydevice`) or its address (`ws://127.0.0.1:9678`).
3. On the object to move, add `FollowLevel`, then `Ossia.Remote`: Client, the GameObject of step 2 (or empty: the first Client of the scene); Address `/level`; Target, the `FollowLevel` component (drag it from the same Inspector); Member `level`.
4. In Max: `[ossia.device mydevice]`, the message `expose oscquery 9234 9678` into it, and `[ossia.parameter level @type float]` fed by a number box.
5. Play. The object's size follows the number box; set `level` in the Inspector, Max's `ossia.parameter` outputs it.

From a script, the same binding:

```csharp
var remote = gameObject.AddComponent<Ossia.Remote> ();
remote.address = "/level";
remote.target = GetComponent<FollowLevel> ();
remote.member = "level";
```

The Remote binds at the next frame, once the device's tree holds the address.

### What happens

- **Connection.** By address, `ws://host:port`; by name, the name the device is announced under over mDNS (`_oscjson._tcp`), looked up for `browseMs` (2 s), then reached at the host and port of its announcement. The connection runs on a thread of its own: the frames go on meanwhile, and a server of the same Unity can answer. Status reads `Connecting`, then `Connected`.
- **Values from the device.** Read with the tree when the Client connects, then each value the device sends, applied to the field in the Client's `Update`, on the main thread. The Remote listens to its address from its binding on.
- **Values to the device.** Every change of the field, whatever made it (a script, the Inspector, an animation), leaves at the end of the frame, after every `Update`; the changes of one frame leave in one OSC bundle (`bundles`, on by default). Measured: eight fields changed in one frame reach the server in one datagram of eight messages; with `bundles` off, in eight.
- **No echo.** A value received is applied and does not leave again. Measured, the server and a script writing `/level` in turns for 5 s: 297 writes by the server, 296 by the script; the server received 297 messages, none of its own values; both ended on the last value written; nothing moved once both stopped.
- **Tree changes.** Nodes the device adds after the connection are applied each frame; a Remote whose address did not exist binds when it appears, and takes its value. Measured: `/later`, created on the server half a second after the connection, bound and applied to the field.
- **Loss and reconnection.** When the server stops, the Client reads `Lost`, the console warns `OSSIA client: lost ws://...`, the Remotes unbind, and with `reconnect` on (the default) it tries again every second until the server is back, then binds the Remotes again and applies the device's values. Measured: lost within 3 s of the stop, connected again within a few seconds of the return, and the field takes the value of the new server.

All of this is measured by `Tests/Runtime/ClientTests.cs`, against a libossia server built in the test as `ossia.device` builds its device, and by the cross bench against Max (`ossia.device`) and TouchDesigner (`prism_oscquery_server` 0.3.3), by address and by name: values both ways, nothing back for 20 values written on either side, 0 divergence in 20 crossed writes (see [Tests](#tests)).

### Limits

- A server that does not answer crossed writes (ossia-max `2.1.0+prism.8` and earlier, ossia 2.1.0, a WebSocket++ server of the C API): when the scene and the server write the same address in the same instant, each keeps the other's value. Measured with a libossia client and such a server, 20 crossings: 20 divergences. Since `2.1.0+prism.10`, the client of the core reads the value again 100 ms after a crossing, and both end on the server's value (`C API: a mirror reads the value again after a crossing`); not measured from a scene. The servers of ossia-prism since `2.1.0+prism.9` (ossia-max, the Unity Controller) and of TouchDesigner (`prism_oscquery_server` since 0.3.2) answer them.
- One address per Remote, and one field or property per Remote: a script with five followed fields needs five Remotes.
- The types are those of [Accepted types](#accepted-types); a value whose type the field cannot take (a `float` for a `Vector3` field) is ignored without a message, and the field keeps its value.
- A node the device removes stays in the Client's tree, marked as removed (the default of the libossia mirror; ossia-max removes it): a Remote bound to it stays bound and receives nothing more. Not tested.
- A connection by name reads the announcement once per attempt: a device renamed while connected is followed at its old address until it is lost.
- `ossia.client` and `ossia.remote` features not offered: patterns (`/shape.*/size`), `@unit` conversion, `@clip`, `@mode`.


### Ossia.Controller

On the GameObject named `OssiaController`.

| Field | Default | Role |
|---|---|---|
| `deviceName` | `unity` | Name of the device, under which it is announced over mDNS (`_oscjson._tcp`) |
| `oscPort` | `1234` | UDP port on which the device receives OSC |
| `wsPort` | `5678` | TCP port of the OSCQuery server: HTTP (the tree) and WebSocket |
| `listenAddress` | empty | The address both ports listen on: `127.0.0.1` for the clients of this machine only, the address of one interface for that network only; empty, every interface. On one address the device is not announced over mDNS: clients connect with `ws://<address>:<wsPort>` |
| `runInBackground` | on | Keeps the scene following the values received when Unity loses focus (see [Troubleshooting](#troubleshooting)) |
| `resolveAnnouncedName` | on | Reads the name the device is announced under over mDNS (see [mDNS](#mdns)) |
| `bundles` | on | Sends what changes during a frame at its end, in OSC bundles; off, each value at once in its own message |
| `forwardToClients` | on | Forwards a value a client sends to the other clients, never back to its sender |
| `appName` | `Unity` | Name used by `PresetController` and `Logger` |
| `AnnouncedName` (read only) | null | The name the device is announced under, once found |

The device starts in `Awake` and is freed when the Controller is destroyed or Play stops (`OnDestroy`, `OnApplicationQuit`), which frees its ports and withdraws its mDNS announcement: a Controller created again on the same ports, in the same Play or the next one, serves its own tree. An exposed object whose Controller is destroyed stops publishing. The values received are applied in the Controller's `Update`. The server is that of ossia-max, on a network context a thread of the Controller runs (`ossia network`): it answers the clients and receives their values while the frames run, and the values wait in a queue for the main thread.

### Ossia.Client

| Field | Default | Role |
|---|---|---|
| `server` | `ws://127.0.0.1:5678` | The device to follow: the name it is announced under over mDNS, or `ws://host:port` |
| `reconnect` | on | Tries again every second when the server goes away, until it comes back |
| `bundles` | on | Sends what the Remotes change during a frame at its end, in OSC bundles |
| `browseMs` | `2000` | How long a connection by name looks for the device over mDNS, in milliseconds |
| `status` (shown, read only) | `Disconnected` | Where the connection stands, with the address or the error |
| `Status` (property) | | `Disconnected`, `Connecting`, `Connected` or `Lost` |
| `Url`, `LastError`, `Device` (properties) | | The address while connected, why the last attempt failed, the mirror of the device |

Connects in `OnEnable`, disconnects in `OnDisable`; `Connect()` and `Disconnect()` do the same from a script. Leaving Play disables the Client after the `OnApplicationQuit` of the Controllers: the Client frees its mirror then, and the library is reset (`ossia_device_reset_static`) once the last Controller or Client has freed its device. Measured in the editor, a Controller, a Client that follows it and a Remote bound to the Controller's tree, Play then Stop twice: no crash (`Tests/Editor/PlayCycleTests.cs`); with the package `2.1.0+prism.9`, Unity crashed at the first Stop.

### Ossia.Remote

| Field | Default | Role |
|---|---|---|
| `client` | empty | The Client that follows the device; empty, the first Client of the scene |
| `address` | `/` | The address on the device, `/level`, `/cube/color` |
| `target` | empty | The component whose field or property follows the address |
| `member` | empty | The name of its public field or property |
| `sendUpdates` | on | Sends the changes of the field to the device; off, the field only follows it |
| `Bound` (property) | | True once the address was found on the device |
| `Received` (event) | | Raised on the main thread with each value received from the device, after the field is set |

### Ossia.ExposeAttributes

| Field | Default | Role |
|---|---|---|
| `sendUpdates` | on | Pushes to the clients, each frame, the fields and transform values changed in Unity, whatever changed them; never a value received from a client. A scene keeps the value it saved |
| `Received` (event) | | Raised on the main thread with the name and the value of each parameter a client sets, after the field is set: a script can hold its own changes for a while, as the sample "Custom parameters" pauses its spin while a client sets the rotation |

Publishes the transform (`position`, `rotation`, `localScale`) and the fields and properties marked `[Ossia.Expose]` of every component of its GameObject. The other properties of the transform (`localPosition`, `eulerAngles`, `localEulerAngles`, `localRotation`, `right`, `up`, `forward`, `lossyScale`) say again what these three say, and are not published: set back, they disagreed with them, `up` or `right` holding a part of the orientation only. Measured, an object a script turns at `Quaternion.Euler(20, angle, 0)`: `right` written back as read gave it `(0, 202.5, 0)` for `(20, 202.5, 0)` (`Tests/Runtime/TurningObjectTests.cs`).

### Ossia.Expose

`[Ossia.Expose("<name>")]` on a public field or property: the name of the parameter under the script's node; without a name, the name of the field or property. Its options: `Min` and `Max` (published in `RANGE`, the same for each component of a vector, not enforced), `Unit` (an ossia unit, `angle.degree`, `gain.db`, `position.cart3D`; by default the unit the type gives) and `Access` (`ossia_access_mode.GET`, `SET` or `BI`, the default). See [Accepted types](#accepted-types) and [Limits](#limits).

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
| Max does not show what changes in Unity | Send Updates is unchecked on `ExposeAttributes` (on by default; a scene keeps the value it saved, as "Exposed cube" does). | Check it. |
| `OSSIA client: "mydevice" is not announced over mDNS` | No device of that name on the network, or mDNS does not pass (see [mDNS](#mdns)). | `dns-sd -B _oscjson._tcp` lists the names; set Server to that name, or to `ws://host:port`. |
| `OSSIA client: ws://...: the tree of ws://... could not be read` (or `no OSCQuery server answers at ws://...`) | The server is not started, or another port. | Start it (`expose oscquery <osc> <ws>` in Max), check the port with `curl -s http://host:port`. |
| `OSSIA remote /x: Y has no public field or property "z"` | Member misspelt, or the field is private. | Make the field public, copy its exact name. |
| A Remote stays unbound, nothing moves | The address is not in the device's tree. | `curl -s http://host:port` shows the tree; copy the `FULL_PATH`. |
| The cube of a sample is pink | The Exposed cube scene uses Unity's built-in default material, which the Universal and HD render pipelines cannot draw. | Cosmetic: the values still pass. Give the cube a material of your pipeline. |

## Tests

The package carries Edit Mode and Play Mode tests (`Tests/`). To run them, list the package in the `testables` of the project's `Packages/manifest.json`:

```
"testables": [ "io.ossia.ossia-unity" ]
```

then Window > General > Test Runner, or in batch mode: `Unity -batchmode -projectPath <project> -runTests -testPlatform EditMode -testResults results.xml`. The tests use ports 1234 and 5678 (second Play), 11234 and 15678, 21234 to 21237 and 25678 to 25681, 27234 and 27678, 28234 and 28678, 29234 to 29238 and 29678 to 29682 (the clients), 30234 and 30678, 32234 and 32678, 41234, 41235, 45678 and 45679 (mDNS), UDP 10203 or the next free ports (the libossia clients of the tests), UDP ports the system chooses (the tests that count datagrams), the mDNS name `ossia_unity_client_test_<process>`, and `dns-sd` (macOS).

### With Max and TouchDesigner

The cross bench, in the repository (`tests/Unity/cross/`), runs the package against Max and TouchDesigner with no click, on one machine. One command:

```
python3 tests/Unity/cross/cross_run.py <run folder> --package <io.ossia.ossia-unity-*.tgz> \
  --td-project <TouchDesigner project folder> --td-tests <prism-td-components>/oscquery/tests
```

The run folder is outside the repository: a Unity project with the package, the probe and the sample "Follow a device", and its macOS player, built again when the package or a probe file is newer, go there. The TouchDesigner project folder is that of the Installation of prism-td-components (`prism_oscquery.tox`, `prism_oscquery_server.tox`, `.venv/`, `TDPyEnvManagerContext.yaml`); `docs_run.py` of its tests writes the TouchDesigner project that runs `cross_td.py`. Max 9 with ossia-prism must be installed and restarted after a new install of the package (Max keeps the library it loaded); TouchDesigner must not be running. The script opens `cross.maxpat` (with `cross_peer.js`) and the sample's `FollowADevice.maxpat` in Max, starts TouchDesigner and two players, and drives them over text and OSC datagrams on 127.0.0.1 (ports 7493 to 7498).

Pairs: Unity serves `unity_cross` (9870, 9871), followed by Max (`ossia.client`), TouchDesigner (`prism_oscquery`) and a second Unity (`Ossia.Client`); Unity follows Max (`ossia.device max_cross`, 9880, 9881) and TouchDesigner (`prism_oscquery_server td_cross`, 9890, 9891); each by address, then by name. In each pair: a value written on each side reaches the other; 20 values written on one side, 50 ms apart, away from any change of the other, all applied and none back, on each side; 20 crossed writes, both sides writing a different value in the same instant, both ending on the same value. Then the scene of "Follow a device" follows `follow_me` of `FollowADevice.maxpat`, by name and by address, Max writing `cube/size` as another client and reading `cube/angle`. The report merges the logs of the runner, Max, TouchDesigner and the players in time order and ends with `RESULT: PASS` or `RESULT: FAIL`; it is written to `<run folder>/cross_run/report.txt`.

On 2026-11-05, Max 9.2, the sources of ossia-prism `2.1.0+prism.10`, TouchDesigner 2025.33230 with `prism_oscquery` 0.3.3, Unity 6000.3.27f2: `RESULT: PASS`, 73 checks, none failed. On 2026-11-03, with `2.1.0+prism.9` and 0.3.2, the same. Nothing came back in any of the 20-value bursts; 0 divergence in the 100 crossed writes of the five pairs. With the library of ossia-prism `2.1.0+prism.8` still loaded in Max, `ossia.device` sent back each of the 20 values Unity wrote. The crossed writes of the pair "Max server x Unity client" did not diverge with it either: on one machine, the value of Max seldom reaches Unity between the frame where Unity changes the field and the end of that frame, when Unity sends it; the answer of the server to a crossing is proven by the tests of the C API and the Play Mode test `CrossedWritesConverge`, red without it.

The "doc only" test of "Follow a device": `python3 tests/Unity/cross/doc_follow.py <folder> --package <tgz>` creates a new project, adds the package and imports the sample as the Package Manager does (by their effect on the files), opens `FollowADevice.maxpat` in Max, and plays the scene in batch mode: `RESULT: PASS` on 2026-11-03 (see [The sample](#the-sample)).

## Functional audit

What the package does for each requirement of the PRISM platform, and what proves it, for `2.1.0+prism.10`. The requirements: values travel in OSC bundles, and only those that change (12); both directions, without echo (13): a value received is applied (a), every local change leaves whatever its source (b), a value received and applied does not leave again, and two sides writing at once agree (c).

| Requirement | Server (`Controller`, `ExposeAttributes`) | Client (`Client`, `Remote`) | Proven by |
|---|---|---|---|
| 12, bundles | what changes during a frame leaves at its end, in bundles of at most 1400 bytes | the same (`bundles` on the Client) | `Tests/Runtime/ServerBundleTests.cs`, `ClientTests.cs` |
| 12, only what changes | a field leaves when its value changed | the same | `Tests/Runtime/SendUpdatesTests.cs`, `ClientTests.cs` |
| 13a, received values applied | at the next frame, on the main thread | the same | `Tests/Runtime/SendUpdatesTests.cs`, `ClientTests.cs`; `tests/Unity/cross/cross_run.py` |
| 13b, every source leaves | read each frame: a script, the Inspector, an animation | the same | `Tests/Runtime/SendUpdatesTests.cs`, `ClientTests.cs`; `cross_run.py`; the bench of the TouchDesigner component (`bidir_run.py`), Unity (a) and (b) |
| 13c, no echo | a value applied does not leave again; its own values are not applied again | the same | `Tests/Runtime/ConcurrentWriteTests.cs`, `TurningObjectTests.cs`; `cross_run.py` (0 of 20 back on each side); `bidir_run.py` (0 of 50) |
| 13c, two writers agree | a write that crosses a value sent less than 100 ms before is answered with the final value | reads the value again 100 ms after a crossing | `Tests/Runtime/ForwardTests.cs`, `CrossedWritesConverge`; `cross_run.py` (0 divergence out of 100 crossings) |
| Discovery by name | announces `_oscjson._tcp`; `AnnouncedName` says the name it got | `server` takes a name | `Tests/Editor/MdnsTests.cs`, `Tests/Runtime/AnnouncedNameTests.cs`; `cross_run.py` (by name) |
| Reconnection | a client that comes back is served; Stop then Play serves again | tries again every second with `reconnect` | `Tests/Runtime/ClientTests.cs`, `Tests/Runtime/ControllerLifetimeTests.cs` |
| Leaving Play | frees its ports, with or without a Client in the scene | the same | `Tests/Editor/PlayCycleTests.cs` |
| Taking control | the event `ExposedObject.Received`: a script holds its own changes for a while | the event `Remote.Received` | `Tests/Runtime/ExposeOptionsTests.cs`, `ClientTests.cs`; the sample "Custom parameters" |
| Where it listens | `Controller.listenAddress`; on one address, not announced | no choice of interface | `Tests/Runtime/ListenAddressTests.cs` |
| Who follows (map) | `?PRISM_CONNECTIONS`, the last 256 UDP senders | sends `PRISM_NAME` and what it listens to | `C API` tests of `tests/C/unit-tests/unit-tests-c-network.cpp` |

What it does not do, also in the limits: a field does not hold network values during a gesture by itself: a script does it with `Received`; a client does not choose the interface it follows from; a node the device removes stays in the Client's tree, marked as removed; `ossia.client` and `ossia.remote` features not offered (patterns, unit conversion, `@clip`, `@mode`).

## Not tested

macOS Intel (the library is universal, its x86_64 part has not run); Windows, Linux, iOS (no library in the package); render pipeline HDRP; `PresetController` and `Logger`; Max, TouchDesigner and Unity on different machines.
