using NUnit.Framework;
using UnityEngine;

namespace Ossia.Tests
{
  public class ControllerTests
  {
    // The defaults of a Controller added to a scene, those of the earlier
    // versions, which had them fixed in the code.
    [Test]
    public void DefaultConfiguration()
    {
      var go = new GameObject("OssiaController");
      try {
        var c = go.AddComponent<Controller>();
        Assert.AreEqual("unity", c.deviceName);
        Assert.AreEqual(1234, c.oscPort);
        Assert.AreEqual(5678, c.wsPort);
        Assert.IsTrue(c.runInBackground);
      } finally {
        Object.DestroyImmediate(go);
      }
    }
  }
}
