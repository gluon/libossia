using System;
using System.Collections;
using System.Net;
using System.Net.Sockets;
using System.Text;
using NUnit.Framework;
using UnityEditor;
using UnityEngine;
using UnityEngine.TestTools;

namespace Ossia.Tests
{
  // Play, Stop, Play again in the same editor: the Controller frees its
  // device when Play stops, so the second Play gets the ports back.
  // EnterPlayMode and ExitPlayMode exist only in Edit Mode tests. The test
  // keeps no local variable across them: the domain reloads on each.
  public class PlayCycleTests
  {
    internal const int OscPort = 1234;
    internal const int WsPort = 5678;

    [UnityTest]
    public IEnumerator SecondPlayGetsThePortsBack()
    {
      Assert.IsTrue(PortsFree(), "ports 1234 and 5678 must be free before the test");
      new GameObject("OssiaController").AddComponent<Controller>();

      yield return new EnterPlayMode();
      yield return null;
      StringAssert.Contains("\"FULL_PATH\"", HttpGet(WsPort), "first Play: the OSCQuery server answers");

      yield return new ExitPlayMode();
      Assert.IsNull(HttpGet(WsPort), "after Stop: nothing answers on 5678");
      Assert.IsTrue(PortsFree(), "after Stop: ports 1234 and 5678 are free");

      yield return new EnterPlayMode();
      yield return null;
      StringAssert.Contains("\"FULL_PATH\"", HttpGet(WsPort), "second Play: the OSCQuery server answers again");

      yield return new ExitPlayMode();
      Assert.IsTrue(PortsFree(), "after the second Stop: ports 1234 and 5678 are free");
    }

    // The scene of the demo: a Controller that serves a cube, a Client that
    // follows that same server and a Remote bound to one of its addresses.
    // Leaving Play disables the Client after the Controller's
    // OnApplicationQuit: the Client still frees its own mirror then.
    [UnityTest]
    public IEnumerator StopWithAClientConnected()
    {
      Assert.IsTrue(PortsFree(), "ports 1234 and 5678 must be free before the test");
      new GameObject("OssiaController").AddComponent<Controller>().resolveAnnouncedName = false;
      var cube = GameObject.CreatePrimitive(PrimitiveType.Cube);
      cube.name = "Cube";
      cube.AddComponent<ExposeAttributes>();
      var client = new GameObject("OssiaClient").AddComponent<Client>();
      client.server = "ws://127.0.0.1:" + WsPort;
      var remote = new GameObject("OssiaRemote").AddComponent<Remote>();
      remote.client = client;
      remote.address = "/Cube/transform/position";
      remote.target = cube.transform;
      remote.member = "position";

      yield return new EnterPlayMode();
      yield return WaitBound();
      Assert.IsTrue(FindRemote().Bound, "first Play: the Client is connected and the Remote bound");

      yield return new ExitPlayMode();
      Assert.IsTrue(PortsFree(), "after Stop: ports 1234 and 5678 are free");

      yield return new EnterPlayMode();
      yield return WaitBound();
      Assert.IsTrue(FindRemote().Bound, "second Play: the Client is connected and the Remote bound");

      yield return new ExitPlayMode();
      Assert.IsTrue(PortsFree(), "after the second Stop: ports 1234 and 5678 are free");
    }

    static Remote FindRemote()
    {
      return UnityEngine.Object.FindAnyObjectByType<Remote>();
    }

    static IEnumerator WaitBound()
    {
      float end = Time.realtimeSinceStartup + 10;
      while (!FindRemote().Bound && Time.realtimeSinceStartup < end)
        yield return null;
    }

    [TearDown]
    public void RemoveController()
    {
      foreach (var name in new[] { "OssiaController", "Cube", "OssiaClient", "OssiaRemote" }) {
        var go = GameObject.Find(name);
        if (go != null)
          UnityEngine.Object.DestroyImmediate(go);
      }
    }

    // The body of GET / on the port, null if nothing answers.
    internal static string HttpGet(int port)
    {
      try {
        using (var c = new TcpClient()) {
          if (!c.ConnectAsync(IPAddress.Loopback, port).Wait(2000))
            return null;
          c.ReceiveTimeout = 2000;
          var s = c.GetStream();
          var req = Encoding.ASCII.GetBytes("GET / HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n");
          s.Write(req, 0, req.Length);
          var sb = new StringBuilder();
          var buf = new byte[65536];
          int n;
          while ((n = s.Read(buf, 0, buf.Length)) > 0)
            sb.Append(Encoding.UTF8.GetString(buf, 0, n));
          return sb.ToString();
        }
      } catch (Exception) {
        return null;
      }
    }

    internal static bool PortsFree()
    {
      try {
        using (var u = new Socket(AddressFamily.InterNetwork, SocketType.Dgram, ProtocolType.Udp))
          u.Bind(new IPEndPoint(IPAddress.Any, OscPort));
        using (var t = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp))
          t.Bind(new IPEndPoint(IPAddress.Any, WsPort));
        return true;
      } catch (SocketException) {
        return false;
      }
    }
  }
}
