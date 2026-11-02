using System;
using System.Collections;
using System.Collections.Concurrent;
using System.Linq;
using System.Runtime.InteropServices;
using NUnit.Framework;
using UnityEngine;
using UnityEngine.TestTools;

namespace Ossia.Tests
{
  public class ConcurrentFields : MonoBehaviour
  {
    [Ossia.Expose("value")]
    public float value = 0;
  }

  // A client and Unity write the same address (D-0042, acte 13): a value
  // received is applied, every local change leaves whatever its source
  // (script, Inspector, animation), and a value received does not leave
  // again. The client is an OSCQuery mirror of libossia, the core ossia-max
  // uses, in this process: it follows the address as ossia.remote does.
  public class ConcurrentWriteTests
  {
    const int OscPort = 28234;
    const int WsPort = 28678;
    // Values each writer uses: what the client receives tells who wrote it
    const float ClientBase = 1000, ScriptBase = 2000, InspectorValue = 3000;

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
    static readonly ConcurrentQueue<float> received = new ConcurrentQueue<float>();

    // The callback also runs, on the thread that pushes, when the mirror
    // pushes its own value: only what a network thread delivers is received.
    static int pushingThread;

    [AOT.MonoPInvokeCallback(typeof(ValueCallback))]
    static void OnValue(IntPtr ctx, IntPtr value)
    {
      if (System.Threading.Thread.CurrentThread.ManagedThreadId != pushingThread)
        received.Enqueue(ossia_value_to_float(value));
    }

    GameObject controllerObject, shape;
    ConcurrentFields fields;
    IntPtr mirror, param, callback;

    IEnumerator Setup()
    {
      pushingThread = System.Threading.Thread.CurrentThread.ManagedThreadId;
      while (received.TryDequeue(out _)) { }
      for (GameObject old; (old = GameObject.Find("OssiaController")) != null; )
        UnityEngine.Object.DestroyImmediate(old);

      controllerObject = new GameObject("OssiaController");
      controllerObject.SetActive(false);
      var c = controllerObject.AddComponent<Controller>();
      c.deviceName = "ossia_unity_concurrent_test";
      c.oscPort = OscPort;
      c.wsPort = WsPort;
      c.resolveAnnouncedName = false;
      controllerObject.SetActive(true);

      shape = new GameObject("Shape");
      fields = shape.AddComponent<ConcurrentFields>();
      shape.AddComponent<ExposeAttributes>();   // Send Updates on by default
      yield return null;
      yield return null;

      mirror = ossia_device_create(ossia_protocol_oscquery_mirror_create("ws://127.0.0.1:" + WsPort), "ossia_unity_concurrent_mirror");
      Assert.AreNotEqual(IntPtr.Zero, mirror, "mirror");
      Assert.AreEqual(1, ossia_device_update_namespace(mirror), "tree read");
      var node = ossia_node_find(ossia_device_get_root_node(mirror), "/Shape/" + typeof(ConcurrentFields).ToString() + "/value");
      Assert.AreNotEqual(IntPtr.Zero, node, "remote node");
      param = ossia_node_get_parameter(node);
      // A callback makes the mirror listen to the address
      callback = ossia_parameter_add_callback(param, Marshal.GetFunctionPointerForDelegate(onValue), IntPtr.Zero);
      yield return new WaitForSecondsRealtime(0.3f);
      while (received.TryDequeue(out _)) { }
    }

    [UnityTearDown]
    public IEnumerator Destroy()
    {
      if (param != IntPtr.Zero && callback != IntPtr.Zero)
        ossia_parameter_remove_callback(param, callback);
      if (mirror != IntPtr.Zero)
        ossia_device_free(mirror);
      mirror = param = callback = IntPtr.Zero;
      if (shape != null)
        UnityEngine.Object.DestroyImmediate(shape);
      if (controllerObject != null)
        UnityEngine.Object.DestroyImmediate(controllerObject);
      yield return null;
    }

    static IEnumerator Until(Func<bool> done, float seconds)
    {
      float end = Time.realtimeSinceStartup + seconds;
      while (!done() && Time.realtimeSinceStartup < end)
        yield return null;
    }

    static float[] Drain()
    {
      var l = new System.Collections.Generic.List<float>();
      while (received.TryDequeue(out var v))
        l.Add(v);
      return l.ToArray();
    }

    [UnityTest]
    public IEnumerator ClientAndScriptWriteTheSameAddressFor5Seconds()
    {
      yield return Setup();
      var all = new System.Collections.Generic.List<float>();

      // Each frame, one of the two writes, for 5 s
      int clientWrites = 0, scriptWrites = 0;
      float end = Time.realtimeSinceStartup + 5;
      for (int i = 0; Time.realtimeSinceStartup < end; i++) {
        if (i % 2 == 0)
          ossia_parameter_push_f(param, ClientBase + (clientWrites++ % 900));
        else
          fields.value = ScriptBase + (scriptWrites++ % 900);
        yield return new WaitForSecondsRealtime(1f / 120);
        all.AddRange(Drain());
      }

      // The client writes last: Unity takes its value
      ossia_parameter_push_f(param, ClientBase + 950);
      yield return Until(() => fields.value == ClientBase + 950, 2);
      Assert.AreEqual(ClientBase + 950, fields.value, "Unity converges on the client's last value");

      // Then the script writes last: the client gets it
      fields.value = ScriptBase + 950;
      yield return Until(() => received.Contains(ScriptBase + 950), 2);
      all.AddRange(Drain());
      Assert.Contains(ScriptBase + 950, all, "the client receives the script's last value");

      // Nothing more moves: no oscillation
      yield return new WaitForSecondsRealtime(1);
      var after = Drain();
      all.AddRange(after);

      var echoes = all.Count(v => v >= ClientBase && v < ScriptBase);
      var fromScript = all.Count(v => v >= ScriptBase && v < InspectorValue);
      Debug.Log("CONCURRENT clientWrites=" + clientWrites + " scriptWrites=" + scriptWrites
        + " received=" + all.Count + " fromScript=" + fromScript + " echoes=" + echoes + " afterStop=" + after.Length);
      Assert.AreEqual(0, echoes, "values the client wrote came back to it");
      Assert.AreEqual(0, after.Length, "messages after both stopped");
      Assert.LessOrEqual(all.Count, scriptWrites + 1, "the client receives at most one message per change in Unity");
    }

    // Every source of a change in Unity publishes.
    [UnityTest]
    public IEnumerator InspectorAndAnimationPublish()
    {
      yield return Setup();

#if UNITY_EDITOR
      var so = new UnityEditor.SerializedObject(fields);
      so.FindProperty("value").floatValue = InspectorValue;
      so.ApplyModifiedProperties();
      yield return Until(() => received.Contains(InspectorValue), 2);
      Assert.Contains(InspectorValue, Drain(), "Inspector");
#endif

      var clip = new AnimationClip { legacy = true };
      clip.SetCurve("", typeof(ConcurrentFields), "value", AnimationCurve.Linear(0, 0, 0.5f, 1));
      var animation = shape.AddComponent<Animation>();
      animation.AddClip(clip, "ramp");
      animation.Play("ramp");
      yield return new WaitForSecondsRealtime(0.8f);
      animation.Stop();
      yield return new WaitForSecondsRealtime(0.2f);
      var ramp = Drain().Where(v => v >= 0 && v <= 1).ToArray();
      Debug.Log("CONCURRENT animation values received=" + ramp.Length + " last=" + (ramp.Length > 0 ? ramp.Last() : -1));
      Assert.Greater(ramp.Length, 1, "animation");
    }
  }
}
