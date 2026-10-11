// Copyright 2026 Roger Linn Design (https://www.rogerlinndesign.com)
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package main

import (
	"os"
	"testing"
)

func TestSwitchCasesReadsRangesAndSharedCases(t *testing.T) {
	body := `
  byte split = LEFT;
  switch (parameter) {
    case 0:
      if (inRange(value, 0, 2)) {
        Split[split].midiMode = value;
      }
      break;
    case 2: case 3:
    case 4:
      if (inRange(value, 0, 1)) {
        set(value);
      }
      break;
    case 35:
      if (inRange(value, 0, 4)) {
        switch (value) {
          case 0:
            x();
            break;
          case 1:
            y();
            break;
        }
      }
      break;
    case 62:
      toggle();
      break;
    case 63:
      if (inRange(value, 0, 5)) {
        load(value);
      }
      break;
  }
`
	cases := switchCases(body)
	if len(cases) != 7 {
		t.Fatalf("got %d cases: %v", len(cases), cases)
	}
	if c := cases[0]; !c.ranged || c.min != 0 || c.max != 2 {
		t.Errorf("case 0: %+v", c)
	}
	for _, n := range []int{2, 3, 4} {
		if c := cases[n]; !c.ranged || c.max != 1 {
			t.Errorf("case %d: %+v", n, c)
		}
	}
	if c := cases[35]; !c.ranged || c.max != 4 {
		t.Errorf("case 35: %+v", c)
	}
	if c := cases[62]; c.ranged {
		t.Errorf("case 62 has no range check and must not borrow the next one: %+v", c)
	}
	if c := cases[63]; !c.ranged || c.max != 5 {
		t.Errorf("case 63: %+v", c)
	}
}

func TestFirmwareCasesParse(t *testing.T) {
	source, err := os.ReadFile("../../ls_midi.ino")
	if err != nil {
		t.Skip("run from the repository")
	}
	received := switchCases(functionBody(string(source), "receivedNrpn"))
	if c := received[30]; !c.ranged || c.min != 1 || c.max != 11 {
		t.Errorf("color main: %+v", c)
	}
	if c := received[19]; !c.ranged || c.min != 1 || c.max != 96 {
		t.Errorf("bend range: %+v", c)
	}
	if _, ok := received[299]; !ok {
		t.Errorf("the query is handled")
	}
	answered := switchCases(functionBody(string(source), "sendNrpnParameter"))
	if _, ok := answered[275]; !ok {
		t.Errorf("275 is answered")
	}
}

func TestParseNrpnRow(t *testing.T) {
	tables := map[string][]nrpnOption{"color": {{0, "default"}, {1, "Red"}, {11, "Pink"}}}
	p, err := parseNrpnRow(37, "0-14", "Split Left Transpose Pitch (0-6: -7 to -1, 7: 0, 8-14: +1 to +7)", tables)
	if err != nil || p.Kind != "enum" || len(p.Options) != 15 || p.Options[0].Label != "-7" || p.Options[14].Label != "+7" {
		t.Errorf("transpose: %+v %v", p, err)
	}
	p, _ = parseNrpnRow(30, "1-11", "Split Left Color Main (see color value table below)", tables)
	if p.Kind != "enum" || len(p.Options) != 2 || p.Name != "Color Main" {
		t.Errorf("color: %+v", p)
	}
	p, _ = parseNrpnRow(40, "0-128", "Split Left MIDI CC For Fader 1 (0-127: CC, 128: Channel Aftertouch)", tables)
	if p.Kind != "number" || p.Note == "" {
		t.Errorf("fader: %+v", p)
	}
	p, _ = parseNrpnRow(272, "-", "Device Model (200 or 128), read-only, ask for it with NRPN 299", tables)
	if p.Kind != "readonly" || p.Name != "Model" {
		t.Errorf("model: %+v", p)
	}
}
