using System;
using System.Collections.Generic;
using System.Threading;
using UnityEngine;

namespace Ossia
{
  public enum ClientStatus { Disconnected, Connecting, Connected, Lost }

  //! Follows a remote OSCQuery device, as ossia.client does in Max: a Max
  //! patch, TouchDesigner, another Unity. The Remote components bind the
  //! fields of scripts to its addresses.
  public class Client : MonoBehaviour
  {
    [Tooltip ("The device to follow: the name it is announced under over mDNS (\"max\"), "
            + "or its address, ws://host:port")]
    public string server = "ws://127.0.0.1:5678";
    [Tooltip ("Connect again, every second, when the server goes away, until it comes back")]
    public bool reconnect = true;
    [Tooltip ("Send the values that change during a frame together, at the end of the frame, in OSC bundles. "
            + "Off, each value leaves as its own message as soon as it changes.")]
    public bool bundles = true;
    [Tooltip ("How long a connection by name looks for the device over mDNS, in milliseconds")]
    public int browseMs = 2000;
    [Tooltip ("Read only: where the connection stands")]
    [SerializeField] string status = "Disconnected";

    public ClientStatus Status { get; private set; }
    //! The address of the server while connected: ws://host:port
    public string Url { get; private set; }
    //! Why the last attempt failed
    public string LastError { get; private set; }
    //! The mirror of the remote device while connected, null otherwise
    public Ossia.Device Device { get { return device; } }

    void OnEnable ()
    {
      Connect ();
    }

    void OnDisable ()
    {
      Disconnect ();
    }

    //! Starts a connection, on a thread of its own: reading the name over
    //! mDNS, the WebSocket and the tree take seconds, which would hold the
    //! frame, and a server of this same Unity could not answer meanwhile.
    public void Connect ()
    {
      Disconnect ();
      wanted = true;
      // The Controller leaves the library as it is while this one may hold
      // a mirror
      held = true;
      Controller.Hold ();
      SetStatus (ClientStatus.Connecting, "Connecting to " + server);
      StartAttempt ();
    }

    public void Disconnect ()
    {
      wanted = false;
      retryAt = -1;
      if (attempt != null) {
        attempt.Join ();
        attempt.Release ();
        attempt = null;
      }
      Detach ();
      if (Status != ClientStatus.Disconnected)
        SetStatus (ClientStatus.Disconnected, "Disconnected");
      if (held) {
        held = false;
        Controller.Let ();
      }
    }

    void StartAttempt ()
    {
      attempts++;
      attempt = new Attempt (server, browseMs, "unity_client_" + GetInstanceID () + "_" + attempts);
    }

    void Update ()
    {
      if (attempt != null && attempt.Done) {
        var a = attempt;
        attempt = null;
        a.Join ();
        if (a.Device != IntPtr.Zero)
          Attach (a);
        else
          Failed (a.Error);
      }

      if (device != null) {
        // The nodes the server added or removed since the last frame
        device.RunCommands ();
        if (!device.Connected ()) {
          Debug.LogWarning ("OSSIA client: lost " + Url);
          Detach ();
          SetStatus (ClientStatus.Lost, "Lost " + lostUrl);
          if (reconnect && wanted)
            retryAt = Time.realtimeSinceStartup + retrySeconds;
          return;
        }
        Ossia.Message m;
        while (queue.Pop (out m)) {
          List<Remote> l;
          if (byParameter.TryGetValue (m.Address, out l))
            foreach (var r in l)
              r.ReceiveUpdates ();
        }
        foreach (var r in remotes)
          if (!r.Bound)
            Bind (r);
      }

      if (retryAt >= 0 && attempt == null && Time.realtimeSinceStartup >= retryAt) {
        retryAt = -1;
        StartAttempt ();
      }
    }

    // After the Update of every script: what changed this frame
    void LateUpdate ()
    {
      if (device == null)
        return;
      foreach (var r in remotes)
        r.SendUpdates ();
      if (pending_parameters.Count > 0) {
        device.PushBundle (pending_parameters, pending_values);
        pending_parameters.Clear ();
        pending_values.Clear ();
      }
    }

    void OnDestroy ()
    {
      Disconnect ();
    }

    void Attach (Attempt a)
    {
      device = new Ossia.Device (a.Device);
      Url = a.Url;
      lostUrl = a.Url;
      queue = new Ossia.MessageQueue (device);
      foreach (var r in remotes)
        Bind (r);
      SetStatus (ClientStatus.Connected, "Connected to " + Url);
      Debug.Log ("OSSIA client: connected to " + Url + (Url == server ? "" : " (" + server + ")"));
    }

    void Failed (string error)
    {
      LastError = error;
      if (Status == ClientStatus.Lost) {
        // Still away: tried again in a second
        SetStatus (ClientStatus.Lost, "Lost " + lostUrl + ": " + error);
      } else {
        Debug.LogWarning ("OSSIA client: " + server + ": " + error);
        SetStatus (reconnect ? ClientStatus.Lost : ClientStatus.Disconnected, error);
      }
      if (reconnect && wanted)
        retryAt = Time.realtimeSinceStartup + retrySeconds;
    }

    // Frees the mirror; the remotes bind again to the next one
    void Detach ()
    {
      foreach (var r in remotes)
        r.Unbind ();
      byParameter.Clear ();
      pending_parameters.Clear ();
      pending_values.Clear ();
      if (queue != null)
        queue.Free ();
      queue = null;
      if (device != null)
        device.Free ();
      device = null;
      Url = null;
    }

    void SetStatus (ClientStatus s, string text)
    {
      Status = s;
      status = text;
    }

    internal void Add (Remote r)
    {
      if (remotes.Contains (r))
        return;
      remotes.Add (r);
      if (device != null)
        Bind (r);
    }

    internal void Remove (Remote r)
    {
      remotes.Remove (r);
      Forget (r);
      r.Unbind ();
    }

    void Bind (Remote r)
    {
      var p = r.Bind (device);
      if (p == null)
        return;
      List<Remote> l;
      if (!byParameter.TryGetValue (p.ossia_parameter, out l)) {
        l = new List<Remote> ();
        byParameter.Add (p.ossia_parameter, l);
        // The mirror listens to the address from now on
        queue.Register (p);
      }
      l.Add (r);
    }

    void Forget (Remote r)
    {
      if (r.Parameter == null)
        return;
      List<Remote> l;
      if (byParameter.TryGetValue (r.Parameter.ossia_parameter, out l)) {
        l.Remove (r);
        if (l.Count == 0) {
          byParameter.Remove (r.Parameter.ossia_parameter);
          if (queue != null)
            queue.Unregister (r.Parameter);
        }
      }
    }

    //! What a Remote sends: at the end of the frame, in one bundle, or at
    //! once when bundles is off.
    internal void Publish (Ossia.Parameter p, Ossia.Value v)
    {
      if (!bundles) {
        p.PushValue (v);
        return;
      }
      pending_parameters.Add (p);
      pending_values.Add (v);
    }

    // One connection: the name read over mDNS, the WebSocket, the tree
    class Attempt
    {
      public volatile bool Done;
      public IntPtr Device = IntPtr.Zero;
      public string Url;
      public string Error;
      readonly Thread thread;

      public Attempt (string server, int browseMs, string deviceName)
      {
        thread = new Thread (() => {
          try {
            Run (server, browseMs, deviceName);
          } catch (Exception e) {
            Error = e.Message;
          }
          Done = true;
        });
        thread.IsBackground = true;
        thread.Name = "ossia client connection";
        thread.Start ();
      }

      void Run (string server, int browseMs, string deviceName)
      {
        if (server.StartsWith ("ws://")) {
          Url = server;
        } else {
          foreach (var d in Zeroconf.ListOscqueryDevices (browseMs)) {
            if (d.Name == server) {
              Url = "ws://" + d.Host.TrimEnd ('.') + ":" + d.Port;
              break;
            }
          }
          if (Url == null) {
            Error = "\"" + server + "\" is not announced over mDNS";
            return;
          }
        }

        var proto = Network.ossia_protocol_oscquery_mirror_create (Url);
        if (proto == IntPtr.Zero) {
          Error = "no OSCQuery server answers at " + Url;
          return;
        }
        // A name of its own: the library keeps a device by its name until
        // it is freed
        var dev = Network.ossia_device_create (proto, deviceName);
        if (dev == IntPtr.Zero) {
          Error = "the device could not be created";
          return;
        }
        if (Network.ossia_device_update_namespace (dev) == 0) {
          Network.ossia_device_free (dev);
          Error = "the tree of " + Url + " could not be read";
          return;
        }
        Device = dev;
      }

      public void Join ()
      {
        thread.Join ();
      }

      //! The device of an attempt nobody takes
      public void Release ()
      {
        if (Device != IntPtr.Zero)
          Network.ossia_device_free (Device);
        Device = IntPtr.Zero;
      }
    }

    const float retrySeconds = 1;

    bool wanted;
    bool held;
    int attempts;
    float retryAt = -1;
    string lostUrl;
    Attempt attempt;
    Ossia.Device device;
    Ossia.MessageQueue queue;
    readonly List<Remote> remotes = new List<Remote> ();
    readonly Dictionary<IntPtr, List<Remote>> byParameter = new Dictionary<IntPtr, List<Remote>> ();
    readonly List<Ossia.Parameter> pending_parameters = new List<Ossia.Parameter> ();
    readonly List<Ossia.Value> pending_values = new List<Ossia.Value> ();
  }
}
