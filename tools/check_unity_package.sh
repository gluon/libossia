#!/bin/sh
# SPDX-FileCopyrightText: 2026 CNRS
# SPDX-FileContributor: Julien Bayle, PRISM <https://www.prism.cnrs.fr/>
#
# Checks a Unity package for two things Unity needs to load a package
# installed from a tarball, where the package folder is read-only:
#   - a .meta next to every file and folder; Unity ignores an asset without
#     one. Hidden names and names ending with ~ (Samples~), and what they
#     hold, are not imported by Unity and are left out;
#   - a "version" in package.json that is semver (no leading "v").
# usage: check_unity_package.sh <package.tgz | installed package folder>
# Exit code 0 when both hold, 1 otherwise.
set -eu
src=$1
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
if [ -d "$src" ]; then
  cp -R "$src" "$tmp/package"
else
  tar xzf "$src" -C "$tmp"
fi
cd "$tmp/package"

total=0
missing=0
for f in $(find . -mindepth 1 \( -name '.*' -o -name '*~' \) -prune -o ! -name '*.meta' -print | sort); do
  total=$((total + 1))
  if [ ! -e "$f.meta" ]; then
    missing=$((missing + 1))
    echo "no .meta: ${f#./}"
  fi
done
echo "$missing of $total files and folders have no .meta"

version=$(sed -n 's/^ *"version": *"\([^"]*\)".*/\1/p' package.json)
semver='^(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)(-[0-9A-Za-z-]+(\.[0-9A-Za-z-]+)*)?(\+[0-9A-Za-z-]+(\.[0-9A-Za-z-]+)*)?$'
if printf '%s' "$version" | grep -Eq "$semver"; then
  echo "version \"$version\": semver"
  version_ok=1
else
  echo "version \"$version\": not semver"
  version_ok=0
fi

[ "$missing" -eq 0 ] && [ "$version_ok" -eq 1 ]
