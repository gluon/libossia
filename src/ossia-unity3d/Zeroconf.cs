using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace Ossia
{
  internal static class Zeroconf
  {
    internal struct Announced
    {
      public string Name;
      public string Host;
      public int Port;
    }

    //! The OSCQuery devices announced over mDNS. Blocks for timeoutMs.
    internal static List<Announced> ListOscqueryDevices(int timeoutMs)
    {
      IntPtr names, hosts, ports;
      UIntPtr sz;
      Network.ossia_oscquery_list_devices (timeoutMs, out names, out hosts, out ports, out sz);

      int size = (int)sz.ToUInt64 ();
      var list = new List<Announced> (size);
      for (int i = 0; i < size; i++) {
        list.Add (new Announced {
          Name = Marshal.PtrToStringAnsi (Marshal.ReadIntPtr (names, i * IntPtr.Size)),
          Host = Marshal.PtrToStringAnsi (Marshal.ReadIntPtr (hosts, i * IntPtr.Size)),
          Port = Marshal.ReadInt32 (ports, i * sizeof (int))
        });
      }
      Network.ossia_oscquery_devices_free (names, hosts, ports, sz);
      return list;
    }

    //! The name a server of this machine listening on wsPort is announced
    //! under, null if none: the device announced on this host with that
    //! WebSocket port, as ossia-max's oscquery_directory does.
    internal static string AnnouncedName(IEnumerable<Announced> devices, string selfHost, int wsPort)
    {
      var self = HostLabel (selfHost);
      foreach (var d in devices) {
        if (d.Port == wsPort && HostLabel (d.Host) == self)
          return d.Name;
      }
      return null;
    }

    //! The first label of a host name, in lower case: "machine" for the
    //! SRV target "machine.local." as for the host name "Machine".
    internal static string HostLabel(string host)
    {
      if (host == null)
        return "";
      int dot = host.IndexOf ('.');
      return (dot < 0 ? host : host.Substring (0, dot)).ToLowerInvariant ();
    }
  }
}
