#!/bin/sh
# Copyright 2026 Roger Linn Design (https://www.rogerlinndesign.com)
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# Builds the Virtual LinnStrument from an older release of the firmware, to see what the
# instrument did before a change or how a page behaves on a firmware customers still run.
#
#   tools/virtual/build-release.sh 2.3.4
#
# The release's sketch goes in a git worktree under build/virtual-<tag>/src, with this
# checkout's tools/, tests/support/ and Go module copied in (the app and its mocks are newer
# than the sketch), and the app is built under build/virtual-<tag>, reusing the JUCE the
# normal build fetched. The path of the binary is printed last. Only one Virtual LinnStrument
# runs at a time, so quit the current one before starting this one.
set -e
tag="$1"
if [ -z "$tag" ]; then
  echo "usage: tools/virtual/build-release.sh TAG" >&2
  exit 1
fi
repo=$(cd "$(dirname "$0")/../.." && pwd)
cd "$repo"
git rev-parse -q --verify "$tag^{commit}" > /dev/null || { echo "no such tag or commit: $tag" >&2; exit 1; }
src="$repo/build/virtual-$tag/src"
out="$repo/build/virtual-$tag"
if [ ! -d "$src" ]; then
  mkdir -p "$out"
  git worktree prune
  git worktree add --detach "$src" "$tag"
fi
rm -rf "$src/tools" "$src/tests/support"
mkdir -p "$src/tests"
cp -R tools "$src/tools"
cp -R tests/support "$src/tests/support"
cp go.mod go.sum "$src/"
# a strchr result that newer compilers refuse to store in a char*, fixed after 2.3.4
if grep -q "^    char\* p = strchr(font->chars" "$src/ls_font.ino"; then
  sed -i.bak 's/^    char\* p = strchr(font->chars/    const char* p = strchr(font->chars/' "$src/ls_font.ino"
  rm -f "$src/ls_font.ino.bak"
fi
old=OFF
grep -q "midiTextAppend" "$src"/*.ino || old=ON
juce=""
if [ -d "$repo/build/virtual/_deps/juce-src" ]; then
  juce="-DJUCE_PATH=$repo/build/virtual/_deps/juce-src"
fi
cmake -S "$src/tools/virtual" -B "$out" -DCMAKE_BUILD_TYPE=Release -DVIRTUAL_OLD_FIRMWARE=$old $juce
cmake --build "$out" --config Release
app="$out/VirtualLinnStrument_artefacts/Release"
if [ -d "$app/Virtual LinnStrument.app" ]; then
  echo "$app/Virtual LinnStrument.app/Contents/MacOS/Virtual LinnStrument"
else
  echo "$app/Virtual LinnStrument"
fi
