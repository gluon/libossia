using System;
using System.Collections;
using System.Diagnostics;
using System.Text.RegularExpressions;
using NUnit.Framework;
using UnityEngine;
using UnityEngine.TestTools;

namespace Ossia.Tests
{
  // The Controller reads the name its device is announced under over mDNS:
  // its own, or "name (2)" when another device of the network has it.
  // The other device is registered with dns-sd (macOS) on this machine,
  // on another port.
  [UnityPlatform(RuntimePlatform.OSXEditor, RuntimePlatform.OSXPlayer)]
  public class AnnouncedNameTests
  {
    Process other;

    static IEnumerator StartController(string name, int osc, int ws, Action<Controller> done)
    {
      // Not named OssiaController: the exposed objects of the other tests
      // look up their Controller by that name.
      var go = new GameObject("MdnsController");
      go.SetActive(false);
      var c = go.AddComponent<Controller>();
      c.deviceName = name;
      c.oscPort = osc;
      c.wsPort = ws;
      go.SetActive(true);

      float end = Time.realtimeSinceStartup + 20;
      while (c.AnnouncedName == null && Time.realtimeSinceStartup < end)
        yield return null;
      done(c);
    }

    [UnityTest]
    public IEnumerator AnnouncedUnderItsName()
    {
      const string name = "ossia_unity_name_test";
      Controller c = null;
      yield return StartController(name, 41234, 45678, x => c = x);
      Assert.AreEqual(name, c.AnnouncedName);
      LogAssert.Expect(LogType.Log, "OSSIA: announced \"" + name + "\" over mDNS (_oscjson._tcp)");
    }

    [UnityTest]
    public IEnumerator SecondDeviceOfTheSameNameReadsItsNewName()
    {
      const string name = "ossia_unity_collision_test";
      other = Process.Start("/usr/bin/dns-sd", "-R " + name + " _oscjson._tcp local 45992");
      yield return new WaitForSecondsRealtime(1.5f);

      Controller c = null;
      yield return StartController(name, 41235, 45679, x => c = x);
      Assert.AreEqual(name + " (2)", c.AnnouncedName);
      LogAssert.Expect(LogType.Warning, new Regex("announced as \"" + Regex.Escape(name) + " \\(2\\)\": the name \"" + name + "\" is taken on the network"));
    }

    [TearDown]
    public void StopOther()
    {
      if (other != null && !other.HasExited)
        other.Kill();
      other = null;
    }
  }
}
