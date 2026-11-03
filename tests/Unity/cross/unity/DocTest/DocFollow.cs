// SPDX-FileCopyrightText: 2026 CNRS
// SPDX-FileContributor: Julien Bayle, PRISM <https://www.prism.cnrs.fr/>

// The "doc only" test of the sample "Follow a device" (doc_follow.py): open
// its scene and press Play, as the README says, with FollowADevice.maxpat open in Max, and
// check what the README says happens. Max is driven through cross.maxpat,
// whose observer follows follow_me as another client (OSC on 127.0.0.1:
// cross.maxpat listens on 7493, answers on 7496).
using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using System.Net;
using System.Net.Sockets;
using System.Text;
using NUnit.Framework;
using UnityEngine;
using UnityEngine.SceneManagement;
using UnityEngine.TestTools;

public class DocFollow
{
  static string Scene ()
  {
    return Environment.GetEnvironmentVariable ("DOC_FOLLOW_SCENE");
  }

  static byte[] Pad (byte[] b)
  {
    var n = (b.Length / 4 + 1) * 4;
    var r = new byte[n];
    Array.Copy (b, r, b.Length);
    return r;
  }

  static byte[] Osc (string address, params object[] args)
  {
    var tags = ",";
    var data = new List<byte> ();
    foreach (var a in args) {
      if (a is float f) {
        tags += "f";
        var b = BitConverter.GetBytes (f);
        if (BitConverter.IsLittleEndian) Array.Reverse (b);
        data.AddRange (b);
      } else {
        tags += "s";
        data.AddRange (Pad (Encoding.ASCII.GetBytes ((string)a)));
      }
    }
    return Pad (Encoding.ASCII.GetBytes (address)).Concat (Pad (Encoding.ASCII.GetBytes (tags))).Concat (data).ToArray ();
  }

  static void ToMax (string address, params object[] args)
  {
    using (var u = new UdpClient ()) {
      var b = Osc (address, args);
      u.Send (b, b.Length, new IPEndPoint (IPAddress.Loopback, 7493));
    }
  }

  static IEnumerator Until (Func<bool> done, float seconds)
  {
    float end = Time.realtimeSinceStartup + seconds;
    while (!done () && Time.realtimeSinceStartup < end)
      yield return null;
  }

  static Ossia.Client FindClient ()
  {
    var go = GameObject.Find ("OssiaClient");
    return go != null ? go.GetComponent<Ossia.Client> () : null;
  }

  // As the README says: open the scene, press Play
  [UnityTest]
  public IEnumerator PlayTheSceneWithThePatchOpen ()
  {
    UnityEditor.SceneManagement.EditorSceneManager.OpenScene (Scene ());
    yield return new EnterPlayMode ();
    // Created after the domain reload of Play: what its lambdas capture lives
    yield return InPlay ();
    yield return new ExitPlayMode ();
  }

  IEnumerator InPlay ()
  {
    Debug.Log ("DOC playing " + SceneManager.GetActiveScene ().path);
    Ossia.Client client = null;
    yield return Until (() => (client = FindClient ()) != null, 5);
    Assert.IsNotNull (client, "the scene has its OssiaClient");
    yield return Until (() => client.Status == Ossia.ClientStatus.Connected, 15);
    Assert.AreEqual (Ossia.ClientStatus.Connected, client.Status, client.LastError);
    yield return null;

    // README: the console shows OSSIA client: connected to ws://<machine>.local:9678 (follow_me)
    var log = System.IO.File.ReadAllText (Environment.GetEnvironmentVariable ("DOC_FOLLOW_LOG"));
    var line = log.Split ('\n').Select (l => l.Trim ()).FirstOrDefault (l => l.StartsWith ("OSSIA client: connected to"));
    Debug.Log ("DOC console: " + line);
    Assert.IsNotNull (line, "the console line of the README");
    StringAssert.StartsWith ("OSSIA client: connected to ws://", line);
    StringAssert.EndsWith (":9678 (follow_me)", line);
    // README: the Inspector of OssiaClient reads Connected to ws://...
    var status = new UnityEditor.SerializedObject (client).FindProperty ("status").stringValue;
    Debug.Log ("DOC Inspector status: " + status);
    StringAssert.StartsWith ("Connected to ws://", status);

    // README: in Max, change size and spin, the cube follows
    var cube = GameObject.Find ("Cube");
    var follow = cube.GetComponent ("FollowADevice");
    var spinField = follow.GetType ().GetField ("spin");
    ToMax ("/cross/connect", "observer", "oscquery", "ws://127.0.0.1:9678");
    yield return new WaitForSecondsRealtime (1.5f);
    ToMax ("/cross/put", "observer", "size", 2f);
    ToMax ("/cross/put", "observer", "spin", 180f);
    yield return Until (() => Mathf.Abs (cube.transform.localScale.x - 2f) < 1e-3f && (float)spinField.GetValue (follow) == 180f, 5);
    Debug.Log ("DOC after size 2. and spin 180. in Max: scale " + cube.transform.localScale + ", spin " + spinField.GetValue (follow));
    Assert.AreEqual (2f, cube.transform.localScale.x, 1e-3f, "the cube takes the size of Max");
    Assert.AreEqual (180f, (float)spinField.GetValue (follow), "the cube takes the spin of Max");

    // README: the number under angle follows the cube as it turns
    using (var replies = new UdpClient (new IPEndPoint (IPAddress.Loopback, 7496))) {
      replies.Client.ReceiveTimeout = 2000;
      ToMax ("/cross/count", "observer", "angle");
      IPEndPoint from = null;
      var first = Encoding.ASCII.GetString (replies.Receive (ref from));
      yield return new WaitForSecondsRealtime (1f);
      ToMax ("/cross/count", "observer", "angle");
      var second = Encoding.ASCII.GetString (replies.Receive (ref from));
      Debug.Log ("DOC angle in Max: " + first.Replace ('\0', ' ').Trim () + " then " + second.Replace ('\0', ' ').Trim ());
      Assert.AreNotEqual (first, second, "the angle Max receives moves");
    }
    ToMax ("/cross/disconnect", "observer");
  }
}
