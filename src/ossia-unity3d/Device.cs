using System.Runtime;
using System.Runtime.InteropServices;
using System.Collections.Generic;
using System;

namespace Ossia {
	public class Device
	{
		internal IntPtr ossia_device = IntPtr.Zero;

		public Device(Protocol proto, string name)
		{
			ossia_device = Network.ossia_device_create(proto.ossia_protocol, name);
		}

		internal Device(IntPtr device)
		{
			ossia_device = device;
		}

		public string GetName()
		{
			IntPtr nameptr = Network.ossia_device_get_name (ossia_device);
			if (nameptr == IntPtr.Zero)
				return "ENONAME";
			string name = Marshal.PtrToStringAnsi (nameptr);
			Network.ossia_string_free(nameptr);
			return name;
		}

		public Node GetRootNode()
		{
			return new Node(Network.ossia_device_get_root_node (ossia_device));
		}

		public void Free()
		{
			Network.ossia_device_free (ossia_device);
		}

		//! False once the WebSocket of a mirror went down; true for the
		//! protocols that do not connect.
		public bool Connected()
		{
			return Network.ossia_device_connected (ossia_device) != 0;
		}

		//! Applies the tree changes a mirror received since the last call
		public void RunCommands()
		{
			Network.ossia_device_run_commands (ossia_device);
		}

		//! A server forwards the values a client sends to its other clients
		public void SetEcho(bool echo)
		{
			Network.ossia_device_set_echo (ossia_device, echo ? 1 : 0);
		}

		//! Sets the values on their parameters and sends them together, in
		//! OSC bundles
		public void PushBundle(IList<Parameter> parameters, IList<Value> values)
		{
			int count = Math.Min (parameters.Count, values.Count);
			var p = new IntPtr[count];
			var v = new IntPtr[count];
			int n = 0;
			for (int i = 0; i < count; i++) {
				// Zero once its parameter was deleted
				if (parameters [i].ossia_parameter == IntPtr.Zero)
					continue;
				p [n] = parameters [i].ossia_parameter;
				v [n] = values [i].ossia_value;
				n++;
			}
			if (n > 0)
				Network.ossia_device_push_bundle (ossia_device, p, v, n);
			GC.KeepAlive (values);
		}

		public IntPtr GetDevice() {
			return ossia_device;
		}
	}
}
