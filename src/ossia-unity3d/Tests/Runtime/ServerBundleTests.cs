using System;
using System.Collections;
using System.Linq;
using System.Net.WebSockets;
using System.Text;
using System.Threading;
using NUnit.Framework;
using UnityEngine;
using UnityEngine.TestTools;

namespace Ossia.Tests
{
  public class BundleFields : MonoBehaviour
  {
    [Ossia.Expose("a")] public float a;
    [Ossia.Expose("b")] public float b;
    [Ossia.Expose("c")] public float c;
    [Ossia.Expose("d")] public float d;
    [Ossia.Expose("e")] public float e;
    [Ossia.Expose("f")] public float f;
    [Ossia.Expose("g")] public float g;
    [Ossia.Expose("h")] public float h;
  }

  // The fields an exposed object changes in a frame reach a client of the
  // Controller in one bundle. The client is reduced to its WebSocket, and
  // asks the server to send the values to a UDP port of the test.
  public class ServerBundleTests
  {
    const int OscPort = 30234;
    const int WsPort = 30678;

    GameObject controllerObject, shape;
    UdpSink sink;
    ClientWebSocket ws;

    [UnityTearDown]
    public IEnumerator Destroy ()
    {
      if (ws != null)
        ws.Abort ();
      ws = null;
      if (sink != null)
        sink.Dispose ();
      sink = null;
      if (shape != null)
        UnityEngine.Object.DestroyImmediate (shape);
      if (controllerObject != null)
        UnityEngine.Object.DestroyImmediate (controllerObject);
      yield return null;
    }

    [UnityTest]
    public IEnumerator ChangesOfAFrameLeaveInOneBundle ()
    {
      for (GameObject old; (old = GameObject.Find ("OssiaController")) != null; )
        UnityEngine.Object.DestroyImmediate (old);
      controllerObject = new GameObject ("OssiaController");
      controllerObject.SetActive (false);
      var controller = controllerObject.AddComponent<Controller> ();
      controller.deviceName = "ossia_unity_bundle_test";
      controller.oscPort = OscPort;
      controller.wsPort = WsPort;
      controller.resolveAnnouncedName = false;
      controllerObject.SetActive (true);

      shape = new GameObject ("Shape");
      var fields = shape.AddComponent<BundleFields> ();
      shape.AddComponent<ExposeAttributes> ();
      yield return null;
      yield return null;

      sink = new UdpSink (0);
      ws = new ClientWebSocket ();
      var connect = ws.ConnectAsync (new Uri ("ws://127.0.0.1:" + WsPort), CancellationToken.None);
      yield return new WaitUntil (() => connect.IsCompleted);
      Assert.AreEqual (WebSocketState.Open, ws.State, "WebSocket");
      var start = "{\"COMMAND\":\"START_OSC_STREAMING\",\"DATA\":{\"LOCAL_SERVER_PORT\":" + sink.Port + ",\"LOCAL_SENDER_PORT\":0}}";
      var send = ws.SendAsync (new ArraySegment<byte> (Encoding.UTF8.GetBytes (start)), WebSocketMessageType.Text, true, CancellationToken.None);
      yield return new WaitUntil (() => send.IsCompleted);
      // A client gets only the addresses it listens to
      foreach (var name in new[] { "a", "b", "c", "d", "e", "f", "g", "h" }) {
        var listen = "{\"COMMAND\":\"LISTEN\",\"DATA\":\"/Shape/" + typeof (BundleFields).ToString () + "/" + name + "\"}";
        send = ws.SendAsync (new ArraySegment<byte> (Encoding.UTF8.GetBytes (listen)), WebSocketMessageType.Text, true, CancellationToken.None);
        yield return new WaitUntil (() => send.IsCompleted);
      }
      yield return new WaitForSecondsRealtime (0.3f);
      sink.Drain ();

      Set (fields, 1);
      yield return new WaitForSecondsRealtime (0.3f);
      var bundled = sink.Drain ();
      Debug.Log ("SERVER bundles on: datagrams=" + bundled.Length + " messages=" + bundled.Sum ());
      Assert.AreEqual (new[] { 8 }, bundled, "one datagram, eight messages");

      controller.bundles = false;
      Set (fields, 2);
      yield return new WaitForSecondsRealtime (0.3f);
      var single = sink.Drain ();
      Debug.Log ("SERVER bundles off: datagrams=" + single.Length + " messages=" + single.Sum ());
      Assert.AreEqual (8, single.Length, "bundles off: one datagram per value");
    }

    static void Set (BundleFields f, float v)
    {
      f.a = f.b = f.c = f.d = v;
      f.e = f.f = f.g = f.h = v;
    }
  }
}
