using System.Collections.Generic;
using NUnit.Framework;
using UnityEngine;

namespace Ossia.Tests
{
  // Each C# type goes into an ossia value and comes back unchanged, the way
  // ParameterBindings reads a value for a field or a property.
  public class ValueTests
  {
    static object RoundTrip(object v)
    {
      return new Value(v).Get(v.GetType());
    }

    [Test] public void Float() { Assert.AreEqual(0.25f, RoundTrip(0.25f)); }
    [Test] public void Int() { Assert.AreEqual(-42, RoundTrip(-42)); }
    [Test] public void Bool()
    {
      Assert.AreEqual(true, RoundTrip(true));
      Assert.AreEqual(false, RoundTrip(false));
    }
    [Test] public void String() { Assert.AreEqual("une chaîne", RoundTrip("une chaîne")); }

    [Test] public void Vector2() { Assert.AreEqual(new Vector2(1, 2), RoundTrip(new Vector2(1, 2))); }
    [Test] public void Vector3() { Assert.AreEqual(new Vector3(1, 2, 3), RoundTrip(new Vector3(1, 2, 3))); }
    [Test] public void Vector4() { Assert.AreEqual(new Vector4(1, 2, 3, 4), RoundTrip(new Vector4(1, 2, 3, 4))); }

    [Test] public void Quaternion()
    {
      var q = new Quaternion(0.1f, 0.2f, 0.3f, 0.9f);
      Assert.AreEqual(q, RoundTrip(q));
    }

    // ossia orders a quaternion x, y, z, w (orientation.quaternion), as Unity does
    [Test] public void QuaternionOrderOnTheWire()
    {
      var v = new Value(new Quaternion(0.1f, 0.2f, 0.3f, 0.9f)).GetVec4f();
      Assert.AreEqual(new Vector4(0.1f, 0.2f, 0.3f, 0.9f), v);
    }

    [Test] public void Color() { Assert.AreEqual(new Color(0.1f, 0.2f, 0.3f, 0.4f), RoundTrip(new Color(0.1f, 0.2f, 0.3f, 0.4f))); }

    // color.rgba8: components from 0 to 255
    [Test] public void Color32()
    {
      var c = new Color32(10, 128, 200, 255);
      Assert.AreEqual(c, RoundTrip(c));
      Assert.AreEqual(new Vector4(10, 128, 200, 255), new Value(c).GetVec4f());
    }

    [Test] public void FloatList()
    {
      var l = new float[] { 1.5f, -2, 3 };
      Assert.AreEqual(l, new Value(l).GetFloatArray());
      Assert.AreEqual(l, RoundTrip(l));
    }

    [Test] public void IntList()
    {
      var l = new int[] { 1, 2, 4, 65 };
      Assert.AreEqual(l, new Value(l).GetIntArray());
      Assert.AreEqual(l, RoundTrip(l));
    }

    [Test] public void ValueList()
    {
      var l = new Value(new List<Value> { new Value(1), new Value(2.5f), new Value("trois") });
      var back = l.GetList();
      Assert.AreEqual(3, back.Length);
      Assert.AreEqual(1, back[0].Get());
      Assert.AreEqual(2.5f, back[1].Get());
      Assert.AreEqual("trois", back[2].Get());
    }

    [Test] public void LargeValueList()
    {
      var items = new List<Value>();
      for (int i = 0; i < 64; i++)
        items.Add(new Value(i));
      var back = new Value(items).GetList();
      Assert.AreEqual(64, back.Length);
      for (int i = 0; i < 64; i++)
        Assert.AreEqual(i, back[i].Get());
    }
  }
}
