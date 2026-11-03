using System.Collections;
using System.Collections.Generic;
using System.Net;
using System.Net.Sockets;
using System.Text;
using NUnit.Framework;
using UnityEngine;
using UnityEngine.TestTools;

namespace Ossia.Tests
{
  // A Controller destroyed without leaving Play frees its device and its
  // ports: a new Controller on the same ports serves its own tree.
  public class ControllerLifetimeTests
  {
    const int OscPort = 27234;
    const int WsPort = 27678;
    const string Name = "ossia_unity_lifetime_test";

    readonly List<string> logs = new List<string>();
    void OnLog(string message, string stack, LogType type) { logs.Add(message); }

    static Controller StartController()
    {
      var go = new GameObject("OssiaController");
      go.SetActive(false);
      var c = go.AddComponent<Controller>();
      c.deviceName = Name;
      c.oscPort = OscPort;
      c.wsPort = WsPort;
      c.resolveAnnouncedName = false;
      go.SetActive(true);
      return c;
    }

    [UnityTest]
    public IEnumerator RecreatedControllerServesOnTheSamePorts()
    {
      Application.logMessageReceived += OnLog;
      try {
        var first = StartController();
        var a = new GameObject("First");
        a.AddComponent<ExposeAttributes>();
        yield return null;
        yield return null;
        StringAssert.Contains("/First", HttpGet(WsPort));

        Object.DestroyImmediate(a);
        Object.DestroyImmediate(first.gameObject);
        yield return null;

        var second = StartController();
        var b = new GameObject("Second");
        b.AddComponent<ExposeAttributes>();
        yield return null;
        yield return null;

        var tree = HttpGet(WsPort);
        StringAssert.Contains("/Second", tree);
        StringAssert.DoesNotContain("/First", tree);
        Assert.IsFalse(logs.Exists(l => l.Contains("Transport Error")), string.Join("\n", logs));

        Object.DestroyImmediate(b);
        Object.DestroyImmediate(second.gameObject);
      } finally {
        Application.logMessageReceived -= OnLog;
      }
    }

    // An object exposed under a Controller that is destroyed before it stops
    // publishing, without an error.
    [UnityTest]
    public IEnumerator ExposedObjectOutlivesItsController()
    {
      Application.logMessageReceived += OnLog;
      try {
        var c = StartController();
        var a = new GameObject("Outlives");
        a.AddComponent<ExposeAttributes>();
        yield return null;
        yield return null;

        Object.DestroyImmediate(c.gameObject);
        for (int i = 0; i < 5; i++) {
          a.transform.position += Vector3.one;
          yield return null;
        }
        Assert.IsFalse(logs.Exists(l => l.Contains("is null")), string.Join("\n", logs));
        Object.DestroyImmediate(a);
      } finally {
        Application.logMessageReceived -= OnLog;
      }
    }

    static string HttpGet(int port)
    {
      using (var c = new TcpClient()) {
        c.Connect(IPAddress.Loopback, port);
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
    }
  }
}
