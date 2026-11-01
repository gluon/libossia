using System;
using System.Linq;
using System.Reflection;
using NUnit.Framework;

namespace Ossia.Tests
{
  public class CallbackTests
  {
    // IL2CPP compiles ahead of time: the library can only call static
    // methods marked [MonoPInvokeCallback]. An instance method or a lambda
    // passes under Mono, in the editor, and fails in an IL2CPP player.
    [TestCase(typeof(Node), "OnNodeDeleting")]
    [TestCase(typeof(Parameter), "OnParameterDeleting")]
    [TestCase(typeof(Controller), "DebugLogCallback")]
    public void NativeCallbackIsStaticAndMarked(Type type, string name)
    {
      var m = type.GetMethod(name, BindingFlags.Static | BindingFlags.NonPublic | BindingFlags.Public);
      Assert.IsNotNull(m, type.Name + "." + name + " must be a static method");
      Assert.IsTrue(m.GetCustomAttributes(typeof(AOT.MonoPInvokeCallbackAttribute), false).Any(),
          type.Name + "." + name + " must be marked [MonoPInvokeCallback]");
    }

    [Test]
    public void EveryMarkedCallbackIsStatic()
    {
      var methods = typeof(Node).Assembly.GetTypes()
        .SelectMany(t => t.GetMethods(BindingFlags.Instance | BindingFlags.Static | BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.DeclaredOnly))
        .Where(m => m.GetCustomAttributes(typeof(AOT.MonoPInvokeCallbackAttribute), false).Any())
        .ToList();
      Assert.IsNotEmpty(methods);
      foreach (var m in methods)
        Assert.IsTrue(m.IsStatic, m.DeclaringType.Name + "." + m.Name);
    }

    // The deleting callbacks reach the C# objects through their handle.
    [Test]
    public void DeletedNodeAndParameterAreReleased()
    {
      var dev = new Device(new Local(), "callback_test");
      try {
        var root = dev.GetRootNode();
        var bar = Node.CreateNode(root, "/foo/bar");
        var param = bar.CreateParameter(ossia_type.VEC3F);
        GC.Collect();
        GC.WaitForPendingFinalizers();

        var foo = root.GetChild(0);
        Assert.AreEqual("foo", foo.GetName());
        Assert.AreNotEqual(IntPtr.Zero, bar.GetNode());

        foo.RemoveChild(foo.GetChild(0));
        Assert.AreEqual(IntPtr.Zero, bar.GetNode());
        Assert.IsNull(bar.GetParameter());
        var handle = typeof(Parameter).GetField("ossia_parameter", BindingFlags.Instance | BindingFlags.NonPublic);
        Assert.AreEqual(IntPtr.Zero, handle.GetValue(param));
      } finally {
        dev.Free();
      }
    }
  }
}
