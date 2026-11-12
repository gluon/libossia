#!/bin/sh
# SPDX-FileCopyrightText: 2026 CNRS
# SPDX-FileContributor: Julien Bayle, PRISM <https://www.prism.cnrs.fr/>
#
# Compiles the scripts of the Unity package with the C# compiler and the
# UnityEngine assemblies of a Unity editor, without opening the editor:
# unsafe code allowed, as ossia.asmdef says.
#   - given a package (.tgz, or the installed package folder): its Scripts;
#   - otherwise: the sources the install puts in Scripts, every .cs of
#     src/ossia-unity3d but Tests and Samples~ (src/ossia-unity3d/CMakeLists.txt),
#     examples included.
# Unity compiles more (the tests, the samples), and only Unity runs them: this
# check only proves that the scripts compile.
# usage: check_unity_scripts.sh <Unity.app> [package.tgz | installed package folder]
#   e.g. check_unity_scripts.sh /Applications/Unity/Hub/Editor/6000.3.27f2/Unity.app
# Exit code 0 when the scripts compile.
set -eu
app=$1
scripting=$app/Contents/Resources/Scripting
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT

if [ $# -ge 2 ]; then
  if [ -d "$2" ]; then
    cp -R "$2" "$tmp/package"
  else
    tar xzf "$2" -C "$tmp"
  fi
  scripts=$tmp/package/Scripts
  list() { (cd "$scripts" && find . -name '*.cs' | sort); }
else
  scripts=$(cd "$(dirname "$0")/../src/ossia-unity3d" && pwd)
  list() { (cd "$scripts" && find . -name '*.cs' -not -path './Tests/*' -not -path './Samples~/*' | sort); }
fi

rsp=$tmp/csc.rsp
{
  echo "-nologo -noconfig -nostdlib -target:library -unsafe -langversion:9"
  echo "-define:UNITY_2023_1_OR_NEWER"
  echo "-out:\"$tmp/ossia.dll\""
  echo "-r:\"$scripting/NetStandard/ref/2.1.0/netstandard.dll\""
  for dll in "$scripting"/Managed/UnityEngine/*.dll; do
    echo "-r:\"$dll\""
  done
  list | while read -r f; do echo "\"$scripts/${f#./}\""; done
} > "$rsp"

count=$(grep -c '\.cs"$' "$rsp")
"$scripting/NetCoreRuntime/dotnet" "$scripting/DotNetSdkRoslyn/csc.dll" "@$rsp"
echo "$count scripts of the package compile"
