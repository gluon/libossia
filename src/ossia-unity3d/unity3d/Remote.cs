using System;
using System.Reflection;
using UnityEngine;

namespace Ossia
{
  //! Binds a field or a property of a script to an address of the device a
  //! Client follows, both ways, as ossia.remote does in Max: the value of the
  //! address is applied to the field, on the main thread, and every change
  //! of the field is sent, whatever made it (a script, the Inspector, an
  //! animation). A value received is not sent back.
  public class Remote : MonoBehaviour
  {
    [Tooltip ("The Client that follows the device; the first Client of the scene when empty")]
    public Client client;
    [Tooltip ("The address on the remote device, e.g. /level or /shape/color")]
    public string address = "/";
    [Tooltip ("The script whose field or property follows the address")]
    public Component target;
    [Tooltip ("The name of the public field or property of the script")]
    public string member;
    [Tooltip ("Send the changes of the field to the device. Off, the field only follows it.")]
    public bool sendUpdates = true;

    //! True once the address was found on the device
    public bool Bound { get { return parameter != null; } }

    internal Ossia.Parameter Parameter { get { return parameter; } }

    void OnEnable ()
    {
      if (client == null)
#if UNITY_2023_1_OR_NEWER
        client = FindFirstObjectByType<Client> ();
#else
        client = FindObjectOfType<Client> ();
#endif
      if (client == null) {
        Debug.LogWarning ("OSSIA remote " + address + ": no Client in the scene");
        return;
      }
      if (!Resolve ())
        return;
      client.Add (this);
    }

    void OnDisable ()
    {
      if (client != null)
        client.Remove (this);
    }

    bool Resolve ()
    {
      if (target == null) {
        Debug.LogWarning ("OSSIA remote " + address + ": no target script");
        return false;
      }
      const BindingFlags flags = BindingFlags.Public | BindingFlags.Instance;
      field = target.GetType ().GetField (member, flags);
      property = field == null ? target.GetType ().GetProperty (member, flags) : null;
      if (field == null && property == null) {
        Debug.LogWarning ("OSSIA remote " + address + ": " + target.GetType () + " has no public field or property \"" + member + "\"");
        return false;
      }
      return true;
    }

    Type MemberType { get { return field != null ? field.FieldType : property.PropertyType; } }

    object Read ()
    {
      if (field != null)
        return field.GetValue (target);
      return property.CanRead ? property.GetValue (target, null) : null;
    }

    void Write (object v)
    {
      if (field != null)
        field.SetValue (target, v);
      else if (property.CanWrite)
        property.SetValue (target, v, null);
    }

    //! The parameter at the address, null while the device has none; the
    //! field takes its value.
    internal Ossia.Parameter Bind (Ossia.Device device)
    {
      if (target == null || (field == null && property == null))
        return null;
      var node = Ossia.Node.FindNode (device.GetRootNode (), address);
      if (node == null)
        return null;
      var p = node.GetParameter ();
      if (p == null)
        return null;
      parameter = p;
      previousValue = null;
      published = null;
      ReceiveUpdates ();
      return p;
    }

    internal void Unbind ()
    {
      parameter = null;
      previousValue = null;
      published = null;
    }

    //! The device -> the field
    internal void ReceiveUpdates ()
    {
      if (parameter == null || target == null)
        return;
      try {
        var new_val = parameter.GetValue ().Get (MemberType);
        if (new_val == null)
          return;
        // The queue also receives the values sent from here
        if (new_val.Equals (published))
          return;
        published = null;
        var cur_val = Read ();
        if (!new_val.Equals (cur_val))
          Write (new_val);
        // A value received is not sent back
        previousValue = new_val;
      } catch (Exception e) {
        Debug.LogWarning ("OSSIA remote " + address + ": " + e.Message);
      }
    }

    //! The field -> the device, at the end of the frame
    internal void SendUpdates ()
    {
      if (!sendUpdates || parameter == null || target == null)
        return;
      var val = Read ();
      if (val == null || val.Equals (previousValue))
        return;
      previousValue = val;
      published = val;
      client.Publish (parameter, new Value (val));
    }

    FieldInfo field;
    PropertyInfo property;
    Ossia.Parameter parameter;
    object previousValue;
    // The last value sent, until a value from the device replaces it
    object published;
  }
}
