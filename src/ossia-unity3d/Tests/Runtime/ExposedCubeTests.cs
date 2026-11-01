using System.Collections;
using System.Net;
using System.Net.Sockets;
using System.Text;
using NUnit.Framework;
using UnityEngine;
using UnityEngine.TestTools;

namespace Ossia.Tests
{
  // A Controller and a cube with ExposeAttributes, as in the sample scene:
  // the cube's transform is in the OSCQuery tree, and an OSC message sent
  // from outside moves it.
  public class ExposedCubeTests
  {
    const int OscPort = 11234;
    const int WsPort = 15678;

    [UnityTest]
    public IEnumerator OscMessageMovesTheCube()
    {
      var go = new GameObject("OssiaController");
      go.SetActive(false);
      var c = go.AddComponent<Controller>();
      c.oscPort = OscPort;
      c.wsPort = WsPort;
      go.SetActive(true);

      var cube = GameObject.CreatePrimitive(PrimitiveType.Cube);
      cube.name = "Cube";
      cube.AddComponent<ExposeAttributes>();
      yield return null;

      StringAssert.Contains("/Cube/transform/position", HttpGet(WsPort));

      using (var u = new UdpClient()) {
        var msg = Osc("/Cube/transform/position", 1, 2, 3);
        u.Send(msg, msg.Length, new IPEndPoint(IPAddress.Loopback, OscPort));
      }

      float end = Time.realtimeSinceStartup + 2;
      while (cube.transform.position != new Vector3(1, 2, 3) && Time.realtimeSinceStartup < end)
        yield return null;
      Assert.AreEqual(new Vector3(1, 2, 3), cube.transform.position);
    }

    static byte[] Osc(string address, params float[] values)
    {
      var b = new System.Collections.Generic.List<byte>();
      void Pad(byte[] s) { b.AddRange(s); b.AddRange(new byte[4 - s.Length % 4]); }
      Pad(Encoding.ASCII.GetBytes(address));
      Pad(Encoding.ASCII.GetBytes("," + new string('f', values.Length)));
      foreach (var v in values) {
        var f = System.BitConverter.GetBytes(v);
        System.Array.Reverse(f);
        b.AddRange(f);
      }
      return b.ToArray();
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
