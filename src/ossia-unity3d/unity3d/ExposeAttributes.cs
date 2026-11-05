using System;
using System.Reflection;
using System.Collections.Generic;
using UnityEngine;

namespace Ossia
{

  //! Expose an object through ossia.
  //! This shows only the transform and the fields explicitly marked with [Ossia.Expose].
  public class ExposeAttributes : ExposedObject
  {
    void RegisterComponent(Component component)
    {
      // The public fields of its class and of the classes it derives from
      const BindingFlags flags = BindingFlags.Public | BindingFlags.Instance;
      FieldInfo[] fields = component.GetType().GetFields(flags);

      // Find the fields that are marked for exposition
      OssiaEnabledComponent ossia_c = null;
      foreach (FieldInfo field in fields) {
        if(Attribute.IsDefined(field, typeof(Ossia.Expose))) 
        {
          // Get the type of the field
          ossia_type t;
          try { t = Ossia.Value.GetOssiaType (field.FieldType); } catch { continue; }

          // Create a node for the component
          if (ossia_c == null) {
            ossia_c = makeComponent(component);
          }

          // Create an attribute to bind the field.
          var attr = (Ossia.Expose) Attribute.GetCustomAttribute(field, typeof(Ossia.Expose));

          var oep = new OssiaEnabledField (field, NameOf (attr, field.Name), ossia_c, t);
          ApplyOptions (oep.ossia_parameter, attr, field.FieldType);
          ossia_c.fields.Add(oep);
          controller.Register (oep);
        }
      }

      foreach (PropertyInfo prop in component.GetType().GetProperties ()) 
      {
        if(Attribute.IsDefined(prop, typeof(Ossia.Expose))) {

          // Get the type of the property;
          ossia_type t;
          try { t = Ossia.Value.GetOssiaType (prop.PropertyType); } catch { continue; }

          // Create a node for the component
          if (ossia_c == null) {
            ossia_c = makeComponent(component);
          }

          var attr = (Ossia.Expose) Attribute.GetCustomAttribute(prop, typeof(Ossia.Expose));

          // Create an attribute to bind the property.
          var oep = new OssiaEnabledProperty(prop, NameOf (attr, prop.Name), ossia_c, t);
          ApplyOptions (oep.ossia_parameter, attr, prop.PropertyType);
          ossia_c.properties.Add(oep);
          controller.Register (oep);
        }
      }
    }

    // The transform once: localPosition, eulerAngles, localEulerAngles,
    // localRotation, right, up, forward and lossyScale say again what these
    // say, and set back they disagree with them: up or right holds a part of
    // the orientation only, and the object takes another one.
    static readonly string[] transformProperties = { "position", "rotation", "localScale" };

    void RegisterTransform()
    {
      var component = this.transform;
      var ossia_c = new OssiaEnabledComponent(component, child_node.AddChild("transform"), this);
      ossia_components.Add (ossia_c);

      foreach (var name in transformProperties)
      {
        var prop = typeof(Transform).GetProperty (name);
        var t = Ossia.Value.GetOssiaType (prop.PropertyType);

        // Create an attribute to bind the property.
        var oep = new OssiaEnabledProperty(prop, prop.Name, ossia_c, t);
        // A vector has no unit of its own: the position has one
        if (name == "position")
          Network.ossia_parameter_set_unit (oep.ossia_parameter.ossia_parameter, "position.cart3D");
        ossia_c.properties.Add (oep);
        controller.Register (oep);
      }
    }

    // [Ossia.Expose] without a name: the name of the field or property
    static string NameOf (Ossia.Expose attr, string member)
    {
      return string.IsNullOrEmpty (attr.ExposedName) ? member : attr.ExposedName;
    }

    // Unit first: it may change the type of the bounds
    static void ApplyOptions (Ossia.Parameter p, Ossia.Expose attr, Type t)
    {
      if (!string.IsNullOrEmpty (attr.Unit))
        Network.ossia_parameter_set_unit (p.ossia_parameter, attr.Unit);
      if (!float.IsNaN (attr.Min))
        p.SetMin (Bound (attr.Min, t));
      if (!float.IsNaN (attr.Max))
        p.SetMax (Bound (attr.Max, t));
      if (attr.Access != ossia_access_mode.BI)
        p.SetAccessMode (attr.Access);
    }

    // The bound in the type of the member, the same for each component
    static Ossia.Value Bound (float v, Type t)
    {
      if (t == typeof(int))
        return new Ossia.Value ((int)v);
      if (t == typeof(Vector2))
        return new Ossia.Value (new Vector2 (v, v));
      if (t == typeof(Vector3))
        return new Ossia.Value (new Vector3 (v, v, v));
      if (t == typeof(Vector4))
        return new Ossia.Value (new Vector4 (v, v, v, v));
      return new Ossia.Value (v);
    }


    public override void Start()
    {
      if (child_node != null) {
        Debug.Log("Object already registered.");        
        return;
      }

      controller = Controller.Get ();
      scene_node = controller.SceneNode ();
      if (scene_node == null) {
        Debug.Log("scene_node == null");  
        return;
      }

      child_node = scene_node.AddChild(this.gameObject.name);

      // For each component, we check the public fields.
      // If these fields have the Ossia.Expose attribute,
      // then we create node structures for them.
      foreach (var component in this.gameObject.GetComponents <Component>()) {
        RegisterComponent (component);
      }

      // We also register the transform in all cases.
      RegisterTransform ();
    }


    OssiaEnabledComponent makeComponent(Component c)
    {
      var comp = new OssiaEnabledComponent(c, child_node.AddChild(c.GetType().ToString()), this);
      ossia_components.Add (comp);
      return comp;        
    }

    Ossia.Node scene_node;
  }
}

