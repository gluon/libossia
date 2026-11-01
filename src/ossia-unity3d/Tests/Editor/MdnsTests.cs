using System;
using System.Collections;
using System.Diagnostics;
using System.IO;
using System.Linq;
using NUnit.Framework;
using UnityEngine;
using UnityEngine.TestTools;
using Debug = UnityEngine.Debug;

namespace Ossia.Tests
{
  // In Play the device is announced over mDNS (_oscjson._tcp) under its
  // name, and Stop withdraws the announcement. dns-sd, the macOS client of
  // mDNSResponder, browses the service during the test; its lines carry
  // their own time of day. Times go through files: the domain reloads on
  // each Play and each Stop.
  [UnityPlatform(RuntimePlatform.OSXEditor)]
  public class MdnsTests
  {
    const string Name = "ossia_unity_mdns_test";
    static string Dir => Path.Combine(Path.GetTempPath(), "ossia-unity-mdns-test");
    static string Browse => Path.Combine(Dir, "browse.txt");
    static string Pid => Path.Combine(Dir, "browse.pid");
    static string PlayTime => Path.Combine(Dir, "play.txt");
    static string StopTime => Path.Combine(Dir, "stop.txt");

    [UnityTest]
    public IEnumerator PlayAnnouncesStopWithdraws()
    {
      Directory.CreateDirectory(Dir);
      File.Delete(Browse);
      File.Delete(PlayTime);
      File.Delete(StopTime);
      Process.Start("/bin/sh", $"-c \"/usr/bin/dns-sd -B _oscjson._tcp > '{Browse}' 2>&1 & echo $! > '{Pid}'\"").WaitForExit();

      var go = new GameObject("OssiaController");
      var c = go.AddComponent<Controller>();
      c.deviceName = Name;
      c.oscPort = 21234;
      c.wsPort = 25678;

      Save(PlayTime);
      yield return new EnterPlayMode();
      var deadline = DateTime.Now.AddSeconds(5);
      while (Line("Add") == null && DateTime.Now < deadline)
        yield return null;
      var add = Line("Add");
      Assert.IsNotNull(add, "Play: " + Name + " is announced on _oscjson._tcp");
      Debug.Log($"MDNS announced {Ms(When(add) - Load(PlayTime))} ms after Play was requested: {add}");

      Save(StopTime);
      yield return new ExitPlayMode();
      var stopped = DateTime.Now.TimeOfDay;

      deadline = DateTime.Now.AddSeconds(10);
      while (Line("Rmv") == null && DateTime.Now < deadline)
        yield return null;
      var rmv = Line("Rmv");
      Assert.IsNotNull(rmv, "Stop: the announcement of " + Name + " is withdrawn");
      Debug.Log($"MDNS withdrawn {Ms(When(rmv) - Load(StopTime))} ms after Stop was requested, "
          + $"{Ms(When(rmv) - stopped)} ms from the end of Stop: {rmv}");
    }

    static void Save(string path) { File.WriteAllText(path, DateTime.Now.TimeOfDay.Ticks.ToString()); }
    static TimeSpan Load(string path) { return new TimeSpan(long.Parse(File.ReadAllText(path))); }

    [TearDown]
    public void StopBrowsing()
    {
      if (File.Exists(Pid))
        Process.Start("/bin/kill", File.ReadAllText(Pid).Trim()).WaitForExit();
      var go = GameObject.Find("OssiaController");
      if (go != null)
        UnityEngine.Object.DestroyImmediate(go);
    }

    // First line of dns-sd for this instance, Add or Rmv. The instance name
    // ends the line: "12:37:06.788  Add  3  1 local.  _oscjson._tcp.  name"
    static string Line(string kind)
    {
      if (!File.Exists(Browse))
        return null;
      string[] lines;
      using (var f = new FileStream(Browse, FileMode.Open, FileAccess.Read, FileShare.ReadWrite))
      using (var r = new StreamReader(f))
        lines = r.ReadToEnd().Split('\n');
      return lines.FirstOrDefault(l => l.Contains(" " + kind + " ") && l.TrimEnd().EndsWith(" " + Name));
    }

    static TimeSpan When(string line) { return TimeSpan.Parse(line.Substring(0, 12)); }
    static int Ms(TimeSpan t) { return (int)t.TotalMilliseconds; }
  }
}
