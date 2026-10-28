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
# Runs the GitHub Actions jobs the way the Linux runner does, in an x86_64 Ubuntu container, so a
# push is not the first place the firmware build and the desktop tests meet Linux. Needs Docker;
# on Apple silicon the container runs under emulation and takes several minutes.
#
#   tools/ci-linux.sh
set -e

repo=$(cd "$(dirname "$0")/.." && pwd)
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
mkdir -p "$work/linnstrument-firmware"
cp -R "$repo"/. "$work/linnstrument-firmware/"
rm -rf "$work/linnstrument-firmware/build" "$work/linnstrument-firmware/.git"

cat > "$work/ci.sh" <<'INNER'
set -e
export DEBIAN_FRONTEND=noninteractive
apt-get update -qq >/dev/null
apt-get install -y -qq curl ca-certificates git build-essential nodejs >/dev/null 2>&1
GO_VERSION=$(sed -n 's/^go //p' /work/linnstrument-firmware/go.mod)
curl -sSL "https://go.dev/dl/go${GO_VERSION}.linux-amd64.tar.gz" | tar -C /usr/local -xz
export PATH=/usr/local/go/bin:$PATH
curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | BINDIR=/usr/local/bin sh >/dev/null 2>&1
cd /work
echo "== build job"
arduino-cli core update-index >/dev/null
arduino-cli core install arduino:sam@1.6.12 2>&1 | tail -1
arduino-cli compile --fqbn arduino:sam:arduino_due_x --libraries linnstrument-firmware/libraries --output-dir out linnstrument-firmware 2>&1 | grep -i "sketch uses\|error"
cd /work/linnstrument-firmware
echo "== tool job"
go vet ./...
go test ./...
# a pipe would hide a failing step from set -e, so each one reports through a file
go run ./tools/linnstrument test > /tmp/tests.log 2>&1 || { cat /tmp/tests.log; exit 1; }
tail -1 /tmp/tests.log
node --test examples/browser/ > /tmp/node.log 2>&1 || { cat /tmp/node.log; exit 1; }
go run ./tools/linnstrument nrpns /tmp/nrpn_parameters.js >/dev/null && cmp /tmp/nrpn_parameters.js examples/browser/nrpn_parameters.js
grep -E "^# (pass|fail)" /tmp/node.log
GOOS=linux GOARCH=amd64 go build -o /tmp/linnstrument-linux-amd64 ./tools/linnstrument
GOOS=linux GOARCH=arm64 go build -o /tmp/linnstrument-linux-arm64 ./tools/linnstrument
GOOS=windows GOARCH=amd64 go build -o /tmp/linnstrument-windows.exe ./tools/linnstrument
echo "== all jobs passed"
INNER

exec docker run --rm --platform linux/amd64 -v "$work:/work" -w /work ubuntu:24.04 sh /work/ci.sh
