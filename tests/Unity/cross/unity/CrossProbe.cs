// SPDX-FileCopyrightText: 2026 CNRS
// SPDX-FileContributor: Julien Bayle, PRISM <https://www.prism.cnrs.fr/>

// The Unity player of the cross bench (tests/Unity/cross/cross_run.py), with
// no click. Started with:
//   -crossOut <folder>   log file <folder>/<tag>.log; without it, nothing runs
//   -crossTag <tag>      unity1 or unity2, the prefix of every reply
//   -crossListen <port>  UDP port of the commands, on 127.0.0.1
//   -crossReply <port>   UDP port of the replies
// Commands, one per datagram:
//   hello                         -> ready
//   serve <name> <osc> <ws>       a Controller and /probe/CrossFields/value -> served
//   follow <server> <address>     an Ossia.Client and an Ossia.Remote on <address>
//   status                        -> status <Status> <bound 0|1> <url>
//   unfollow                      the Client goes away
//   set <v>                       the field takes v (published at the end of the frame)
//   read                          -> value <v>
//   reset, count                  -> count <arrivals> <own>: the values that reached the
//                                 parameter (a message queue of the probe's own) and,
//                                 among them, the ones published here
//   burst <n> <frames>            value set n times, every <frames> frames -> burst_done <n> <last>
//   scene <server>                the scene of the sample "Follow a device", its Client
//                                 following <server> -> scened
//   cube                          -> cube <Status> <size> <r> <g> <b> <a> <spin> <angle>
//   quit
using System;
using System.Collections.Concurrent;
using System.Globalization;
using System.IO;
using System.Net;
using System.Net.Sockets;
using System.Reflection;
using System.Text;
using UnityEngine;
using UnityEngine.SceneManagement;

public class CrossProbe : MonoBehaviour
{
  UdpClient socket;
  IPEndPoint runner;
  string tag, log;
  readonly ConcurrentQueue<string> inbox = new ConcurrentQueue<string> ();

  GameObject controllerObject, probe, clientObject;
  CrossFields fields;
  Ossia.Client client;
  Ossia.Remote remote;
  Ossia.MessageQueue queue;
  Ossia.Device queued;
  int arrivals, own;
  int burstLeft, burstEvery, burstWait, burstCount;
  float burstValue;

  static string Arg (string name)
  {
    var a = Environment.GetCommandLineArgs ();
    for (int i = 0; i < a.Length - 1; i++)
      if (a[i] == name)
        return a[i + 1];
    return null;
  }

  static string F (float v)
  {
    return v.ToString ("0.######", CultureInfo.InvariantCulture);
  }

  static float P (string s)
  {
    return float.Parse (s, CultureInfo.InvariantCulture);
  }

  [RuntimeInitializeOnLoadMethod (RuntimeInitializeLoadType.AfterSceneLoad)]
  static void Boot ()
  {
    var dir = Arg ("-crossOut");
    if (dir == null)
      return;
    var go = new GameObject ("CrossProbe");
    DontDestroyOnLoad (go);
    var p = go.AddComponent<CrossProbe> ();
    p.tag = Arg ("-crossTag") ?? "unity";
    p.log = Path.Combine (dir, p.tag + ".log");
    p.socket = new UdpClient (new IPEndPoint (IPAddress.Loopback, int.Parse (Arg ("-crossListen"))));
    p.runner = new IPEndPoint (IPAddress.Loopback, int.Parse (Arg ("-crossReply")));
    p.socket.BeginReceive (p.OnReceive, null);
  }

  void OnReceive (IAsyncResult r)
  {
    try {
      IPEndPoint from = null;
      var data = socket.EndReceive (r, ref from);
      inbox.Enqueue (Encoding.UTF8.GetString (data));
      socket.BeginReceive (OnReceive, null);
    } catch (ObjectDisposedException) {
    } catch (Exception e) {
      Say ("FAIL link: " + e.Message);
    }
  }

  void Tell (string text)
  {
    var b = Encoding.UTF8.GetBytes (tag + " " + text);
    socket.Send (b, b.Length, runner);
  }

  void Say (string text)
  {
    var t = (DateTime.UtcNow - new DateTime (1970, 1, 1, 0, 0, 0, DateTimeKind.Utc)).TotalSeconds;
    File.AppendAllText (log, t.ToString ("0.000", CultureInfo.InvariantCulture) + " " + text + "\n");
  }

  void Start ()
  {
    Application.runInBackground = true;
    Application.targetFrameRate = 60;
    Say ("started: Unity " + Application.unityVersion);
  }

  // The parameter the bench watches: the probe's own, or the remote one
  Ossia.Parameter Watched (out Ossia.Device device)
  {
    device = null;
    if (client != null && remote != null && remote.Bound && client.Device != null) {
      device = client.Device;
      var n = Ossia.Node.FindNode (device.GetRootNode (), remote.address);
      return n == null ? null : n.GetParameter ();
    }
    if (controllerObject != null) {
      var c = controllerObject.GetComponent<Ossia.Controller> ();
      device = c.GetDevice ();
      if (device == null)
        return null;
      var n = Ossia.Node.FindNode (device.GetRootNode (), "/probe/CrossFields/value");
      return n == null ? null : n.GetParameter ();
    }
    return null;
  }

  void Update ()
  {
    Ossia.Device device;
    var p = Watched (out device);
    if (p != null && device != queued) {
      if (queue != null)
        queue.Free ();
      queue = new Ossia.MessageQueue (device);
      queue.Register (p);
      queued = device;
    }
    if (p == null && queue != null) {
      queue.Free ();
      queue = null;
      queued = null;
    }
    if (queue != null) {
      Ossia.Message m;
      while (queue.Pop (out m))
        arrivals++;
    }
    if (burstLeft > 0 && --burstWait <= 0) {
      burstValue += 0.5f;
      Own (burstValue);
      burstLeft--;
      burstCount++;
      burstWait = burstEvery;
      if (burstLeft == 0) {
        Say ("burst done: " + burstCount + " values, the last " + F (burstValue));
        Tell ("burst_done " + burstCount + " " + F (burstValue));
      }
    }
    string line;
    while (inbox.TryDequeue (out line)) {
      try {
        Command (line.Trim ().Split (' '));
      } catch (Exception e) {
        Say ("FAIL command " + line + ": " + e.Message);
      }
    }
  }

  string sceneServer;

  // The scene of the sample: its Client follows the server asked
  void OnSceneLoaded (Scene s, LoadSceneMode m)
  {
    SceneManager.sceneLoaded -= OnSceneLoaded;
    var cl = FindFirstObjectByType<Ossia.Client> ();
    if (cl != null && sceneServer != null && cl.server != sceneServer) {
      cl.server = sceneServer;
      cl.Connect ();
    }
    client = cl;
    remote = null;
    Tell ("scened " + (cl != null ? cl.server : "-"));
  }

  // A change made here: published once at the end of the frame
  void Own (float v)
  {
    if (fields == null)
      return;
    if (fields.value != v)
      own++;
    fields.value = v;
  }

  void Command (string[] a)
  {
    Say ("command " + string.Join (" ", a));
    switch (a[0]) {
    case "hello":
      Tell ("ready");
      break;
    case "serve":
      controllerObject = new GameObject ("OssiaController");
      controllerObject.SetActive (false);
      var c = controllerObject.AddComponent<Ossia.Controller> ();
      c.deviceName = a[1];
      c.oscPort = int.Parse (a[2]);
      c.wsPort = int.Parse (a[3]);
      c.resolveAnnouncedName = false;
      controllerObject.SetActive (true);
      probe = new GameObject ("probe");
      fields = probe.AddComponent<CrossFields> ();
      probe.AddComponent<Ossia.ExposeAttributes> ();
      Tell ("served");
      break;
    case "follow":
      clientObject = new GameObject ("probe_client");
      clientObject.SetActive (false);
      client = clientObject.AddComponent<Ossia.Client> ();
      client.server = a[1];
      fields = clientObject.AddComponent<CrossFields> ();
      remote = clientObject.AddComponent<Ossia.Remote> ();
      remote.client = client;
      remote.address = a[2];
      remote.target = fields;
      remote.member = "value";
      clientObject.SetActive (true);
      break;
    case "status":
      if (client == null)
        Tell ("status none 0 -");
      else
        Tell ("status " + client.Status + " " + (remote != null && remote.Bound ? 1 : 0) + " " + (client.Url ?? "-"));
      break;
    case "unfollow":
      if (clientObject != null)
        Destroy (clientObject);
      clientObject = null;
      client = null;
      remote = null;
      fields = probe != null ? probe.GetComponent<CrossFields> () : null;
      break;
    case "set":
      Own (P (a[1]));
      break;
    case "read":
      Tell ("value " + (fields != null ? F (fields.value) : "-"));
      break;
    case "reset":
      arrivals = own = 0;
      Tell ("reset");
      break;
    case "count":
      Tell ("count " + arrivals + " " + own);
      break;
    case "burst":
      burstLeft = int.Parse (a[1]);
      burstEvery = Math.Max (1, int.Parse (a[2]));
      burstWait = burstEvery;
      burstCount = 0;
      burstValue = fields != null ? Mathf.Floor (fields.value) + 100 : 100;
      break;
    case "scene":
      sceneServer = a.Length > 1 ? a[1] : null;
      SceneManager.sceneLoaded += OnSceneLoaded;
      SceneManager.LoadScene ("FollowADevice");
      break;
    case "cube":
      var cube = GameObject.Find ("Cube");
      var follow = cube != null ? cube.GetComponent ("FollowADevice") : null;
      if (follow == null) {
        Tell ("cube none");
        break;
      }
      var t = follow.GetType ();
      float size = (float)t.GetField ("size").GetValue (follow);
      var color = (Color)t.GetField ("color").GetValue (follow);
      float spin = (float)t.GetField ("spin").GetValue (follow);
      float angle = (float)t.GetField ("angle").GetValue (follow);
      Tell ("cube " + (client != null ? client.Status.ToString () : "none") + " " + F (size) + " " + F (color.r) + " " + F (color.g)
            + " " + F (color.b) + " " + F (color.a) + " " + F (spin) + " " + F (angle));
      break;
    case "quit":
      Say ("quit");
      socket.Close ();
      Application.Quit ();
      break;
    }
  }
}
