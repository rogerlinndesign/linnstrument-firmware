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
# A walk through User Firmware Mode from the shell with SendMIDI and ReceiveMIDI
# (https://github.com/gbevin/SendMIDI, https://github.com/gbevin/ReceiveMIDI). The instrument
# must be in its normal MIDI mode and show up as "LinnStrument MIDI".
#
#   examples/user_firmware_demo.sh [seconds to watch touches, default 5]
set -e

DEV="LinnStrument MIDI"
WATCH=${1:-5}

for tool in sendmidi receivemidi; do
  command -v "$tool" >/dev/null 2>&1 || { echo "$tool is not installed" >&2; exit 1; }
done
if ! receivemidi list | grep -q "$DEV"; then
  echo "no \"$DEV\" device, is it connected and in MIDI mode?" >&2
  exit 1
fi

echo "Entering User Firmware Mode"
sendmidi dev "$DEV" nrpn 245 1

echo "Enabling pressure on the bottom row and X on the second row"
sendmidi dev "$DEV" ch 1 cc 12 1
sendmidi dev "$DEV" ch 2 cc 10 1

echo "Painting a diagonal in green, one cell per row"
for row in 0 1 2 3 4 5 6 7; do
  sendmidi dev "$DEV" cc 20 $((row + 2)) cc 21 "$row" cc 22 3
done

# receivemidi prints an NRPN twice, once when the value MSB arrives and once complete,
# so the reply shows as "245 0" followed by "245 1"
echo "Asking the instrument whether the mode is on (expect NRPN 245 value 1)"
receivemidi dev "$DEV" nrpn &
RM=$!
sleep 0.3
sendmidi dev "$DEV" nrpn 299 245
sleep 0.7
kill $RM 2>/dev/null || true
wait $RM 2>/dev/null || true

echo "Watching touches for $WATCH seconds, play something"
receivemidi dev "$DEV" &
RM=$!
sleep "$WATCH"
kill $RM 2>/dev/null || true
wait $RM 2>/dev/null || true

echo "Back to the normal firmware"
sendmidi dev "$DEV" nrpn 245 0
