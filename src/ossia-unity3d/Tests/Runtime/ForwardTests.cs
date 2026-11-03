using System;
using System.Collections;
using System.Collections.Concurrent;
using System.Linq;
using System.Runtime.InteropServices;
using System.Threading;
using NUnit.Framework;
using UnityEngine;
using UnityEngine.TestTools;

namespace Ossia.Tests
{
  public class ForwardFields : MonoBehaviour
  {
    [Ossia.Expose("value")]
    public float value = 0;
  }

  // Two clients of the Controller see each other: what A writes reaches
  // Unity and B, once, and never comes back to A. The clients are libossia
  // OSCQuery mirrors, the core of ossia.remote, in this process.
  public class ForwardTests
  {
    const int OscPort = 32234;
    const int WsPort = 32678;

    [DllImport("ossia")] static extern IntPtr ossia_protocol_oscquery_mirror_create(string host);
    [DllImport("ossia")] static extern IntPtr ossia_device_create(IntPtr protocol, string name);
    [DllImport("ossia")] static extern void ossia_device_free(IntPtr device);
    [DllImport("ossia")] static extern int ossia_device_update_namespace(IntPtr device);
    [DllImport("ossia")] static extern IntPtr ossia_device_get_root_node(IntPtr device);
    [DllImport("ossia")] static extern IntPtr ossia_node_find(IntPtr node, string path);
    [DllImport("ossia")] static extern IntPtr ossia_node_get_parameter(IntPtr node);
    [DllImport("ossia")] static extern IntPtr ossia_parameter_add_callback(IntPtr param, IntPtr callback, IntPtr ctx);
    [DllImport("ossia")] static extern void ossia_parameter_remove_callback(IntPtr param, IntPtr index);
    [DllImport("ossia")] static extern void ossia_parameter_push_f(IntPtr param, float value);
    [DllImport("ossia")] static extern float ossia_value_to_float(IntPtr value);

    delegate void ValueCallback(IntPtr ctx, IntPtr value);
    static readonly ValueCallback onValue = OnValue;
    // What each mirror receives from the network, by its index in ctx
    static readonly ConcurrentQueue<float>[] received = { new ConcurrentQueue<float>(), new ConcurrentQueue<float>() };
    static int mainThread;

    [AOT.MonoPInvokeCallback(typeof(ValueCallback))]
    static void OnValue(IntPtr ctx, IntPtr value)
    {
      // The mirror's own pushes run their callbacks on the main thread
      if (Thread.CurrentThread.ManagedThreadId != mainThread)
        received[(int)ctx].Enqueue(ossia_value_to_float(value));
    }

    GameObject controllerObject, shape;
    Controller controller;
    ForwardFields fields;
    readonly IntPtr[] mirrors = new IntPtr[2], parameters = new IntPtr[2], callbacks = new IntPtr[2];

    IEnumerator Setup(bool forward)
    {
      mainThread = Thread.CurrentThread.ManagedThreadId;
      foreach (var q in received)
        while (q.TryDequeue(out _)) { }
      for (GameObject old; (old = GameObject.Find("OssiaController")) != null; )
        UnityEngine.Object.DestroyImmediate(old);

      controllerObject = new GameObject("OssiaController");
      controllerObject.SetActive(false);
      controller = controllerObject.AddComponent<Controller>();
      controller.deviceName = "ossia_unity_forward_test";
      controller.oscPort = OscPort;
      controller.wsPort = WsPort;
      controller.resolveAnnouncedName = false;
      controller.forwardToClients = forward;
      controllerObject.SetActive(true);

      shape = new GameObject("Shape");
      fields = shape.AddComponent<ForwardFields>();
      shape.AddComponent<ExposeAttributes>();
      yield return null;
      yield return null;

      for (int i = 0; i < 2; i++) {
        mirrors[i] = ossia_device_create(ossia_protocol_oscquery_mirror_create("ws://127.0.0.1:" + WsPort), "ossia_unity_forward_mirror_" + i);
        Assert.AreNotEqual(IntPtr.Zero, mirrors[i], "mirror " + i);
        Assert.AreEqual(1, ossia_device_update_namespace(mirrors[i]), "tree read by " + i);
        var node = ossia_node_find(ossia_device_get_root_node(mirrors[i]), "/Shape/" + typeof(ForwardFields).ToString() + "/value");
        Assert.AreNotEqual(IntPtr.Zero, node, "remote node " + i);
        parameters[i] = ossia_node_get_parameter(node);
        callbacks[i] = ossia_parameter_add_callback(parameters[i], Marshal.GetFunctionPointerForDelegate(onValue), (IntPtr)i);
      }
      yield return new WaitForSecondsRealtime(0.3f);
      foreach (var q in received)
        while (q.TryDequeue(out _)) { }
    }

    [UnityTearDown]
    public IEnumerator Destroy()
    {
      for (int i = 0; i < 2; i++) {
        if (parameters[i] != IntPtr.Zero && callbacks[i] != IntPtr.Zero)
          ossia_parameter_remove_callback(parameters[i], callbacks[i]);
        if (mirrors[i] != IntPtr.Zero)
          ossia_device_free(mirrors[i]);
        mirrors[i] = parameters[i] = callbacks[i] = IntPtr.Zero;
      }
      if (shape != null)
        UnityEngine.Object.DestroyImmediate(shape);
      if (controllerObject != null)
        UnityEngine.Object.DestroyImmediate(controllerObject);
      yield return null;
    }

    static float[] Drain(int i)
    {
      var l = new System.Collections.Generic.List<float>();
      while (received[i].TryDequeue(out var v))
        l.Add(v);
      return l.ToArray();
    }

    static IEnumerator Until(Func<bool> done, float seconds)
    {
      float end = Time.realtimeSinceStartup + seconds;
      while (!done() && Time.realtimeSinceStartup < end)
        yield return null;
    }

    [UnityTest]
    public IEnumerator TwoClientsSeeEachOther()
    {
      yield return Setup(true);

      ossia_parameter_push_f(parameters[0], 0.25f);
      yield return Until(() => fields.value == 0.25f && received[1].Contains(0.25f), 2);
      yield return new WaitForSecondsRealtime(0.5f);
      var a = Drain(0);
      var b = Drain(1);
      Debug.Log("FORWARD A wrote 0.25: Unity=" + fields.value + " B received=[" + string.Join(" ", b) + "] A received=[" + string.Join(" ", a) + "]");
      Assert.AreEqual(0.25f, fields.value, "Unity applies A's value");
      Assert.AreEqual(new[] { 0.25f }, b, "B receives A's value, once");
      Assert.IsEmpty(a, "A does not receive its own value");

      // And the other way, while Unity also changes the field
      ossia_parameter_push_f(parameters[1], 0.5f);
      yield return Until(() => received[0].Contains(0.5f), 2);
      fields.value = 0.75f;
      yield return Until(() => received[0].Contains(0.75f) && received[1].Contains(0.75f), 2);
      yield return new WaitForSecondsRealtime(0.5f);
      a = Drain(0);
      b = Drain(1);
      Debug.Log("FORWARD B wrote 0.5, Unity 0.75: A received=[" + string.Join(" ", a) + "] B received=[" + string.Join(" ", b) + "]");
      Assert.AreEqual(new[] { 0.5f, 0.75f }, a, "A receives B's value, then Unity's");
      Assert.AreEqual(new[] { 0.75f }, b, "B receives Unity's value, not its own");
    }

    [UnityTest]
    public IEnumerator ForwardingOff()
    {
      yield return Setup(false);

      ossia_parameter_push_f(parameters[0], 0.25f);
      yield return Until(() => fields.value == 0.25f, 2);
      yield return new WaitForSecondsRealtime(0.5f);
      Assert.AreEqual(0.25f, fields.value, "Unity applies A's value");
      Assert.IsEmpty(Drain(1), "B receives nothing");
      Assert.IsEmpty(Drain(0), "A receives nothing");
    }
  }
}
