using System.Runtime;
using System.Runtime.InteropServices;
using System.Collections.Generic;
using System;

namespace Ossia
{
	[System.AttributeUsage(System.AttributeTargets.All)]
	public class Expose : System.Attribute
	{
		public string ExposedName;

		//! Bounds published in RANGE; NaN, the default, leaves that side open.
		//! For a vector, each component has the same bound.
		public float Min = float.NaN;
		public float Max = float.NaN;
		//! An ossia unit, e.g. "angle.degree", "gain.db", "position.cart3D";
		//! null, the default: the unit of the type (a Color, a Quaternion)
		public string Unit;
		//! GET: clients read it, a value they send is ignored; SET: they
		//! write it only; BI, the default: both
		public ossia_access_mode Access = ossia_access_mode.BI;

		public Expose()
		{
			this.ExposedName = String.Empty;
		}
		public Expose(string name)
		{
			this.ExposedName = name;
		}
	}
}
