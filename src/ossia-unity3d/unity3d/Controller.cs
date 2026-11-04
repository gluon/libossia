using UnityEngine;
using System.Runtime;
using System.Runtime.InteropServices;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Threading;
using Ossia;

namespace Ossia
{
  public class Controller : MonoBehaviour
  {
    public string appName = "Unity";

    [Tooltip ("Name of the OSCQuery device, the name it is announced under over mDNS")]
    public string deviceName = "unity";
    [Tooltip ("UDP port on which the device receives OSC")]
    public int oscPort = 1234;
    [Tooltip ("TCP port of the OSCQuery server: HTTP and WebSocket")]
    public int wsPort = 5678;
    [Tooltip ("Keep the player loop running when the application loses focus. "
            + "Off, Update stops and the values received are no longer applied until the window is focused again.")]
    public bool runInBackground = true;
    [Tooltip ("Read the name the device is announced under over mDNS, about two seconds after the start: "
            + "AnnouncedName, a console line, and a warning when it is not deviceName")]
    public bool resolveAnnouncedName = true;
    [Tooltip ("Send the values that change during a frame together, at the end of the frame, in OSC bundles. "
            + "Off, each value leaves as its own message as soon as it changes.")]
    public bool bundles = true;
    [Tooltip ("Forward a value a client sends to the other clients, never back to its sender, as ossia.device does in Max")]
    public bool forwardToClients = true;

    void Awake ()
    {
      Debug.Log ("OSSIA: Starting");
      // The values received are applied in Update, which Unity stops, in the
      // editor as in a player, when the application loses focus and the
      // project's Run In Background is off. Set at runtime only: the
      // project settings are left as they are.
      if (runInBackground)
        Application.runInBackground = true;
      if (!set) {
        set = true;
        Hold ();

        // Setup the log so that the errors in the C API are shown in the
        // Unity3D console.
        // Convert callback_delegate into a function pointer that can be
        // used in unmanaged code.
        IntPtr intptr_delegate =
          Marshal.GetFunctionPointerForDelegate (callback_delegate);

        // Call the API passing along the function pointer.
        Ossia.Network.ossia_set_debug_logger (intptr_delegate);

        local_protocol = new Ossia.Local ();
        local_device = new Ossia.Device (local_protocol, deviceName);
        scene_node = local_device.GetRootNode ();

        Queue = new Ossia.MessageQueue (local_device);

        // The server of ossia-max, which sends bundles, on a network
        // context that a thread of its own runs
        network_context = new Ossia.NetworkContext ();
        oscq_protocol = new Ossia.OSCQuery (network_context, oscPort, wsPort);
        local_protocol.ExposeTo (oscq_protocol);
        local_device.SetEcho (forwardToClients);
        network_context.Start ();

        // Once the server runs: its creation fails when a port is taken
        if (resolveAnnouncedName && oscq_protocol.ossia_protocol != IntPtr.Zero)
          LookUpAnnouncedName ();
      }
    }

    //! The name the device is announced under over mDNS (_oscjson._tcp),
    //! null until it is found, about two seconds after Awake. It differs from
    //! deviceName when another device of the network already had that name:
    //! the system then announces this one as "name (2)".
    public string AnnouncedName { get; private set; }

    // Long enough to see the announcement the server made in Awake, short
    // enough not to hold the next Play: in the editor, its domain reload
    // waits for the browsing thread, which cannot leave the library before.
    const int announcedNameTimeoutMs = 2000;

    // Browsing blocks: on a thread of its own, once, which hands the name
    // over to Update, on the main thread.
    void LookUpAnnouncedName ()
    {
      int port = wsPort;
      string self = System.Net.Dns.GetHostName ();
      var thread = new Thread (() => {
        found_name = Zeroconf.AnnouncedName (Zeroconf.ListOscqueryDevices (announcedNameTimeoutMs), self, port) ?? "";
      });
      thread.IsBackground = true;
      thread.Name = "ossia mdns lookup";
      thread.Start ();
    }

    void ReportAnnouncedName ()
    {
      var name = found_name;
      if (name == null || name_reported || quitting)
        return;
      name_reported = true;
      if (name == "") {
        Debug.LogWarning ("OSSIA: \"" + deviceName + "\" was not found among the devices announced over mDNS");
        return;
      }
      AnnouncedName = name;
      Debug.Log ("OSSIA: announced \"" + name + "\" over mDNS (_oscjson._tcp)");
      if (name != deviceName)
        Debug.LogWarning ("OSSIA: announced as \"" + name + "\": the name \"" + deviceName + "\" is taken on the network");
    }

    void Update()
    {
      ReportAnnouncedName ();
      Ossia.Message m;
      while (Queue.Pop (out m)) {
        if (Hash.ContainsKey (m.Address)) {
          Hash [m.Address].ReceiveUpdates (m.Value);
        }
      }
    }

    // After the Update of every object: what they published this frame
    void LateUpdate ()
    {
      Flush ();
    }

    //! What an exposed object sends: queued until the end of the frame and
    //! sent in one bundle, or sent at once when bundles is off.
    internal void Publish (Ossia.Parameter p, Ossia.Value v)
    {
      if (!bundles) {
        p.PushValue (v);
        return;
      }
      pending_parameters.Add (p);
      pending_values.Add (v);
    }

    void Flush ()
    {
      if (!set || pending_parameters.Count == 0)
        return;
      local_device.PushBundle (pending_parameters, pending_values);
      pending_parameters.Clear ();
      pending_values.Clear ();
    }

    public Ossia.Node SceneNode ()
    {
      return scene_node;
    }

    void OnApplicationQuit ()
    {
      Debug.Log ("OSSIA: Quitting");
      quitting = true;
      leaving = true;
      Release ();
    }

    // Destroyed without leaving Play: the library keeps a device by its name
    // until it is freed, and with it its server and its ports.
    void OnDestroy ()
    {
      Release ();
    }

    void Release ()
    {
      if (!set)
        return;
      set = false;
      // No network event while the device goes away
      network_context.Stop ();
      Hash.Clear ();
      pending_parameters.Clear ();
      pending_values.Clear ();
      Queue.Free ();
      // Clears the tree, then deletes the device, its protocols and their
      // servers
      local_device.Free ();
      network_context.Free ();
      local_device = null;
      local_protocol = null;
      oscq_protocol = null;
      network_context = null;
      scene_node = null;
      Let ();
    }

    // The devices of this package in the library: one per Controller until
    // it is released, one per Client from Connect to Disconnect.
    // ossia_device_reset_static deletes every device the library keeps,
    // the mirror a Client frees in its OnDisable included, and leaving Play
    // disables the objects after every OnApplicationQuit: the reset waits
    // for the last of them to free its own.
    internal static void Hold ()
    {
      live++;
    }

    internal static void Let ()
    {
      // Frees what remains in the library, devices made without a
      // Controller included, once Play stops
      if (--live == 0 && leaving)
        Network.ossia_device_reset_static ();
    }

    // With the domain reload off, the statics stay from one Play to the next
    [RuntimeInitializeOnLoadMethod (RuntimeInitializeLoadType.SubsystemRegistration)]
    static void Entering ()
    {
      leaving = false;
    }

    public Ossia.Device GetDevice ()
    {
      return local_device;
    }

    [AOT.MonoPInvokeCallback (typeof (DebugLogDelegate))]
    static void DebugLogCallback (string str)
    {
      Debug.Log ("OSSIA : " + str);
    }

    internal void Register(Ossia.OssiaEnabledField p)
    {
      Queue.Register (p.ossia_parameter);
      Hash.Add (p.ossia_parameter.ossia_parameter, p);
    }
    internal void Register(Ossia.OssiaEnabledProperty p)
    {
      Queue.Register (p.ossia_parameter);
      Hash.Add (p.ossia_parameter.ossia_parameter, p);
    }
    internal void Unregister(Ossia.OssiaEnabledField p)
    {
      Queue.Unregister (p.ossia_parameter);
      Hash.Remove (p.ossia_parameter.ossia_parameter);
    }
    internal void Unregister(Ossia.OssiaEnabledProperty p)
    {
      Queue.Unregister (p.ossia_parameter);
      Hash.Remove (p.ossia_parameter.ossia_parameter);
    }


    internal static Controller Get()
    {
      GameObject controller = GameObject.Find ("OssiaController");
      if (controller == null) {
        throw new Exception ("Controller GameObject not found");
      }
      var dev = controller.GetComponent<Ossia.Controller> ();
      if (dev == null) {
        throw new Exception ("Controller component not found");
      }
      return dev;
    }



    bool set = false;
    // Controllers and Clients whose device is not freed yet
    static int live = 0;
    // Play stops: set by OnApplicationQuit
    static bool leaving = false;
    volatile bool quitting = false;
    volatile string found_name = null;
    bool name_reported = false;

    Ossia.Local local_protocol = null;
    Ossia.Device local_device = null;
    Ossia.OSCQuery oscq_protocol = null;
    Ossia.NetworkContext network_context = null;
    readonly List<Ossia.Parameter> pending_parameters = new List<Ossia.Parameter> ();
    readonly List<Ossia.Value> pending_values = new List<Ossia.Value> ();

    Ossia.Node scene_node;
    Ossia.Network main;
    Ossia.MessageQueue Queue;
    Dictionary<IntPtr, Ossia.OssiaEnabledElement> Hash = new Dictionary<IntPtr, Ossia.OssiaEnabledElement>();

    public delegate void DebugLogDelegate (string str);

    // Static, so that IL2CPP can call it and the GC never collects it
    static readonly DebugLogDelegate callback_delegate = DebugLogCallback;
  }
}
