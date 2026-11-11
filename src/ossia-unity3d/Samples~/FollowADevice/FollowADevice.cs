using UnityEngine;

// Follows follow_me, the device FollowADevice.maxpat exposes in Max. The
// Remote components of the cube bind these fields to its addresses: Max sets
// size, color and spin, Unity turns the cube and sends its angle back.
public class FollowADevice : MonoBehaviour
{
  public float size = 1;               // /cube/size
  public Color color = Color.white;    // /cube/color, r g b a from 0 to 1
  public float spin = 45;              // /cube/spin, degrees per second
  public float angle;                  // /cube/angle, sent to Max

  Renderer shape;

  void Awake ()
  {
    // Keep playing when Unity loses the focus, as when Max is clicked: the
    // scene has no Controller, which does it otherwise
    Application.runInBackground = true;
  }

  void Start ()
  {
    shape = GetComponent<Renderer> ();
  }

  void Update ()
  {
    angle = Mathf.Repeat (angle + spin * Time.deltaTime, 360);
    transform.localRotation = Quaternion.Euler (20, angle, 0);
    transform.localScale = Vector3.one * size;
    if (shape != null)
      shape.material.color = color;
  }
}
