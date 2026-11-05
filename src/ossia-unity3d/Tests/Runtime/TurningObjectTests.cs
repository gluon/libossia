using System.Collections;
using System.Collections.Generic;
using System.Linq;
using System.Net;
using System.Net.Sockets;
using System.Reflection;
using System.Text;
using NUnit.Framework;
using UnityEngine;
using UnityEngine.TestTools;

namespace Ossia.Tests
{
  // The script of a turning object: an angle that grows at the speed a client
  // sets, and the rotation it gives the object. Step is its Update, called by
  // the test.
  public class Turning : MonoBehaviour
  {
    [Ossia.Expose("angle")]
    public float angle;
    [Ossia.Expose("speed")]
    public float speed;

    public void Step()
    {
      angle += speed / 60;
      transform.localRotation = Quaternion.Euler(20, angle, 0);
    }
  }

  // An object exposed with Send Updates that a script turns, while a client
  // sets its speed over OSC. The test plays the frames itself, in an order
  // Unity may choose for objects of the same execution order: the script,
  // ExposeAttributes, then the Controller, which reads its queue in Update
  // and sends the bundle of the frame in LateUpdate.
  public class TurningObjectTests
  {
    // Ports of their own for each test: a destroyed Controller does not free
    // its device.
    static int tests;
    int oscPort;

    Controller controller;
    ExposeAttributes expose;
    Turning turning;

    IEnumerator Setup()
    {
      for (GameObject old; (old = GameObject.Find("OssiaController")) != null; )
        Object.DestroyImmediate(old);

      var go = new GameObject("OssiaController");
      go.SetActive(false);
      controller = go.AddComponent<Controller>();
      oscPort = 33234 + tests;
      controller.oscPort = oscPort;
      controller.wsPort = 33678 + tests;
      tests++;
      controller.resolveAnnouncedName = false;
      go.SetActive(true);

      var obj = new GameObject("Turning");
      turning = obj.AddComponent<Turning>();
      expose = obj.AddComponent<ExposeAttributes>();
      // Published by Frame, in the order it chooses
      expose.sendUpdates = false;
      yield return null;
      yield return null;
      // Updated by Frame only
      controller.enabled = false;
    }

    [UnityTearDown]
    public IEnumerator Destroy()
    {
      if (turning != null)
        Object.DestroyImmediate(turning.gameObject);
      if (controller != null)
        Object.DestroyImmediate(controller.gameObject);
      yield return null;
    }

    void Call(string method)
    {
      typeof(Controller).GetMethod(method, BindingFlags.Instance | BindingFlags.NonPublic).Invoke(controller, null);
    }

    // The object publishes before the Controller reads its queue
    void Frame()
    {
      turning.Step();
      expose.SendUpdates();
      Call("Update");
      Call("LateUpdate");
    }

    IEnumerator ClientSetsSpeed(float v)
    {
      Send("Ossia.Tests.Turning/speed", v);
      float end = Time.realtimeSinceStartup + 2;
      while (turning.speed != v && Time.realtimeSinceStartup < end) {
        Frame();
        yield return null;
      }
      Assert.AreEqual(v, turning.speed, "speed set by the client");
    }

    [UnityTest]
    public IEnumerator AngleNeverGoesBack()
    {
      yield return Setup();
      yield return ClientSetsSpeed(45);

      float start = turning.angle;
      int back = 0;
      for (int i = 0; i < 120; i++) {
        turning.Step();
        float set = turning.angle;
        expose.SendUpdates();
        Call("Update");
        Call("LateUpdate");
        // The Controller set back a value the object published before
        if (turning.angle < set)
          back++;
        yield return null;
      }
      Assert.AreEqual(0, back, "frames where the Controller set the angle back");
      Assert.AreEqual(start + 120 * 45 / 60f, turning.angle, 1e-3f, "angle after 120 frames at 45 degrees per second");
      Assert.AreEqual(0f, Quaternion.Angle(Quaternion.Euler(20, turning.angle, 0), turning.transform.localRotation), 1e-2f, "rotation of the object");
    }

    // A client writes back, alone, a value of the transform as it read it a
    // few frames before: when it turns the object, the object takes back the
    // orientation that was read. A parameter that holds only a part of the
    // orientation (up, right) turns it to another one.
    [UnityTest]
    public IEnumerator TransformValueWrittenBackGivesTheOrientationRead()
    {
      yield return Setup();
      yield return ClientSetsSpeed(45);

      var t = turning.transform;
      var names = typeof(Transform).GetProperties()
        .Where(p => p.CanWrite && Node.FindNode(expose.child_node, "transform/" + p.Name) != null)
        .Select(p => p.Name).ToList();
      Assert.Contains("rotation", names);

      var turned = new List<string>();
      var wrong = new List<string>();
      foreach (var name in names) {
        for (int i = 0; i < 30; i++) {
          Frame();
          yield return null;
        }
        var read = t.localRotation;
        var value = typeof(Transform).GetProperty(name).GetValue(t, null);
        for (int i = 0; i < 30; i++) {
          Frame();
          yield return null;
        }
        // What the object published in the last frame: the client's value
        // comes after, in a frame of its own
        Call("Update");
        var before = t.localRotation;
        Assert.Greater(Quaternion.Angle(read, before), 5f, "the object turned since " + name + " was read");

        if (value is Vector3 v3)
          Send("transform/" + name, v3.x, v3.y, v3.z);
        else if (value is Quaternion q)
          Send("transform/" + name, q.x, q.y, q.z, q.w);
        var parameter = Node.FindNode(expose.child_node, "transform/" + name).GetParameter();
        float end = Time.realtimeSinceStartup + 2;
        while (!parameter.GetValue().Get(value.GetType()).Equals(value) && Time.realtimeSinceStartup < end)
          yield return null;
        Call("Update");

        if (Quaternion.Angle(before, t.localRotation) > 1e-2f) {
          turned.Add(name);
          if (Quaternion.Angle(read, t.localRotation) > 1e-2f)
            wrong.Add(name + " " + t.localRotation.eulerAngles + " for " + read.eulerAngles);
        }
      }
      Assert.Contains("rotation", turned);
      Assert.IsEmpty(wrong, "orientation after a value written back");
    }

    void Send(string path, params float[] values)
    {
      var b = new List<byte>();
      void Pad(byte[] s) { b.AddRange(s); b.AddRange(new byte[4 - s.Length % 4]); }
      Pad(Encoding.ASCII.GetBytes("/Turning/" + path));
      Pad(Encoding.ASCII.GetBytes("," + new string('f', values.Length)));
      foreach (var v in values) {
        var f = System.BitConverter.GetBytes(v);
        System.Array.Reverse(f);
        b.AddRange(f);
      }
      var msg = b.ToArray();
      using (var u = new UdpClient())
        u.Send(msg, msg.Length, new IPEndPoint(IPAddress.Loopback, oscPort));
    }
  }
}
