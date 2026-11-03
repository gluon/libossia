// SPDX-FileCopyrightText: 2026 CNRS
// SPDX-FileContributor: Julien Bayle, PRISM <https://www.prism.cnrs.fr/>

using UnityEngine;

// The field the cross bench writes and reads, exposed as
// /probe/CrossFields/value when the player serves.
public class CrossFields : MonoBehaviour
{
  [Ossia.Expose ("value")]
  public float value;
}
