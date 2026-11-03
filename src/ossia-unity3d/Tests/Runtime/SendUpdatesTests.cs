using System.Collections;
using System.Reflection;
using NUnit.Framework;
using UnityEngine;
using UnityEngine.TestTools;

namespace Ossia.Tests
{
  public class SendUpdatesFields : MonoBehaviour
  {
    [Ossia.Expose("flag")]
    public bool flag = true;
    [Ossia.Expose("speed")]
    public float speed = 45;
  }

  // Send Updates on: what ExposeAttributes publishes comes back through the
  // Controller's message queue, which receives the local pushes too. Unity
  // calls the Update of the Controller, of ExposeAttributes and of the script
  // that moves the object in an order of its own; when the script runs between
  // the publication and the Controller, the Controller must not set back the
  // value published before the script changed it.
  public class SendUpdatesTests
  {
    // Ports of their own for each test, as the other tests of the package:
    // a destroyed Controller does not free its device.
    static int tests;

    Controller controller;
    ExposeAttributes expose;
    SendUpdatesFields fields;
    Parameter flag;

    IEnumerator Setup()
    {
      // ExposeAttributes registers with the first object named
      // OssiaController: the one an earlier test may have left.
      for (GameObject old; (old = GameObject.Find("OssiaController")) != null; )
        Object.DestroyImmediate(old);

      var go = new GameObject("OssiaController");
      go.SetActive(false);
      controller = go.AddComponent<Controller>();
      controller.oscPort = 21234 + tests;
      controller.wsPort = 25678 + tests;
      tests++;
      controller.resolveAnnouncedName = false;
      go.SetActive(true);

      var shape = new GameObject("Shape");
      fields = shape.AddComponent<SendUpdatesFields>();
      expose = shape.AddComponent<ExposeAttributes>();
      expose.sendUpdates = true;
      yield return null;
      yield return null;

      flag = Find("flag");
      Assert.IsNotNull(flag);
    }

    // From the node of this test's object: the objects of the tests before
    // are still in the tree, as Shape.1, Shape.2...
    Parameter Find(string name)
    {
      return Node.FindNode(expose.child_node, typeof(SendUpdatesFields).ToString() + "/" + name).GetParameter();
    }

    // What Unity calls once a frame on the Controller: the messages received
    // are applied to the fields and properties.
    void ControllerUpdate()
    {
      typeof(Controller).GetMethod("Update", BindingFlags.Instance | BindingFlags.NonPublic).Invoke(controller, null);
    }

    // What the object and the Controller do in a frame: the object publishes,
    // the Controller sends the bundle at the end of the frame.
    void Publish()
    {
      expose.SendUpdates();
      typeof(Controller).GetMethod("LateUpdate", BindingFlags.Instance | BindingFlags.NonPublic).Invoke(controller, null);
    }

    // The next test looks up its Controller by name: this one must be gone.
    [UnityTearDown]
    public IEnumerator Destroy()
    {
      if (fields != null)
        Object.DestroyImmediate(fields.gameObject);
      if (controller != null)
        Object.DestroyImmediate(controller.gameObject);
      yield return null;
    }

    [UnityTest]
    public IEnumerator BoolFieldKeepsTheValueSetInUnity()
    {
      yield return Setup();
      Assert.IsTrue(fields.flag);

      foreach (var v in new[] { false, true, false }) {
        fields.flag = v;
        for (int i = 0; i < 10; i++)
          yield return null;
        Assert.AreEqual(v, fields.flag, "field after 10 frames");
        Assert.AreEqual(v, flag.GetValue().GetBool(), "parameter after 10 frames");
      }
    }

    [UnityTest]
    public IEnumerator FieldChangedAfterThePublicationIsKept()
    {
      yield return Setup();

      fields.speed = 10;
      Publish();              // publishes 10
      fields.speed = 20;      // the script changes it again
      ControllerUpdate();     // receives the publication of 10
      Assert.AreEqual(20f, fields.speed);
    }

    [UnityTest]
    public IEnumerator RotationAfterThePublicationIsKept()
    {
      yield return Setup();
      var t = fields.transform;

      for (int i = 0; i < 10; i++) {
        Publish();
        t.Rotate(Vector3.up, 5);
        var rotated = t.rotation;
        ControllerUpdate();
        Assert.AreEqual(0f, Quaternion.Angle(rotated, t.rotation), 1e-3f, "step " + i);
      }
    }

    // A value received from a client still reaches the field, also when it
    // is the value published before.
    [UnityTest]
    public IEnumerator ValueFromAClientIsApplied()
    {
      yield return Setup();
      var speed = Find("speed");

      fields.speed = 10;
      Publish();
      ControllerUpdate();

      speed.PushValue(30f);   // as a client sets it
      ControllerUpdate();
      Assert.AreEqual(30f, fields.speed);

      expose.sendUpdates = false;
      speed.PushValue(10f);   // back to the value published before
      ControllerUpdate();
      Assert.AreEqual(10f, fields.speed);
    }
  }
}
