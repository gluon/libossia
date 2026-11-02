# Writes a Unity .meta file next to each file and folder of the installed
# package that has none yet. The GUID derives from the path in the package,
# so it is the same from one build to the next: the sample scene and the
# projects that use the package keep their references to the scripts.
# Folders whose name ends with ~ (Samples~) are not imported by Unity, except
# the scripts of the samples: Unity copies a sample into the project with its
# .meta files, and its scene finds its scripts by their GUID.

file(GLOB_RECURSE entries
  LIST_DIRECTORIES true
  RELATIVE "${OSSIA_UNITY_PACKAGE_ROOT}"
  "${OSSIA_UNITY_PACKAGE_ROOT}/*")

foreach(path IN LISTS entries)
  if(path MATCHES "\\.meta$" OR path MATCHES "(^|/)\\.")
    continue()
  endif()
  if(path MATCHES "~(/|$)" AND NOT path MATCHES "^Samples~/.+\\.cs$")
    continue()
  endif()
  set(full "${OSSIA_UNITY_PACKAGE_ROOT}/${path}")
  if(EXISTS "${full}.meta")
    continue()
  endif()

  string(UUID guid
    NAMESPACE 09EC9332-39A2-4937-AF47-7C62ABC1AB89
    NAME "package/${path}"
    TYPE MD5)
  string(REPLACE "-" "" guid "${guid}")
  string(TOLOWER "${guid}" guid)

  if(IS_DIRECTORY "${full}")
    set(importer "folderAsset: yes\nDefaultImporter:\n  externalObjects: {}\n")
  elseif(path MATCHES "\\.cs$")
    set(importer "MonoImporter:\n  externalObjects: {}\n  serializedVersion: 2\n  defaultReferences: []\n  executionOrder: 0\n  icon: {instanceID: 0}\n")
  elseif(path MATCHES "\\.asmdef$")
    set(importer "AssemblyDefinitionImporter:\n  externalObjects: {}\n")
  elseif(path STREQUAL "package.json")
    set(importer "PackageManifestImporter:\n  externalObjects: {}\n")
  elseif(path MATCHES "\\.(md|txt)$")
    set(importer "TextScriptImporter:\n  externalObjects: {}\n")
  else()
    set(importer "DefaultImporter:\n  externalObjects: {}\n")
  endif()

  file(WRITE "${full}.meta"
    "fileFormatVersion: 2\nguid: ${guid}\n${importer}  userData: \n  assetBundleName: \n  assetBundleVariant: \n")
endforeach()
