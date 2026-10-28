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
#
# Prints the name every build of this checkout carries: the firmware version as the instrument's
# screen shows it, then the commit, such as 240.075-1a2b3c4. CI names the firmware, the tool and
# the Virtual LinnStrument after it.
#
#   tools/build-name.sh        for the checked-out commit
#   tools/build-name.sh SHA    for that commit
set -e
root=$(cd "$(dirname "$0")/.." && pwd)
ino="$root/linnstrument-firmware.ino"
version=$(sed -n 's/^const char\* OSVersion = "\([0-9.]*\)";/\1/p' "$ino")
build=$(sed -n 's/^const char\* OSVersionBuild = "\(\.[0-9]*\)";/\1/p' "$ino")
sha=${1:-$(git -C "$root" rev-parse HEAD)}
# OSVersion ends with the dot OSVersionBuild starts with
echo "${version%.}${build}-$(echo "$sha" | cut -c1-7)"
