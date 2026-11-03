using System;
using System.Collections;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Linq;
using System.Net;
using System.Net.Sockets;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
using NUnit.Framework;
using UnityEngine;
using UnityEngine.TestTools;

namespace Ossia.Tests
{
  public class ClientFields : MonoBehaviour
  {
    public float level;
    public float p0, p1, p2, p3, p4, p5, p6, p7;
  }

  // A libossia server in the test, built as ossia-max builds the device of an
  // ossia.device: a multiplex protocol exposed through the OSCQuery server
  // of ossia-max, on a network context.
  class TestServer
  {
    public readonly string Name;
    public readonly int OscPort, WsPort;
    public Device Device;
    public Parameter Level;
    public readonly List<Parameter> P = new List<Parameter> ();
    NetworkContext ctx;
    IntPtr callback;

    [DllImport ("ossia")] static extern IntPtr ossia_node_get_parameter (IntPtr node);
    [DllImport ("ossia")] static extern IntPtr ossia_parameter_add_callback (IntPtr param, IntPtr callback, IntPtr ctx);
    [DllImport ("ossia")] static extern float ossia_value_to_float (IntPtr value);
    delegate void ValueCallback (IntPtr ctx, IntPtr value);
    static readonly ValueCallback onValue = OnValue;
    //! The values /level received from the network, not the ones the test pushed
    public static readonly ConcurrentQueue<float> Received = new ConcurrentQueue<float> ();
    static int mainThread;

    [AOT.MonoPInvokeCallback (typeof (ValueCallback))]
    static void OnValue (IntPtr ctx, IntPtr value)
    {
      if (Thread.CurrentThread.ManagedThreadId != mainThread)
        Received.Enqueue (ossia_value_to_float (value));
    }

    public TestServer (string name, int osc, int ws, float level = 0.3f)
    {
      Name = name;
      OscPort = osc;
      WsPort = ws;
      mainThread = Thread.CurrentThread.ManagedThreadId;
      ctx = new NetworkContext ();
      var local = new Local ();
      Device = new Device (local, name);
      var root = Device.GetRootNode ();
      var levelNode = Node.CreateNode (root, "/level");
      Level = levelNode.CreateParameter (ossia_type.FLOAT);
      Level.PushValue (level);
      for (int i = 0; i < 8; i++)
        P.Add (Node.CreateNode (root, "/p" + i).CreateParameter (ossia_type.FLOAT));
      callback = ossia_parameter_add_callback (ossia_node_get_parameter (levelNode.GetNode ()), Marshal.GetFunctionPointerForDelegate (onValue), IntPtr.Zero);
      local.ExposeTo (new OSCQuery (ctx, osc, ws));
      ctx.Start ();
    }

    public string Url { get { return "ws://127.0.0.1:" + WsPort; } }

    public void Stop ()
    {
      if (Device == null)
        return;
      ctx.Stop ();
      Device.Free ();
      ctx.Free ();
      Device = null;
    }

    public static float[] Drain ()
    {
      var l = new List<float> ();
      while (Received.TryDequeue (out var v))
        l.Add (v);
      return l.ToArray ();
    }
  }

  // Ossia.Client and Ossia.Remote against a libossia server: the tree is
  // read, the server's value reaches the field, every change of the field
  // leaves, nothing comes back, the client reconnects, finds a device by its
  // name, and sends what changes in a frame in one bundle.
  public class ClientTests
  {
    const int OscPort = 29234;
    const int WsPort = 29678;

    TestServer server;
    GameObject clientObject, shape;
    Client client;
    ClientFields fields;
    UdpSink sink;
    readonly List<UnityEngine.Object> created = new List<UnityEngine.Object> ();

    [UnityTearDown]
    public IEnumerator Destroy ()
    {
      if (sink != null)
        sink.Dispose ();
      sink = null;
      foreach (var o in created)
        if (o != null)
          UnityEngine.Object.DestroyImmediate (o);
      created.Clear ();
      if (server != null)
        server.Stop ();
      server = null;
      yield return null;
    }

    IEnumerator Connect (string target, int index = 0)
    {
      while (TestServer.Received.TryDequeue (out _)) { }
      if (server == null)
        server = new TestServer ("ossia_unity_client_server", OscPort + index, WsPort + index);

      shape = new GameObject ("Shape");
      created.Add (shape);
      fields = shape.AddComponent<ClientFields> ();

      clientObject = new GameObject ("OssiaClient");
      created.Add (clientObject);
      clientObject.SetActive (false);
      client = clientObject.AddComponent<Client> ();
      client.server = target ?? server.Url;
      clientObject.SetActive (true);

      AddRemote ("/level", "level");
      yield return Until (() => client.Status == ClientStatus.Connected, 10);
      Assert.AreEqual (ClientStatus.Connected, client.Status, client.LastError);
      yield return null;
    }

    Remote AddRemote (string address, string member)
    {
      var r = shape.AddComponent<Remote> ();
      r.enabled = false;
      r.client = client;
      r.address = address;
      r.target = fields;
      r.member = member;
      r.enabled = true;
      return r;
    }

    static IEnumerator Until (Func<bool> done, float seconds)
    {
      float end = Time.realtimeSinceStartup + seconds;
      while (!done () && Time.realtimeSinceStartup < end)
        yield return null;
    }

    [UnityTest]
    public IEnumerator TreeReadAndValuesBothWays ()
    {
      yield return Connect (null);
      Assert.IsTrue (client.Device.GetRootNode ().ChildSize () >= 9, "tree read");

      // The server's value, read with the tree
      yield return Until (() => fields.level == 0.3f, 2);
      Assert.AreEqual (0.3f, fields.level, "value of the server on connection");

      // Server -> field
      server.Level.PushValue (0.8f);
      yield return Until (() => fields.level == 0.8f, 2);
      Assert.AreEqual (0.8f, fields.level, "value of the server applied");

      // Field -> server
      fields.level = 0.42f;
      yield return Until (() => server.Level.GetValue ().GetFloat () == 0.42f, 2);
      Assert.AreEqual (0.42f, server.Level.GetValue ().GetFloat (), "field sent");

      yield return new WaitForSecondsRealtime (0.3f);
      var received = TestServer.Drain ();
      Debug.Log ("CLIENT received by the server: " + string.Join (" ", received));
      Assert.AreEqual (new[] { 0.42f }, received, "the server receives the change once, and not its own values");
    }

    [UnityTest]
    public IEnumerator InspectorAndAnimationAreSent ()
    {
      yield return Connect (null);
      yield return Until (() => fields.level == 0.3f, 2);
      TestServer.Drain ();

#if UNITY_EDITOR
      var so = new UnityEditor.SerializedObject (fields);
      so.FindProperty ("level").floatValue = 0.55f;
      so.ApplyModifiedProperties ();
      yield return Until (() => TestServer.Received.Contains (0.55f), 2);
      Assert.Contains (0.55f, TestServer.Drain (), "Inspector");
#endif

      var clip = new AnimationClip { legacy = true };
      clip.SetCurve ("", typeof (ClientFields), "level", AnimationCurve.Linear (0, 0, 0.5f, 1));
      var animation = shape.AddComponent<Animation> ();
      animation.AddClip (clip, "ramp");
      animation.Play ("ramp");
      yield return new WaitForSecondsRealtime (0.8f);
      animation.Stop ();
      yield return new WaitForSecondsRealtime (0.3f);
      var ramp = TestServer.Drain ();
      Debug.Log ("CLIENT animation values received by the server=" + ramp.Length);
      Assert.Greater (ramp.Length, 1, "animation");
    }

    // The server and a script write the same address, in turn, for 5 s
    [UnityTest]
    public IEnumerator NoEchoWhenBothWriteFor5Seconds ()
    {
      yield return Connect (null);
      TestServer.Drain ();
      const float ServerBase = 1000, ScriptBase = 2000;

      int serverWrites = 0, scriptWrites = 0;
      var all = new List<float> ();
      float end = Time.realtimeSinceStartup + 5;
      for (int i = 0; Time.realtimeSinceStartup < end; i++) {
        if (i % 2 == 0)
          server.Level.PushValue (ServerBase + (serverWrites++ % 900));
        else
          fields.level = ScriptBase + (scriptWrites++ % 900);
        yield return new WaitForSecondsRealtime (1f / 120);
        all.AddRange (TestServer.Drain ());
      }

      server.Level.PushValue (ServerBase + 950);
      yield return Until (() => fields.level == ServerBase + 950, 2);
      Assert.AreEqual (ServerBase + 950, fields.level, "Unity converges on the server's last value");

      fields.level = ScriptBase + 950;
      yield return Until (() => TestServer.Received.Contains (ScriptBase + 950), 2);
      all.AddRange (TestServer.Drain ());
      Assert.Contains (ScriptBase + 950, all, "the server receives the script's last value");

      yield return new WaitForSecondsRealtime (1);
      var after = TestServer.Drain ();
      all.AddRange (after);
      var echoes = all.Count (v => v >= ServerBase && v < ScriptBase);
      Debug.Log ("CLIENT serverWrites=" + serverWrites + " scriptWrites=" + scriptWrites
        + " received=" + all.Count + " echoes=" + echoes + " afterStop=" + after.Length);
      Assert.AreEqual (0, echoes, "values the server wrote came back to it");
      Assert.AreEqual (0, after.Length, "messages after both stopped");
      Assert.LessOrEqual (all.Count, scriptWrites + 1, "at most one message per change of the field");
    }

    [UnityTest]
    public IEnumerator ReconnectsWhenTheServerComesBack ()
    {
      yield return Connect (null, 1);
      var url = server.Url;

      server.Stop ();
      server = null;
      LogAssert.Expect (LogType.Warning, "OSSIA client: lost " + url);
      yield return Until (() => client.Status == ClientStatus.Lost, 3);
      Assert.AreEqual (ClientStatus.Lost, client.Status, "after the server stopped");
      yield return new WaitForSecondsRealtime (1.5f);
      Assert.AreEqual (ClientStatus.Lost, client.Status, "while the server is away");

      server = new TestServer ("ossia_unity_client_server", OscPort + 1, WsPort + 1, 0.6f);
      yield return Until (() => client.Status == ClientStatus.Connected, 6);
      Assert.AreEqual (ClientStatus.Connected, client.Status, "after the server came back: " + client.LastError);
      yield return Until (() => fields.level == 0.6f, 2);
      Assert.AreEqual (0.6f, fields.level, "the value of the new server");

      fields.level = 0.9f;
      yield return Until (() => server.Level.GetValue ().GetFloat () == 0.9f, 2);
      Assert.AreEqual (0.9f, server.Level.GetValue ().GetFloat (), "field sent to the new server");
    }

    [UnityTest]
    public IEnumerator ConnectsByName ()
    {
      var name = "ossia_unity_client_test_" + System.Diagnostics.Process.GetCurrentProcess ().Id;
      server = new TestServer (name, OscPort + 2, WsPort + 2);
      yield return Connect (name, 2);
      Debug.Log ("CLIENT by name: " + name + " -> " + client.Url);
      StringAssert.EndsWith (":" + (WsPort + 2), client.Url);
      yield return Until (() => fields.level == 0.3f, 2);
      Assert.AreEqual (0.3f, fields.level);
    }

    // Eight fields changed in one frame leave in one datagram
    [UnityTest]
    public IEnumerator ChangesOfAFrameLeaveInOneBundle ()
    {
      yield return Connect (null, 3);
      for (int i = 0; i < 8; i++)
        AddRemote ("/p" + i, "p" + i);
      yield return null;
      yield return null;
      sink = new UdpSink (server.OscPort);

      SetAll (1);
      yield return new WaitForSecondsRealtime (0.3f);
      var bundled = sink.Drain ();
      Debug.Log ("CLIENT bundles on: datagrams=" + bundled.Length + " messages=" + bundled.Sum ());
      Assert.AreEqual (new[] { 8 }, bundled, "one datagram, eight messages");

      client.bundles = false;
      SetAll (2);
      yield return new WaitForSecondsRealtime (0.3f);
      var single = sink.Drain ();
      Debug.Log ("CLIENT bundles off: datagrams=" + single.Length + " messages=" + single.Sum ());
      Assert.AreEqual (8, single.Length, "bundles off: one datagram per value");
    }

    void SetAll (float v)
    {
      fields.p0 = fields.p1 = fields.p2 = fields.p3 = v;
      fields.p4 = fields.p5 = fields.p6 = fields.p7 = v;
    }
  }
}
