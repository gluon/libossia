# SPDX-FileCopyrightText: 2026 CNRS
# SPDX-FileContributor: Julien Bayle, PRISM <https://www.prism.cnrs.fr/>
#
# The version written in docs/prism/LISEZMOI.txt, which the package carries at
# its root, is the version of the package; built at a release tag, the tag is
# that version too. Stops the configuration otherwise.
#
# Included by CMakeLists.txt, or run alone:
#   cmake -DLISEZMOI=<file> -DVERSION=<x> [-DTAG=<tag>] -P check_version.cmake
# TAG: the release tag the sources are built at (v2.1.0+prism.N), empty when
# they are at none.

function(ossia_max_check_version lisezmoi version tag)
  file(STRINGS "${lisezmoi}" first LIMIT_COUNT 1)
  if(NOT first STREQUAL "ossia-prism ${version}")
    message(FATAL_ERROR
      "${lisezmoi} starts with \"${first}\", not \"ossia-prism ${version}\": "
      "write the version of the package (OSSIA_MAX_PACKAGE_VERSION) on its first line")
  endif()
  if(tag MATCHES "^v[0-9.]+\\+prism\\.[0-9]+$" AND NOT tag STREQUAL "v${version}")
    message(FATAL_ERROR
      "built at the tag ${tag}, but the package says ${version} (OSSIA_MAX_PACKAGE_VERSION)")
  endif()
endfunction()

if(CMAKE_SCRIPT_MODE_FILE)
  ossia_max_check_version("${LISEZMOI}" "${VERSION}" "${TAG}")
endif()
