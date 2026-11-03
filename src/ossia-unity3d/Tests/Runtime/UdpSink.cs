using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Threading;

namespace Ossia.Tests
{
  // The datagrams sent to 127.0.0.1 on a port a server opened on 0.0.0.0
  class UdpSink : IDisposable
  {
    readonly Socket socket;
    readonly Thread thread;
    volatile bool running = true;
    public readonly ConcurrentQueue<int> Messages = new ConcurrentQueue<int> ();

    public UdpSink (int port)
    {
      socket = new Socket (AddressFamily.InterNetwork, SocketType.Dgram, ProtocolType.Udp);
      socket.SetSocketOption (SocketOptionLevel.Socket, SocketOptionName.ReuseAddress, true);
      socket.Bind (new IPEndPoint (IPAddress.Loopback, port));
      socket.ReceiveTimeout = 50;
      thread = new Thread (() => {
        var buf = new byte[65536];
        while (running) {
          try {
            int n = socket.Receive (buf);
            Messages.Enqueue (CountMessages (buf, n));
          } catch (SocketException) {
          }
        }
      });
      thread.IsBackground = true;
      thread.Start ();
    }

    public int Port { get { return ((IPEndPoint)socket.LocalEndPoint).Port; } }

    // Messages in one datagram: one, or the elements of a bundle
    static int CountMessages (byte[] b, int n)
    {
      if (n < 16 || Encoding.ASCII.GetString (b, 0, 7) != "#bundle")
        return 1;
      int count = 0;
      for (int i = 16; i + 4 <= n; ) {
        int size = (b [i] << 24) | (b [i + 1] << 16) | (b [i + 2] << 8) | b [i + 3];
        i += 4 + size;
        count++;
      }
      return count;
    }

    public int[] Drain ()
    {
      var l = new List<int> ();
      while (Messages.TryDequeue (out var v))
        l.Add (v);
      return l.ToArray ();
    }

    public void Dispose ()
    {
      running = false;
      thread.Join ();
      socket.Close ();
    }
  }
}
