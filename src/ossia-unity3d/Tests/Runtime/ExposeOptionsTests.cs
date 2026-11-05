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
  public class ExposeOptionsFields : MonoBehaviour
  {
    [Ossia.Expose("speed", Min = 0, Max = 360, Unit = "angle.degree", Access = ossia_access_mode.GET)]
    public float speed = 45;
    // No name: the name of the field
    [Ossia.Expose]
    public float gain = 1;
    [Ossia.Expose("size")]
    public Vector3 size = Vector3.one;
  }

  public class ExposeBaseFields : MonoBehaviour
  {
    [Ossia.Expose("level")]
    public float level = 0.5f;
  }

  // Its public fields include the ones of its base class
  public class ExposeDerivedFields : ExposeBaseFields
  {
  }

  // What [Ossia.Expose] publishes, read by a client in the tree of the
  // device: bounds, unit and access mode, a name by default, the fields of a
  // base class, no position unit on a vector that is not one; and the event
  // Received, for the values of a client only.
  public class ExposeOptionsTests
  {
    static int tests;
    int oscPort, wsPort;
    Controller controller;
    readonly List<GameObject> objects = new List<GameObject> ();

    IEnumerator Setup ()
    {
      for (GameObject old; (old = GameObject.Find ("OssiaController")) != null; )
        Object.DestroyImmediate (old);
      var go = new GameObject ("OssiaController");
      go.SetActive (false);
      controller = go.AddComponent<Controller> ();
      oscPort = 34234 + tests;
      wsPort = 34678 + tests;
      tests++;
      controller.oscPort = oscPort;
      controller.wsPort = wsPort;
      controller.resolveAnnouncedName = false;
      go.SetActive (true);
      objects.Add (go);
      yield return null;
    }

    T AddExposed<T> (string name) where T : Component
    {
      var go = new GameObject (name);
      objects.Add (go);
      var c = go.AddComponent<T> ();
      go.AddComponent<ExposeAttributes> ();
      return c;
    }

    [UnityTearDown]
    public IEnumerator Destroy ()
    {
      foreach (var go in objects)
        if (go != null)
          Object.DestroyImmediate (go);
      objects.Clear ();
      yield return null;
    }

    string Get (string path)
    {
      using (var c = new TcpClient ()) {
        c.Connect (IPAddress.Loopback, wsPort);
        c.ReceiveTimeout = 2000;
        var s = c.GetStream ();
        var req = Encoding.ASCII.GetBytes ("GET " + path + " HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n");
        s.Write (req, 0, req.Length);
        var sb = new StringBuilder ();
        var buf = new byte[65536];
        int n;
        while ((n = s.Read (buf, 0, buf.Length)) > 0)
          sb.Append (Encoding.UTF8.GetString (buf, 0, n));
        var all = sb.ToString ();
        Assert.IsTrue (all.StartsWith ("HTTP/1.1 200"), path + ": " + all.Split ('\n')[0]);
        return all.Substring (all.IndexOf ("\r\n\r\n") + 4);
      }
    }

    void Send (string address, float v)
    {
      var b = new List<byte> ();
      void Pad (byte[] s) { b.AddRange (s); b.AddRange (new byte[4 - s.Length % 4]); }
      Pad (Encoding.ASCII.GetBytes (address));
      Pad (Encoding.ASCII.GetBytes (",f"));
      var f = System.BitConverter.GetBytes (v);
      System.Array.Reverse (f);
      b.AddRange (f);
      var msg = b.ToArray ();
      using (var u = new UdpClient ())
        u.Send (msg, msg.Length, new IPEndPoint (IPAddress.Loopback, oscPort));
    }

    [UnityTest]
    public IEnumerator BoundsUnitAndAccess ()
    {
      yield return Setup ();
      AddExposed<ExposeOptionsFields> ("Opts");
      yield return null;
      var json = Get ("/Opts/Ossia.Tests.ExposeOptionsFields/speed");
      StringAssert.Contains ("\"ACCESS\":1", json);
      StringAssert.Contains ("angle.degree", json);
      StringAssert.Contains ("\"MIN\":0", json);
      StringAssert.Contains ("\"MAX\":360", json);
    }

    [UnityTest]
    public IEnumerator NameDefaultsToTheField ()
    {
      yield return Setup ();
      AddExposed<ExposeOptionsFields> ("Opts");
      yield return null;
      StringAssert.Contains ("/Opts/Ossia.Tests.ExposeOptionsFields/gain", Get ("/Opts/Ossia.Tests.ExposeOptionsFields/gain"));
      StringAssert.DoesNotContain ("ExposeOptionsFields/\"", Get ("/"));
    }

    [UnityTest]
    public IEnumerator FieldsOfABaseClassArePublished ()
    {
      yield return Setup ();
      AddExposed<ExposeDerivedFields> ("Derived");
      yield return null;
      StringAssert.Contains ("/Derived/Ossia.Tests.ExposeDerivedFields/level", Get ("/Derived/Ossia.Tests.ExposeDerivedFields/level"));
    }

    [UnityTest]
    public IEnumerator AVectorIsNotAPosition ()
    {
      yield return Setup ();
      AddExposed<ExposeOptionsFields> ("Opts");
      yield return null;
      StringAssert.DoesNotContain ("position", Get ("/Opts/Ossia.Tests.ExposeOptionsFields/size"));
      // The transform's position is one, published as EXTENDED_TYPE
      StringAssert.Contains ("position.cartesian", Get ("/Opts/transform/position"));
    }

    [UnityTest]
    public IEnumerator ExposeComponentsLogsNoLinePerProperty ()
    {
      yield return Setup ();
      int lines = 0;
      Application.LogCallback count = (text, trace, type) => {
        if (text.StartsWith ("Prop:"))
          lines++;
      };
      Application.logMessageReceived += count;
      try {
        var go = new GameObject ("Components");
        objects.Add (go);
        var expose = go.AddComponent<ExposeComponents> ();
        expose.components = new List<Component> { go.transform };
        yield return null;
        yield return null;
      } finally {
        Application.logMessageReceived -= count;
      }
      Assert.AreEqual (0, lines, "lines \"Prop: ...\" in the console");
    }

    [UnityTest]
    public IEnumerator ReceivedForTheValuesOfAClientOnly ()
    {
      yield return Setup ();
      var fields = AddExposed<ExposeOptionsFields> ("Opts");
      var expose = fields.GetComponent<ExposeAttributes> ();
      yield return null;
      var received = new List<string> ();
      expose.Received += (name, value) => received.Add (name + "=" + value);

      // Changed in Unity: published, not received
      for (int i = 0; i < 5; i++) {
        fields.gain = 2 + i;
        yield return null;
      }
      Assert.IsEmpty (received, "after changes made in Unity");

      Send ("/Opts/Ossia.Tests.ExposeOptionsFields/gain", 0.25f);
      float end = Time.realtimeSinceStartup + 2;
      while (received.Count == 0 && Time.realtimeSinceStartup < end)
        yield return null;
      Assert.AreEqual (new[] { "gain=0.25" }, received.ToArray ());
      Assert.AreEqual (0.25f, fields.gain);
    }

    // A client's value comes in after the object published its change and before
    // the end of the frame sends it: the object's value leaves after it, and is
    // the one the parameter and the clients hold. The queue reads [the client's,
    // the object's].
    [UnityTest]
    public IEnumerator AValueReplacedInTheSameFrameIsNotApplied ()
    {
      yield return Setup ();
      var fields = AddExposed<ExposeOptionsFields> ("Opts");
      var expose = fields.GetComponent<ExposeAttributes> ();
      yield return null;
      yield return null;
      var received = new List<string> ();
      expose.Received += (name, value) => received.Add (name + "=" + value);

      fields.gain = 2;
      // Back after the Update of the next frame, before its LateUpdate
      yield return null;
      Send ("/Opts/Ossia.Tests.ExposeOptionsFields/gain", 3f);
      System.Threading.Thread.Sleep (100);
      for (int i = 0; i < 5; i++)
        yield return null;
      Assert.AreEqual (2f, fields.gain, "the field holds what the parameter and the clients hold");
      Assert.IsEmpty (received);
    }
  }
}
