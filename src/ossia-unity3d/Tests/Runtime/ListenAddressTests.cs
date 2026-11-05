using System;
using System.Collections;
using System.Net;
using System.Net.NetworkInformation;
using System.Net.Sockets;
using System.Runtime.InteropServices;
using NUnit.Framework;
using UnityEngine;
using UnityEngine.TestTools;

namespace Ossia.Tests
{
  // A Controller on 127.0.0.1 only: the clients of the machine reach it, the
  // other interfaces refuse its WebSocket, and it is not announced over mDNS.
  public class ListenAddressTests
  {
    const int OscPort = 35234;
    const int WsPort = 35678;
    const string Name = "unity_listen_address_test";

    [DllImport ("ossia")]
    static extern void ossia_oscquery_list_devices (int timeout_ms, out IntPtr names, out IntPtr hosts, out IntPtr ports, out UIntPtr size);
    [DllImport ("ossia")]
    static extern void ossia_oscquery_devices_free (IntPtr names, IntPtr hosts, IntPtr ports, UIntPtr size);

    static bool Announced (string name, int ms)
    {
      ossia_oscquery_list_devices (ms, out var names, out var hosts, out var ports, out var size);
      bool found = false;
      for (int i = 0; i < (int)size.ToUInt64 (); i++)
        found |= Marshal.PtrToStringAnsi (Marshal.ReadIntPtr (names, i * IntPtr.Size)) == name;
      ossia_oscquery_devices_free (names, hosts, ports, size);
      return found;
    }

    static IPAddress OtherIPv4 ()
    {
      foreach (var ni in NetworkInterface.GetAllNetworkInterfaces ()) {
        if (ni.OperationalStatus != OperationalStatus.Up || ni.NetworkInterfaceType == NetworkInterfaceType.Loopback)
          continue;
        foreach (var a in ni.GetIPProperties ().UnicastAddresses)
          if (a.Address.AddressFamily == AddressFamily.InterNetwork && !IPAddress.IsLoopback (a.Address))
            return a.Address;
      }
      return null;
    }

    static bool Connects (IPAddress ip, int port)
    {
      using (var c = new TcpClient ()) {
        try {
          return c.ConnectAsync (ip, port).Wait (500) && c.Connected;
        } catch (Exception) {
          return false;
        }
      }
    }

    [UnityTest]
    public IEnumerator OnTheLoopbackOnly ()
    {
      var other = OtherIPv4 ();
      if (other == null)
        Assert.Ignore ("no interface other than the loopback");
      for (GameObject old; (old = GameObject.Find ("OssiaController")) != null; )
        UnityEngine.Object.DestroyImmediate (old);

      var go = new GameObject ("OssiaController");
      go.SetActive (false);
      var c = go.AddComponent<Controller> ();
      c.deviceName = Name;
      c.oscPort = OscPort;
      c.wsPort = WsPort;
      c.listenAddress = "127.0.0.1";
      c.resolveAnnouncedName = false;
      go.SetActive (true);
      yield return null;
      try {
        Assert.IsTrue (Connects (IPAddress.Loopback, WsPort), "on 127.0.0.1");
        Assert.IsFalse (Connects (other, WsPort), "through another interface");
        Assert.IsFalse (Announced (Name, 2500), "announced over mDNS");
      } finally {
        UnityEngine.Object.DestroyImmediate (go);
      }
    }
  }
}
