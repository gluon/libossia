// SPDX-FileCopyrightText: 2026 CNRS
// SPDX-FileContributor: Julien Bayle, PRISM <https://www.prism.cnrs.fr/>

using System;
using UnityEditor;
using UnityEditor.SceneManagement;

// Builds the player of the cross bench: an empty scene, where the probe
// starts, then the scene of the sample "Follow a device".
public static class CrossBuild
{
  public static void Build ()
  {
    string player = null;
    var a = Environment.GetCommandLineArgs ();
    for (int i = 0; i < a.Length - 1; i++)
      if (a[i] == "-crossPlayer")
        player = a[i + 1];
    const string empty = "Assets/Cross/Empty.unity";
    var scene = EditorSceneManager.NewScene (NewSceneSetup.EmptyScene, NewSceneMode.Single);
    EditorSceneManager.SaveScene (scene, empty);
    PlayerSettings.productName = "cross";
    PlayerSettings.runInBackground = true;
    PlayerSettings.SetScriptingBackend (UnityEditor.Build.NamedBuildTarget.Standalone, ScriptingImplementation.Mono2x);
    var report = BuildPipeline.BuildPlayer (new BuildPlayerOptions {
      scenes = new[] { empty, "Assets/Samples/FollowADevice/FollowADevice.unity" },
      locationPathName = player,
      target = BuildTarget.StandaloneOSX,
      options = BuildOptions.None
    });
    if (report.summary.result != UnityEditor.Build.Reporting.BuildResult.Succeeded)
      EditorApplication.Exit (1);
  }
}
