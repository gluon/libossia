using NUnit.Framework;
using UnityEngine;

namespace Ossia.Tests
{
  public class ExposedObjectTests
  {
    // Send Updates is on when the component is added: what changes in Unity
    // reaches the clients without a box to tick. A scene keeps the value it
    // saved.
    [Test]
    public void SendUpdatesIsOnByDefault()
    {
      var go = new GameObject("Shape");
      try {
        Assert.IsTrue(go.AddComponent<ExposeAttributes>().sendUpdates, "ExposeAttributes");
        Assert.IsTrue(go.AddComponent<ExposeComponents>().sendUpdates, "ExposeComponents");
      } finally {
        Object.DestroyImmediate(go);
      }
    }
  }
}
