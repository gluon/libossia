using System;
using System.Reflection;
using System.Collections.Generic;
using UnityEngine;

namespace Ossia
{
  internal interface OssiaEnabledElement
  {
    void ReceiveUpdates ();
  }

  //! The values a binding pushed, in order, until each one comes back
  //! through the Controller's message queue
  internal class Published
  {
    // A value comes back at the latest a frame after it left: more than
    // that is a value whose message was lost, kept no longer
    const int Kept = 16;
    readonly List<object> values = new List<object> ();

    public void Add (object v)
    {
      values.Add (v);
      if (values.Count > Kept)
        values.RemoveAt (0);
    }

    public void Clear ()
    {
      values.Clear ();
    }

    //! v is a value pushed here: it and the ones pushed before it are back
    public bool CameBack (object v)
    {
      int i = values.FindIndex (p => v.Equals (p));
      if (i < 0)
        return false;
      values.RemoveRange (0, i + 1);
      return true;
    }
  }

  //! Used to register C# fields to Ossia
  internal class OssiaEnabledField : OssiaEnabledElement
  {
    public OssiaEnabledComponent parent;

    public FieldInfo field;
    public string attribute;

    public Ossia.Node ossia_node;
    public Ossia.Parameter ossia_parameter;

    object previousValue;
    // The values SendUpdates pushed that have not come back yet
    readonly Published published = new Published ();

    public OssiaEnabledField (FieldInfo f, string attr, OssiaEnabledComponent par, ossia_type t)
    {
      field = f;
      attribute = attr;
      parent = par;

      ossia_node = parent.component_node.AddChild (attr);
      ossia_parameter = ossia_node.CreateParameter (f.FieldType, t);
      SendUpdates (null);
      // Pushed before the Controller registers the parameter: no message
      // comes back for it.
      published.Clear ();
    }

    //! Ossia -> C#
    public void ReceiveUpdates ()
    {
      try {
        var value = ossia_parameter.GetValue ();
        var new_val = value.Get (field.FieldType);

        // The message queue also receives the values pushed here: applying
        // one would undo what the script changed since. When the object
        // publishes before the Controller reads its queue, the value of the
        // frame before comes back after the one of this frame is published.
        if (published.CameBack (new_val))
          return;

        var cur_val = field.GetValue (parent.component);
        if (!new_val.Equals (cur_val)) {
          field.SetValue (parent.component, new_val);
        }
        // A value received does not leave again: SendUpdates sees no change
        previousValue = new_val;
        parent.OnReceived (attribute, new_val);
      } catch (Exception e) {
        Debug.LogWarning (e.Message);
      }
    }

    //! C# -> Ossia, through the Controller; at once without it
    public void SendUpdates (Controller controller)
    {
      var val = field.GetValue (parent.component);
      if (!val.Equals (previousValue)) {
        previousValue = val;
        published.Add (val);
        if (controller != null)
          controller.Publish (ossia_parameter, new Value (val));
        else
          ossia_parameter.PushValue (new Value (val));
      }
    }
  }

  //! Used to register C# properties to Ossia
  internal class OssiaEnabledProperty : OssiaEnabledElement
  {
    public OssiaEnabledComponent parent;

    public PropertyInfo field;
    public string attribute;

    public Ossia.Node ossia_node;
    public Ossia.Parameter ossia_parameter;

    object previousValue;
    // The values SendUpdates pushed that have not come back yet
    readonly Published published = new Published ();

    public OssiaEnabledProperty (PropertyInfo f, string attr, OssiaEnabledComponent par, ossia_type t)
    {
      field = f;
      attribute = attr;
      parent = par;

      ossia_node = parent.component_node.AddChild (attr);
      ossia_parameter = ossia_node.CreateParameter (f.PropertyType, t);
      SendUpdates (null);
      published.Clear ();
    }

    //! Ossia -> C#
    public void ReceiveUpdates ()
    {
      if (field.CanRead && field.CanWrite) {   
        try {
          var value = ossia_parameter.GetValue ();
          var new_val = value.Get (field.PropertyType);

          // As for a field: a published transform set back would hold the
          // object a frame behind the script that moves it.
          if (published.CameBack (new_val))
            return;

          var cur_val = field.GetValue (parent.component, null);
          if (!new_val.Equals (cur_val)) {
            field.SetValue (parent.component, new_val, null);
          }
          previousValue = new_val;
          parent.OnReceived (attribute, new_val);
        } catch (Exception e) {
          Debug.LogWarning (e.Message);
        }
      }
    }

    //! C# -> Ossia, through the Controller; at once without it
    public void SendUpdates (Controller controller)
    {
      if (field.CanRead) {
        var val = field.GetValue (parent.component, null);
        if (!val.Equals (previousValue)) {
          previousValue = val;
          published.Add (val);
          if (controller != null)
            controller.Publish (ossia_parameter, new Value (val));
          else
            ossia_parameter.PushValue (new Value (val));
        }
      }
    }
  }

  //! A component whose fields have some [Ossia.Expose] attributes
  internal class OssiaEnabledComponent
  {
    public Component component;
    public Ossia.Node component_node;
    // The object that exposes it, which raises Received
    public ExposedObject owner;

    public List<OssiaEnabledField> fields = new List<OssiaEnabledField> ();
    public List<OssiaEnabledProperty> properties = new List<OssiaEnabledProperty> ();

    public OssiaEnabledComponent (Component comp, Ossia.Node node, ExposedObject owner = null)
    {
      component = comp;
      component_node = node;
      this.owner = owner;
    }

    internal void OnReceived (string name, object value)
    {
      if (owner != null)
        owner.OnReceived (name, value);
    }

    //! Ossia -> C#
    public void ReceiveUpdates ()
    {
      foreach (var field in fields) {
        field.ReceiveUpdates ();
      }
      foreach (var property in properties) {
        property.ReceiveUpdates ();
      }
    }

    //! C# -> Ossia
    public void SendUpdates (Controller controller)
    {
      foreach (var field in fields) {
        field.SendUpdates (controller);
      }
      foreach (var property in properties) {
        property.SendUpdates (controller);
      }
    }
  }
}