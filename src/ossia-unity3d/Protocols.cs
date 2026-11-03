using System;
using System.Threading;

namespace Ossia
{
	public class Protocol
	{
		internal IntPtr ossia_protocol;
		protected Protocol(IntPtr impl)
		{
			ossia_protocol = impl;
		}
	}

	public class Local : Protocol
	{
		public Local() :
		base(Network.ossia_protocol_multiplex_create())
		{
		}

		public void ExposeTo(Protocol other)
		{
			Network.ossia_protocol_multiplex_expose_to (ossia_protocol, other.ossia_protocol);
		}
	}

	public class Minuit : Protocol
	{
		public Minuit(string local_name, string ip, int in_port, int out_port) :
		base(Network.ossia_protocol_minuit_create(local_name, ip, in_port, out_port))
		{
		}
	}

	public class OSCQuery : Protocol
	{
		public OSCQuery(int osc_port, int ws_port) :
		base(Network.ossia_protocol_oscquery_server_create(osc_port, ws_port))
		{
		}

		//! The server of ossia-max: it sends a bundle (Device.PushBundle) to
		//! its clients as OSC bundles. It runs on ctx: it opens its ports,
		//! answers and receives only while ctx runs.
		public OSCQuery(NetworkContext ctx, int osc_port, int ws_port) :
		base(Network.ossia_protocol_oscquery_server_asio_create(ctx.ossia_network_context, osc_port, ws_port))
		{
		}
	}

	public class OSCQueryMirror : Protocol
	{
		//! A client of an OSCQuery server, host as ws://127.0.0.1:5678. Blocks
		//! until the WebSocket is up, 1.5 s at most; the protocol is null when
		//! the server did not answer.
		public OSCQueryMirror(string host) :
		base(Network.ossia_protocol_oscquery_mirror_create(host))
		{
		}
	}

	//! The event loop of the servers created on it, on a thread of its own:
	//! the values they receive are applied there, as the websocket servers do.
	public class NetworkContext
	{
		internal IntPtr ossia_network_context;
		Thread thread;
		volatile bool running;

		public NetworkContext()
		{
			ossia_network_context = Network.ossia_network_context_create ();
		}

		//! Once a server waits on it: the loop stops for good when it finds
		//! nothing to do.
		public void Start()
		{
			if (thread != null)
				return;
			running = true;
			var ctx = ossia_network_context;
			thread = new Thread (() => {
				while (running) {
					Network.ossia_network_context_poll (ctx);
					Thread.Sleep (1);
				}
			});
			thread.IsBackground = true;
			thread.Name = "ossia network";
			thread.Start ();
		}

		//! Before the devices of its servers are freed
		public void Stop()
		{
			running = false;
			if (thread != null)
				thread.Join ();
			thread = null;
		}

		public void Free()
		{
			Stop ();
			if (ossia_network_context != IntPtr.Zero)
				Network.ossia_network_context_free (ossia_network_context);
			ossia_network_context = IntPtr.Zero;
		}
	}

	public class OSC : Protocol
	{
		public OSC(string ip, int in_port, int out_port) :
		base(Network.ossia_protocol_osc_create(ip, in_port, out_port))
		{
		}
	}
}
