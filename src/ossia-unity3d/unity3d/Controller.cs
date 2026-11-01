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

        oscq_protocol = new Ossia.OSCQuery (oscPort, wsPort);
        local_protocol.ExposeTo (oscq_protocol);

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
          Hash [m.Address].ReceiveUpdates ();
        }
      }
    }

    public Ossia.Node SceneNode ()
    {
      return scene_node;
    }

    void OnApplicationQuit ()
    {
      Debug.Log ("OSSIA: Quitting");
      quitting = true;
      Network.ossia_device_reset_static ();
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
    volatile bool quitting = false;
    volatile string found_name = null;
    bool name_reported = false;

    Ossia.Local local_protocol = null;
    Ossia.Device local_device = null;
    Ossia.OSCQuery oscq_protocol = null;

    Ossia.Node scene_node;
    Ossia.Network main;
    Ossia.MessageQueue Queue;
    Dictionary<IntPtr, Ossia.OssiaEnabledElement> Hash = new Dictionary<IntPtr, Ossia.OssiaEnabledElement>();

    public delegate void DebugLogDelegate (string str);

    // Static, so that IL2CPP can call it and the GC never collects it
    static readonly DebugLogDelegate callback_delegate = DebugLogCallback;
  }
}
