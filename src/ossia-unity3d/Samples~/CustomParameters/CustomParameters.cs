using UnityEngine;
using UnityEngine.Rendering;

// Five fields of a script, exposed over OSCQuery. The ExposeAttributes
// component of the same GameObject finds the fields marked [Ossia.Expose]
// and publishes each one under /<GameObject name>/<script class>/<name>,
// here /Shape/CustomParameters/speed, /Shape/CustomParameters/axis, ...
// A value sent by a client changes the field; with Send Updates on (the
// default) on ExposeAttributes, a change made in Unity is sent to the clients.
public class CustomParameters : MonoBehaviour
{
  // Rotation speed, in degrees per second
  [Ossia.Expose("speed")]
  public float speed = 45;

  // Rotation axis: 0 x, 1 y, 2 z (any other value: y)
  [Ossia.Expose("axis")]
  public int axis = 1;

  // Rotation on or off
  [Ossia.Expose("spin")]
  public bool spin = true;

  // Color of the shape, r g b a from 0 to 1
  [Ossia.Expose("color")]
  public Color color = new Color(1f, 0.5f, 0f, 1f);

  // Scale of the shape, x y z
  [Ossia.Expose("size")]
  public Vector3 size = Vector3.one;

  Material material;

  void Start()
  {
    var r = GetComponent<Renderer>();
    // The cube of the scene has Unity's built-in default material, which
    // the Universal and High Definition render pipelines cannot draw.
    if (GraphicsSettings.currentRenderPipeline != null) {
      var lit = Shader.Find("Universal Render Pipeline/Lit");
      if (lit == null)
        lit = Shader.Find("HDRP/Lit");
      if (lit != null)
        r.sharedMaterial = new Material(lit);
    }
    material = r.material;
  }

  void Update()
  {
    if (spin) {
      var a = axis == 0 ? Vector3.right : axis == 2 ? Vector3.forward : Vector3.up;
      transform.Rotate(a, speed * Time.deltaTime);
    }
    transform.localScale = size;
    material.color = color;
  }
}
