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

  //! Used to register C# fields to Ossia
  internal class OssiaEnabledField : OssiaEnabledElement
  {
    public OssiaEnabledComponent parent;

    public FieldInfo field;
    public string attribute;

    public Ossia.Node ossia_node;
    public Ossia.Parameter ossia_parameter;

    object previousValue;
    // The last value SendUpdates pushed, until a value from elsewhere replaces it
    object published;

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
      published = null;
    }

    //! Ossia -> C#
    public void ReceiveUpdates ()
    {
      try {
        var value = ossia_parameter.GetValue ();
        var new_val = value.Get (field.FieldType);

        // The message queue also receives the values pushed here: while
        // the parameter holds the value SendUpdates published, applying it
        // would undo what the script changed since.
        if (new_val.Equals (published))
          return;
        published = null;

        var cur_val = field.GetValue (parent.component);
        if (!new_val.Equals (cur_val)) {
          field.SetValue (parent.component, new_val);
        }
        // A value received does not leave again: SendUpdates sees no change
        previousValue = new_val;
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
        published = val;
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
    // The last value SendUpdates pushed, until a value from elsewhere replaces it
    object published;

    public OssiaEnabledProperty (PropertyInfo f, string attr, OssiaEnabledComponent par, ossia_type t)
    {
      field = f;
      attribute = attr;
      parent = par;

      ossia_node = parent.component_node.AddChild (attr);
      ossia_parameter = ossia_node.CreateParameter (f.PropertyType, t);
      SendUpdates (null);
      published = null;
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
          if (new_val.Equals (published))
            return;
          published = null;

          var cur_val = field.GetValue (parent.component, null);
          if (!new_val.Equals (cur_val)) {
            field.SetValue (parent.component, new_val, null);
          }
          previousValue = new_val;
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
          published = val;
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

    public List<OssiaEnabledField> fields = new List<OssiaEnabledField> ();
    public List<OssiaEnabledProperty> properties = new List<OssiaEnabledProperty> ();

    public OssiaEnabledComponent (Component comp, Ossia.Node node)
    {
      component = comp;
      component_node = node;
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