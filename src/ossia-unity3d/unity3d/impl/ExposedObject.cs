using System;
using System.Reflection;
using System.Collections.Generic;
using UnityEngine;

namespace Ossia
{
  public abstract class ExposedObject : MonoBehaviour
  {
    //! Pushes to the clients, each frame, what changed in Unity. On by default;
    //! a scene keeps the value it saved.
    public bool sendUpdates = true;
    public Ossia.Node child_node;
    internal List<OssiaEnabledComponent> ossia_components = new List<OssiaEnabledComponent>();
    protected Ossia.Controller controller = null;

    //! A value from a client applied to a field or a property, on the main
    //! thread: its exposed name and the value. Not for what Unity publishes.
    public event Action<string, object> Received;
    internal void OnReceived (string name, object value)
    {
      Received?.Invoke (name, value);
    }

    public abstract void Start();
    public void ReceiveUpdates()
    {
      if (child_node == null) {
        Start ();
      }

      if (!child_node.GetValueUpdating ()) 
        child_node.SetValueUpdating (true);

      foreach (var component in ossia_components) {
        component.ReceiveUpdates ();
      }
    }

    public void SendUpdates()
    {
      if (child_node == null) {
        Start ();
      }

      if(child_node.GetValueUpdating())
        child_node.SetValueUpdating (false);

      foreach (var component in ossia_components) {
        component.SendUpdates (controller);
      }
    }

    public void Update()
    {
      // Its Controller was destroyed, and its parameters with it
      if (child_node != null && controller == null)
        return;

      if (child_node == null) {
        Debug.Log("Object not registered.");  
        Start ();
      }

      if (sendUpdates) {
        SendUpdates ();
      }
    }

    ~ExposedObject()
    {
      foreach (var c in ossia_components) {
        foreach (var field in c.fields) {
          //controller.Unregister (field);
        }
        foreach (var prop in c.properties) {
          //controller.Unregister (prop);
        }
      }
    }

  }
}