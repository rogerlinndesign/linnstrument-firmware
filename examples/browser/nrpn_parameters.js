/*
Copyright 2026 Roger Linn Design (https://www.rogerlinndesign.com)

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/
// Generated from the NRPN table in midi.md by `go run ./tools/linnstrument nrpns`, do not edit
// by hand. Each parameter: number, group, name, kind (toggle, enum, number, action, choice-action,
// readonly), min, max, the options of an enum, a note for the person, and the original description.
(function (root, factory) {
  if (typeof module === "object" && module.exports) {
    module.exports = factory();
  } else {
    root.LinnNrpns = factory();
  }
})(typeof self !== "undefined" ? self : this, function () {
  "use strict";
  return {
    "parameters": [
      {
        "number": 0,
        "group": "Split Left",
        "name": "MIDI Mode",
        "kind": "enum",
        "min": 0,
        "max": 2,
        "options": [
          {
            "value": 0,
            "label": "One Channel"
          },
          {
            "value": 1,
            "label": "Channel Per Note"
          },
          {
            "value": 2,
            "label": "Channel Per Row"
          }
        ],
        "description": "Split Left MIDI Mode (0: One Channel, 1: Channel Per Note, 2: Channel Per Row)"
      },
      {
        "number": 1,
        "group": "Split Left",
        "name": "MIDI Main Channel",
        "kind": "number",
        "min": 1,
        "max": 16,
        "description": "Split Left MIDI Main Channel"
      },
      {
        "number": 2,
        "group": "Split Left",
        "name": "MIDI Per Note Channel 1",
        "kind": "toggle",
        "min": 0,
        "max": 1,
        "options": [
          {
            "value": 0,
            "label": "Off"
          },
          {
            "value": 1,
            "label": "On"
          }
        ],
        "description": "Split Left MIDI Per Note Channel 1 (0: Off, 1: On)"
      },
      {
        "number": 3,
        "group": "Split Left",
        "name": "MIDI Per Note Channel 2",
        "kind": "toggle",
        "min": 0,
        "max": 1,
        "options": [
          {
            "value": 0,
            "label": "Off"
          },
          {
            "value": 1,
            "label": "On"
          }
        ],
        "description": "Split Left MIDI Per Note Channel 2 (0: Off, 1: On)"
      },
      {
        "number": 4,
        "group": "Split Left",
        "name": "MIDI Per Note Channel 3",
        "kind": "toggle",
        "min": 0,
        "max": 1,
        "options": [
          {
            "value": 0,
            "label": "Off"
          },
          {
            "value": 1,
            "label": "On"
          }
        ],
        "description": "Split Left MIDI Per Note Channel 3 (0: Off, 1: On)"
      },
      {
        "number": 5,
        "group": "Split Left",
        "name": "MIDI Per Note Channel 4",
        "kind": "toggle",
        "min": 0,
        "max": 1,
        "options": [
          {
            "value": 0,
            "label": "Off"
          },
          {
            "value": 1,
            "label": "On"
          }
        ],
        "description": "Split Left MIDI Per Note Channel 4 (0: Off, 1: On)"
      },
      {
        "number": 6,
        "group": "Split Left",
        "name": "MIDI Per Note Channel 5",
        "kind": "toggle",
        "min": 0,
        "max": 1,
        "options": [
          {
            "value": 0,
            "label": "Off"
          },
          {
            "value": 1,
            "label": "On"
          }
        ],
        "description": "Split Left MIDI Per Note Channel 5 (0: Off, 1: On)"
      },
      {
        "number": 7,
        "group": "Split Left",
        "name": "MIDI Per Note Channel 6",
        "kind": "toggle",
        "min": 0,
        "max": 1,
        "options": [
          {
            "value": 0,
            "label": "Off"
          },
          {
            "value": 1,
            "label": "On"
          }
        ],
        "description": "Split Left MIDI Per Note Channel 6 (0: Off, 1: On)"
      },
      {
        "number": 8,
        "group": "Split Left",
        "name": "MIDI Per Note Channel 7",
        "kind": "toggle",
        "min": 0,
        "max": 1,
        "options": [
          {
            "value": 0,
            "label": "Off"
          },
          {
            "value": 1,
            "label": "On"
          }
        ],
        "description": "Split Left MIDI Per Note Channel 7 (0: Off, 1: On)"
      },
      {
        "number": 9,
        "group": "Split Left",
        "name": "MIDI Per Note Channel 8",
        "kind": "toggle",
        "min": 0,
        "max": 1,
        "options": [
          {
            "value": 0,
            "label": "Off"
          },
          {
            "value": 1,
            "label": "On"
          }
        ],
        "description": "Split Left MIDI Per Note Channel 8 (0: Off, 1: On)"
      },
      {
        "number": 10,
        "group": "Split Left",
        "name": "MIDI Per Note Channel 9",
        "kind": "toggle",
        "min": 0,
        "max": 1,
        "options": [
          {
            "value": 0,
            "label": "Off"
          },
          {
            "value": 1,
            "label": "On"
          }
        ],
        "description": "Split Left MIDI Per Note Channel 9 (0: Off, 1: On)"
      },
      {
        "number": 11,
        "group": "Split Left",
        "name": "MIDI Per Note Channel 10",
        "kind": "toggle",
        "min": 0,
        "max": 1,
        "options": [
          {
            "value": 0,
            "label": "Off"
          },
          {
            "value": 1,
            "label": "On"
          }
        ],
        "description": "Split Left MIDI Per Note Channel 10 (0: Off, 1: On)"
      },
      {
        "number": 12,
        "group": "Split Left",
        "name": "MIDI Per Note Channel 11",
        "kind": "toggle",
        "min": 0,
        "max": 1,
        "options": [
          {
            "value": 0,
            "label": "Off"
          },
          {
            "value": 1,
            "label": "On"
          }
        ],
        "description": "Split Left MIDI Per Note Channel 11 (0: Off, 1: On)"
      },
      {
        "number": 13,
        "group": "Split Left",
        "name": "MIDI Per Note Channel 12",
        "kind": "toggle",
        "min": 0,
        "max": 1,
        "options": [
          {
            "value": 0,
            "label": "Off"
          },
          {
            "value": 1,
            "label": "On"
          }
        ],
        "description": "Split Left MIDI Per Note Channel 12 (0: Off, 1: On)"
      },
      {
        "number": 14,
        "group": "Split Left",
        "name": "MIDI Per Note Channel 13",
        "kind": "toggle",
        "min": 0,
        "max": 1,
        "options": [
          {
            "value": 0,
            "label": "Off"
          },
          {
            "value": 1,
            "label": "On"
          }
        ],
        "description": "Split Left MIDI Per Note Channel 13 (0: Off, 1: On)"
      },
      {
        "number": 15,
        "group": "Split Left",
        "name": "MIDI Per Note Channel 14",
        "kind": "toggle",
        "min": 0,
        "max": 1,
        "options": [
          {
            "value": 0,
            "label": "Off"
          },
          {
            "value": 1,
            "label": "On"
          }
        ],
        "description": "Split Left MIDI Per Note Channel 14 (0: Off, 1: On)"
      },
      {
        "number": 16,
        "group": "Split Left",
        "name": "MIDI Per Note Channel 15",
        "kind": "toggle",
        "min": 0,
        "max": 1,
        "options": [
          {
            "value": 0,
            "label": "Off"
          },
          {
            "value": 1,
            "label": "On"
          }
        ],
        "description": "Split Left MIDI Per Note Channel 15 (0: Off, 1: On)"
      },
      {
        "number": 17,
        "group": "Split Left",
        "name": "MIDI Per Note Channel 16",
        "kind": "toggle",
        "min": 0,
        "max": 1,
        "options": [
          {
            "value": 0,
            "label": "Off"
          },
          {
            "value": 1,
            "label": "On"
          }
        ],
        "description": "Split Left MIDI Per Note Channel 16 (0: Off, 1: On)"
      },
      {
        "number": 18,
        "group": "Split Left",
        "name": "MIDI Per Row Lowest Channel",
        "kind": "number",
        "min": 1,
        "max": 16,
        "description": "Split Left MIDI Per Row Lowest Channel"
      },
      {
        "number": 19,
        "group": "Split Left",
        "name": "MIDI Bend Range",
        "kind": "number",
        "min": 1,
        "max": 96,
        "description": "Split Left MIDI Bend Range"
      },
      {
        "number": 20,
        "group": "Split Left",
        "name": "Send X",
        "kind": "toggle",
        "min": 0,
        "max": 1,
        "options": [
          {
            "value": 0,
            "label": "Off"
          },
          {
            "value": 1,
            "label": "On"
          }
        ],
        "description": "Split Left Send X (0: Off, 1: On)"
      },
      {
        "number": 21,
        "group": "Split Left",
        "name": "Pitch Quantize",
        "kind": "toggle",
        "min": 0,
        "max": 1,
        "options": [
          {
            "value": 0,
            "label": "Off"
          },
          {
            "value": 1,
            "label": "On"
          }
        ],
        "description": "Split Left Pitch Quantize (0: Off, 1: On)"
      },
      {
        "number": 22,
        "group": "Split Left",
        "name": "Pitch Quantize Hold",
        "kind": "enum",
        "min": 0,
        "max": 3,
        "options": [
          {
            "value": 0,
            "label": "Off"
          },
          {
            "value": 1,
            "label": "Medium"
          },
          {
            "value": 2,
            "label": "Fast"
          },
          {
            "value": 3,
            "label": "Slow"
          }
        ],
        "description": "Split Left Pitch Quantize Hold (0: Off, 1: Medium, 2: Fast, 3: Slow)"
      },
      {
        "number": 23,
        "group": "Split Left",
        "name": "Pitch Reset On Release",
        "kind": "toggle",
        "min": 0,
        "max": 1,
        "options": [
          {
            "value": 0,
            "label": "Off"
          },
          {
            "value": 1,
            "label": "On"
          }
        ],
        "description": "Split Left Pitch Reset On Release (unprinted parameter, 0: Off, 1: On)"
      },
      {
        "number": 24,
        "group": "Split Left",
        "name": "Send Y",
        "kind": "toggle",
        "min": 0,
        "max": 1,
        "options": [
          {
            "value": 0,
            "label": "Off"
          },
          {
            "value": 1,
            "label": "On"
          }
        ],
        "description": "Split Left Send Y (0: Off, 1: On)"
      },
      {
        "number": 25,
        "group": "Split Left",
        "name": "MIDI CC For Y",
        "kind": "number",
        "min": 0,
        "max": 127,
        "note": "CC 1 or CC 74 are recommended, any CC is possible though",
        "description": "Split Left MIDI CC For Y (CC 1 or CC 74 are recommended, any CC is possible though)"
      },
      {
        "number": 26,
        "group": "Split Left",
        "name": "Relative Y",
        "kind": "toggle",
        "min": 0,
        "max": 1,
        "options": [
          {
            "value": 0,
            "label": "Off"
          },
          {
            "value": 1,
            "label": "On"
          }
        ],
        "description": "Split Left Relative Y (0: Off, 1: On)"
      },
      {
        "number": 27,
        "group": "Split Left",
        "name": "Send Z",
        "kind": "toggle",
        "min": 0,
        "max": 1,
        "options": [
          {
            "value": 0,
            "label": "Off"
          },
          {
            "value": 1,
            "label": "On"
          }
        ],
        "description": "Split Left Send Z (0: Off, 1: On)"
      },
      {
        "number": 28,
        "group": "Split Left",
        "name": "MIDI Expression For Z",
        "kind": "enum",
        "min": 0,
        "max": 2,
        "options": [
          {
            "value": 0,
            "label": "Poly Aftertouch"
          },
          {
            "value": 1,
            "label": "Channel Aftertouch"
          },
          {
            "value": 2,
            "label": "CC defined in #29"
          }
        ],
        "description": "Split Left MIDI Expression For Z (0: Poly Aftertouch, 1: Channel Aftertouch, 2: CC defined in #29)"
      },
      {
        "number": 29,
        "group": "Split Left",
        "name": "MIDI CC For Z",
        "kind": "number",
        "min": 0,
        "max": 127,
        "note": "CC 11 is recommended, any CC is possible though",
        "description": "Split Left MIDI CC For Z (CC 11 is recommended, any CC is possible though)"
      },
      {
        "number": 30,
        "group": "Split Left",
        "name": "Color Main",
        "kind": "enum",
        "min": 1,
        "max": 11,
        "options": [
          {
            "value": 1,
            "label": "Red"
          },
          {
            "value": 2,
            "label": "Yellow"
          },
          {
            "value": 3,
            "label": "Green"
          },
          {
            "value": 4,
            "label": "Cyan"
          },
          {
            "value": 5,
            "label": "Blue"
          },
          {
            "value": 6,
            "label": "Magenta"
          },
          {
            "value": 7,
            "label": "Off"
          },
          {
            "value": 8,
            "label": "White"
          },
          {
            "value": 9,
            "label": "Orange"
          },
          {
            "value": 10,
            "label": "Lime"
          },
          {
            "value": 11,
            "label": "Pink"
          }
        ],
        "description": "Split Left Color Main (see color value table below)"
      },
      {
        "number": 31,
        "group": "Split Left",
        "name": "Color Accent",
        "kind": "enum",
        "min": 1,
        "max": 11,
        "options": [
          {
            "value": 1,
            "label": "Red"
          },
          {
            "value": 2,
            "label": "Yellow"
          },
          {
            "value": 3,
            "label": "Green"
          },
          {
            "value": 4,
            "label": "Cyan"
          },
          {
            "value": 5,
            "label": "Blue"
          },
          {
            "value": 6,
            "label": "Magenta"
          },
          {
            "value": 7,
            "label": "Off"
          },
          {
            "value": 8,
            "label": "White"
          },
          {
            "value": 9,
            "label": "Orange"
          },
          {
            "value": 10,
            "label": "Lime"
          },
          {
            "value": 11,
            "label": "Pink"
          }
        ],
        "description": "Split Left Color Accent (see color value table below)"
      },
      {
        "number": 32,
        "group": "Split Left",
        "name": "Color Played",
        "kind": "enum",
        "min": 0,
        "max": 11,
        "options": [
          {
            "value": 0,
            "label": "as set in Note Lights settings"
          },
          {
            "value": 1,
            "label": "Red"
          },
          {
            "value": 2,
            "label": "Yellow"
          },
          {
            "value": 3,
            "label": "Green"
          },
          {
            "value": 4,
            "label": "Cyan"
          },
          {
            "value": 5,
            "label": "Blue"
          },
          {
            "value": 6,
            "label": "Magenta"
          },
          {
            "value": 7,
            "label": "Off"
          },
          {
            "value": 8,
            "label": "White"
          },
          {
            "value": 9,
            "label": "Orange"
          },
          {
            "value": 10,
            "label": "Lime"
          },
          {
            "value": 11,
            "label": "Pink"
          }
        ],
        "description": "Split Left Color Played (see color value table below)"
      },
      {
        "number": 33,
        "group": "Split Left",
        "name": "Color LowRow",
        "kind": "enum",
        "min": 1,
        "max": 11,
        "options": [
          {
            "value": 1,
            "label": "Red"
          },
          {
            "value": 2,
            "label": "Yellow"
          },
          {
            "value": 3,
            "label": "Green"
          },
          {
            "value": 4,
            "label": "Cyan"
          },
          {
            "value": 5,
            "label": "Blue"
          },
          {
            "value": 6,
            "label": "Magenta"
          },
          {
            "value": 7,
            "label": "Off"
          },
          {
            "value": 8,
            "label": "White"
          },
          {
            "value": 9,
            "label": "Orange"
          },
          {
            "value": 10,
            "label": "Lime"
          },
          {
            "value": 11,
            "label": "Pink"
          }
        ],
        "description": "Split Left Color LowRow (see color value table below)"
      },
      {
        "number": 34,
        "group": "Split Left",
        "name": "LowRow Mode",
        "kind": "enum",
        "min": 0,
        "max": 7,
        "options": [
          {
            "value": 0,
            "label": "Off"
          },
          {
            "value": 1,
            "label": "Sust"
          },
          {
            "value": 2,
            "label": "Restr"
          },
          {
            "value": 3,
            "label": "Strum"
          },
          {
            "value": 4,
            "label": "Arp"
          },
          {
            "value": 5,
            "label": "Bend"
          },
          {
            "value": 6,
            "label": "CC1"
          },
          {
            "value": 7,
            "label": "CC16-18"
          }
        ],
        "description": "Split Left LowRow Mode (0: Off, 1: Sust, 2: Restr, 3: Strum, 4: Arp, 5: Bend, 6: CC1, 7: CC16-18)"
      },
      {
        "number": 35,
        "group": "Split Left",
        "name": "Special",
        "kind": "enum",
        "min": 0,
        "max": 4,
        "options": [
          {
            "value": 0,
            "label": "Off"
          },
          {
            "value": 1,
            "label": "Arpeg"
          },
          {
            "value": 2,
            "label": "CC Faders"
          },
          {
            "value": 3,
            "label": "Strum"
          },
          {
            "value": 4,
            "label": "Sequencer"
          }
        ],
        "description": "Split Left Special (0: Off, 1: Arpeg, 2: CC Faders, 3: Strum, 4: Sequencer)"
      },
      {
        "number": 36,
        "group": "Split Left",
        "name": "Octave",
        "kind": "enum",
        "min": 0,
        "max": 10,
        "options": [
          {
            "value": 0,
            "label": "-5"
          },
          {
            "value": 1,
            "label": "-4"
          },
          {
            "value": 2,
            "label": "-3"
          },
          {
            "value": 3,
            "label": "-2"
          },
          {
            "value": 4,
            "label": "-1"
          },
          {
            "value": 5,
            "label": "0"
          },
          {
            "value": 6,
            "label": "+1"
          },
          {
            "value": 7,
            "label": "+2"
          },
          {
            "value": 8,
            "label": "+3"
          },
          {
            "value": 9,
            "label": "+4"
          },
          {
            "value": 10,
            "label": "+5"
          }
        ],
        "description": "Split Left Octave (0: -5, 1: -4, 2: -3, 3: -2, 4: -1, 5: 0, 6: +1, 7: +2, 8: +3, 9: +4, 10: +5)"
      },
      {
        "number": 37,
        "group": "Split Left",
        "name": "Transpose Pitch",
        "kind": "enum",
        "min": 0,
        "max": 14,
        "options": [
          {
            "value": 0,
            "label": "-7"
          },
          {
            "value": 1,
            "label": "-6"
          },
          {
            "value": 2,
            "label": "-5"
          },
          {
            "value": 3,
            "label": "-4"
          },
          {
            "value": 4,
            "label": "-3"
          },
          {
            "value": 5,
            "label": "-2"
          },
          {
            "value": 6,
            "label": "-1"
          },
          {
            "value": 7,
            "label": "0"
          },
          {
            "value": 8,
            "label": "+1"
          },
          {
            "value": 9,
            "label": "+2"
          },
          {
            "value": 10,
            "label": "+3"
          },
          {
            "value": 11,
            "label": "+4"
          },
          {
            "value": 12,
            "label": "+5"
          },
          {
            "value": 13,
            "label": "+6"
          },
          {
            "value": 14,
            "label": "+7"
          }
        ],
        "description": "Split Left Transpose Pitch (0-6: -7 to -1, 7: 0, 8-14: +1 to +7)"
      },
      {
        "number": 38,
        "group": "Split Left",
        "name": "Transpose Lights",
        "kind": "enum",
        "min": 0,
        "max": 14,
        "options": [
          {
            "value": 0,
            "label": "-7"
          },
          {
            "value": 1,
            "label": "-6"
          },
          {
            "value": 2,
            "label": "-5"
          },
          {
            "value": 3,
            "label": "-4"
          },
          {
            "value": 4,
            "label": "-3"
          },
          {
            "value": 5,
            "label": "-2"
          },
          {
            "value": 6,
            "label": "-1"
          },
          {
            "value": 7,
            "label": "0"
          },
          {
            "value": 8,
            "label": "+1"
          },
          {
            "value": 9,
            "label": "+2"
          },
          {
            "value": 10,
            "label": "+3"
          },
          {
            "value": 11,
            "label": "+4"
          },
          {
            "value": 12,
            "label": "+5"
          },
          {
            "value": 13,
            "label": "+6"
          },
          {
            "value": 14,
            "label": "+7"
          }
        ],
        "description": "Split Left Transpose Lights (0-6: -7 to -1, 7: 0, 8-14: +1 to +7)"
      },
      {
        "number": 39,
        "group": "Split Left",
        "name": "MIDI Expression For Y",
        "kind": "enum",
        "min": 0,
        "max": 2,
        "options": [
          {
            "value": 0,
            "label": "Poly Aftertouch"
          },
          {
            "value": 1,
            "label": "Channel Aftertouch"
          },
          {
            "value": 2,
            "label": "CC defined in #25"
          }
        ],
        "description": "Split Left MIDI Expression For Y (0: Poly Aftertouch, 1: Channel Aftertouch, 2: CC defined in #25)"
      },
      {
        "number": 40,
        "group": "Split Left",
        "name": "MIDI CC For Fader 1",
        "kind": "number",
        "min": 0,
        "max": 128,
        "note": "0-127: CC, 128: Channel Aftertouch",
        "description": "Split Left MIDI CC For Fader 1 (0-127: CC, 128: Channel Aftertouch)"
      },
      {
        "number": 41,
        "group": "Split Left",
        "name": "MIDI CC For Fader 2",
        "kind": "number",
        "min": 0,
        "max": 128,
        "note": "0-127: CC, 128: Channel Aftertouch",
        "description": "Split Left MIDI CC For Fader 2 (0-127: CC, 128: Channel Aftertouch)"
      },
      {
        "number": 42,
        "group": "Split Left",
        "name": "MIDI CC For Fader 3",
        "kind": "number",
        "min": 0,
        "max": 128,
        "note": "0-127: CC, 128: Channel Aftertouch",
        "description": "Split Left MIDI CC For Fader 3 (0-127: CC, 128: Channel Aftertouch)"
      },
      {
        "number": 43,
        "group": "Split Left",
        "name": "MIDI CC For Fader 4",
        "kind": "number",
        "min": 0,
        "max": 128,
        "note": "0-127: CC, 128: Channel Aftertouch",
        "description": "Split Left MIDI CC For Fader 4 (0-127: CC, 128: Channel Aftertouch)"
      },
      {
        "number": 44,
        "group": "Split Left",
        "name": "MIDI CC For Fader 5",
        "kind": "number",
        "min": 0,
        "max": 128,
        "note": "0-127: CC, 128: Channel Aftertouch",
        "description": "Split Left MIDI CC For Fader 5 (0-127: CC, 128: Channel Aftertouch)"
      },
      {
        "number": 45,
        "group": "Split Left",
        "name": "MIDI CC For Fader 6",
        "kind": "number",
        "min": 0,
        "max": 128,
        "note": "0-127: CC, 128: Channel Aftertouch",
        "description": "Split Left MIDI CC For Fader 6 (0-127: CC, 128: Channel Aftertouch)"
      },
      {
        "number": 46,
        "group": "Split Left",
        "name": "MIDI CC For Fader 7",
        "kind": "number",
        "min": 0,
        "max": 128,
        "note": "0-127: CC, 128: Channel Aftertouch",
        "description": "Split Left MIDI CC For Fader 7 (0-127: CC, 128: Channel Aftertouch)"
      },
      {
        "number": 47,
        "group": "Split Left",
        "name": "MIDI CC For Fader 8",
        "kind": "number",
        "min": 0,
        "max": 128,
        "note": "0-127: CC, 128: Channel Aftertouch",
        "description": "Split Left MIDI CC For Fader 8 (0-127: CC, 128: Channel Aftertouch)"
      },
      {
        "number": 48,
        "group": "Split Left",
        "name": "LowRow X Behavior",
        "kind": "toggle",
        "min": 0,
        "max": 1,
        "options": [
          {
            "value": 0,
            "label": "Hold"
          },
          {
            "value": 1,
            "label": "Fader"
          }
        ],
        "description": "Split Left LowRow X Behavior (0: Hold, 1: Fader)"
      },
      {
        "number": 49,
        "group": "Split Left",
        "name": "MIDI CC For LowRow X",
        "kind": "number",
        "min": 0,
        "max": 128,
        "note": "0-127: CC, 128: Channel Aftertouch",
        "description": "Split Left MIDI CC For LowRow X (0-127: CC, 128: Channel Aftertouch)"
      },
      {
        "number": 50,
        "group": "Split Left",
        "name": "LowRow XYZ Behavior",
        "kind": "toggle",
        "min": 0,
        "max": 1,
        "options": [
          {
            "value": 0,
            "label": "Hold"
          },
          {
            "value": 1,
            "label": "Fader"
          }
        ],
        "description": "Split Left LowRow XYZ Behavior (0: Hold, 1: Fader)"
      },
      {
        "number": 51,
        "group": "Split Left",
        "name": "MIDI CC For LowRow XYZ X",
        "kind": "number",
        "min": 0,
        "max": 128,
        "note": "0-127: CC, 128: Channel Aftertouch",
        "description": "Split Left MIDI CC For LowRow XYZ X (0-127: CC, 128: Channel Aftertouch)"
      },
      {
        "number": 52,
        "group": "Split Left",
        "name": "MIDI CC For LowRow XYZ Y",
        "kind": "number",
        "min": 0,
        "max": 128,
        "note": "0-127: CC, 128: Channel Aftertouch",
        "description": "Split Left MIDI CC For LowRow XYZ Y (0-127: CC, 128: Channel Aftertouch)"
      },
      {
        "number": 53,
        "group": "Split Left",
        "name": "MIDI CC For LowRow XYZ Z",
        "kind": "number",
        "min": 0,
        "max": 128,
        "note": "0-127: CC, 128: Channel Aftertouch",
        "description": "Split Left MIDI CC For LowRow XYZ Z (0-127: CC, 128: Channel Aftertouch)"
      },
      {
        "number": 54,
        "group": "Split Left",
        "name": "Minimum CC Value For Y",
        "kind": "number",
        "min": 0,
        "max": 127,
        "description": "Split Left Minimum CC Value For Y"
      },
      {
        "number": 55,
        "group": "Split Left",
        "name": "Maximum CC Value For Y",
        "kind": "number",
        "min": 0,
        "max": 127,
        "description": "Split Left Maximum CC Value For Y"
      },
      {
        "number": 56,
        "group": "Split Left",
        "name": "Minimum CC Value For Z",
        "kind": "number",
        "min": 0,
        "max": 127,
        "description": "Split Left Minimum CC Value For Z"
      },
      {
        "number": 57,
        "group": "Split Left",
        "name": "Maximum CC Value For Z",
        "kind": "number",
        "min": 0,
        "max": 127,
        "description": "Split Left Maximum CC Value For Z"
      },
      {
        "number": 58,
        "group": "Split Left",
        "name": "CC Value For Z in 14-bit",
        "kind": "number",
        "min": 0,
        "max": 1,
        "description": "Split Left CC Value For Z in 14-bit"
      },
      {
        "number": 59,
        "group": "Split Left",
        "name": "Initial Value For Relative Y",
        "kind": "number",
        "min": 0,
        "max": 127,
        "description": "Split Left Initial Value For Relative Y"
      },
      {
        "number": 60,
        "group": "Split Left",
        "name": "Channel Per Row MIDI Channel Order",
        "kind": "toggle",
        "min": 0,
        "max": 1,
        "options": [
          {
            "value": 0,
            "label": "Normal"
          },
          {
            "value": 1,
            "label": "Reversed"
          }
        ],
        "description": "Split Left Channel Per Row MIDI Channel Order (0: Normal, 1: Reversed)"
      },
      {
        "number": 61,
        "group": "Split Left",
        "name": "Touch Animation",
        "kind": "enum",
        "min": 0,
        "max": 14,
        "options": [
          {
            "value": 0,
            "label": "Same"
          },
          {
            "value": 1,
            "label": "Crosses"
          },
          {
            "value": 2,
            "label": "Circles"
          },
          {
            "value": 3,
            "label": "Squares"
          },
          {
            "value": 4,
            "label": "Diamonds"
          },
          {
            "value": 5,
            "label": "Stars"
          },
          {
            "value": 6,
            "label": "Sparkles"
          },
          {
            "value": 7,
            "label": "Curtains"
          },
          {
            "value": 8,
            "label": "Blinds"
          },
          {
            "value": 9,
            "label": "Targets"
          },
          {
            "value": 10,
            "label": "Up"
          },
          {
            "value": 11,
            "label": "Down"
          },
          {
            "value": 12,
            "label": "Left"
          },
          {
            "value": 13,
            "label": "Right"
          },
          {
            "value": 14,
            "label": "Orbits"
          }
        ],
        "description": "Split Left Touch Animation, see the touch animation values below"
      },
      {
        "number": 62,
        "group": "Split Left",
        "name": "Sequencer Toggle Play",
        "kind": "action",
        "min": 1,
        "max": 1,
        "description": "Split Left Sequencer Toggle Play"
      },
      {
        "number": 63,
        "group": "Split Left",
        "name": "Sequencer Previous Pattern",
        "kind": "action",
        "min": 1,
        "max": 1,
        "description": "Split Left Sequencer Previous Pattern"
      },
      {
        "number": 64,
        "group": "Split Left",
        "name": "Sequencer Next Pattern",
        "kind": "action",
        "min": 1,
        "max": 1,
        "description": "Split Left Sequencer Next Pattern"
      },
      {
        "number": 65,
        "group": "Split Left",
        "name": "Sequencer Select Pattern Number",
        "kind": "number",
        "min": 0,
        "max": 3,
        "description": "Split Left Sequencer Select Pattern Number"
      },
      {
        "number": 66,
        "group": "Split Left",
        "name": "Sequencer Toggle Mute",
        "kind": "action",
        "min": 1,
        "max": 1,
        "description": "Split Left Sequencer Toggle Mute"
      },
      {
        "number": 100,
        "group": "Split Right",
        "name": "MIDI Mode",
        "kind": "enum",
        "min": 0,
        "max": 2,
        "options": [
          {
            "value": 0,
            "label": "One Channel"
          },
          {
            "value": 1,
            "label": "Channel Per Note"
          },
          {
            "value": 2,
            "label": "Channel Per Row"
          }
        ],
        "description": "Split Right MIDI Mode (0: One Channel, 1: Channel Per Note, 2: Channel Per Row)"
      },
      {
        "number": 101,
        "group": "Split Right",
        "name": "MIDI Main Channel",
        "kind": "number",
        "min": 1,
        "max": 16,
        "description": "Split Right MIDI Main Channel"
      },
      {
        "number": 102,
        "group": "Split Right",
        "name": "MIDI Per Note Channel 1",
        "kind": "toggle",
        "min": 0,
        "max": 1,
        "options": [
          {
            "value": 0,
            "label": "Off"
          },
          {
            "value": 1,
            "label": "On"
          }
        ],
        "description": "Split Right MIDI Per Note Channel 1 (0: Off, 1: On)"
      },
      {
        "number": 103,
        "group": "Split Right",
        "name": "MIDI Per Note Channel 2",
        "kind": "toggle",
        "min": 0,
        "max": 1,
        "options": [
          {
            "value": 0,
            "label": "Off"
          },
          {
            "value": 1,
            "label": "On"
          }
        ],
        "description": "Split Right MIDI Per Note Channel 2 (0: Off, 1: On)"
      },
      {
        "number": 104,
        "group": "Split Right",
        "name": "MIDI Per Note Channel 3",
        "kind": "toggle",
        "min": 0,
        "max": 1,
        "options": [
          {
            "value": 0,
            "label": "Off"
          },
          {
            "value": 1,
            "label": "On"
          }
        ],
        "description": "Split Right MIDI Per Note Channel 3 (0: Off, 1: On)"
      },
      {
        "number": 105,
        "group": "Split Right",
        "name": "MIDI Per Note Channel 4",
        "kind": "toggle",
        "min": 0,
        "max": 1,
        "options": [
          {
            "value": 0,
            "label": "Off"
          },
          {
            "value": 1,
            "label": "On"
          }
        ],
        "description": "Split Right MIDI Per Note Channel 4 (0: Off, 1: On)"
      },
      {
        "number": 106,
        "group": "Split Right",
        "name": "MIDI Per Note Channel 5",
        "kind": "toggle",
        "min": 0,
        "max": 1,
        "options": [
          {
            "value": 0,
            "label": "Off"
          },
          {
            "value": 1,
            "label": "On"
          }
        ],
        "description": "Split Right MIDI Per Note Channel 5 (0: Off, 1: On)"
      },
      {
        "number": 107,
        "group": "Split Right",
        "name": "MIDI Per Note Channel 6",
        "kind": "toggle",
        "min": 0,
        "max": 1,
        "options": [
          {
            "value": 0,
            "label": "Off"
          },
          {
            "value": 1,
            "label": "On"
          }
        ],
        "description": "Split Right MIDI Per Note Channel 6 (0: Off, 1: On)"
      },
      {
        "number": 108,
        "group": "Split Right",
        "name": "MIDI Per Note Channel 7",
        "kind": "toggle",
        "min": 0,
        "max": 1,
        "options": [
          {
            "value": 0,
            "label": "Off"
          },
          {
            "value": 1,
            "label": "On"
          }
        ],
        "description": "Split Right MIDI Per Note Channel 7 (0: Off, 1: On)"
      },
      {
        "number": 109,
        "group": "Split Right",
        "name": "MIDI Per Note Channel 8",
        "kind": "toggle",
        "min": 0,
        "max": 1,
        "options": [
          {
            "value": 0,
            "label": "Off"
          },
          {
            "value": 1,
            "label": "On"
          }
        ],
        "description": "Split Right MIDI Per Note Channel 8 (0: Off, 1: On)"
      },
      {
        "number": 110,
        "group": "Split Right",
        "name": "MIDI Per Note Channel 9",
        "kind": "toggle",
        "min": 0,
        "max": 1,
        "options": [
          {
            "value": 0,
            "label": "Off"
          },
          {
            "value": 1,
            "label": "On"
          }
        ],
        "description": "Split Right MIDI Per Note Channel 9 (0: Off, 1: On)"
      },
      {
        "number": 111,
        "group": "Split Right",
        "name": "MIDI Per Note Channel 10",
        "kind": "toggle",
        "min": 0,
        "max": 1,
        "options": [
          {
            "value": 0,
            "label": "Off"
          },
          {
            "value": 1,
            "label": "On"
          }
        ],
        "description": "Split Right MIDI Per Note Channel 10 (0: Off, 1: On)"
      },
      {
        "number": 112,
        "group": "Split Right",
        "name": "MIDI Per Note Channel 11",
        "kind": "toggle",
        "min": 0,
        "max": 1,
        "options": [
          {
            "value": 0,
            "label": "Off"
          },
          {
            "value": 1,
            "label": "On"
          }
        ],
        "description": "Split Right MIDI Per Note Channel 11 (0: Off, 1: On)"
      },
      {
        "number": 113,
        "group": "Split Right",
        "name": "MIDI Per Note Channel 12",
        "kind": "toggle",
        "min": 0,
        "max": 1,
        "options": [
          {
            "value": 0,
            "label": "Off"
          },
          {
            "value": 1,
            "label": "On"
          }
        ],
        "description": "Split Right MIDI Per Note Channel 12 (0: Off, 1: On)"
      },
      {
        "number": 114,
        "group": "Split Right",
        "name": "MIDI Per Note Channel 13",
        "kind": "toggle",
        "min": 0,
        "max": 1,
        "options": [
          {
            "value": 0,
            "label": "Off"
          },
          {
            "value": 1,
            "label": "On"
          }
        ],
        "description": "Split Right MIDI Per Note Channel 13 (0: Off, 1: On)"
      },
      {
        "number": 115,
        "group": "Split Right",
        "name": "MIDI Per Note Channel 14",
        "kind": "toggle",
        "min": 0,
        "max": 1,
        "options": [
          {
            "value": 0,
            "label": "Off"
          },
          {
            "value": 1,
            "label": "On"
          }
        ],
        "description": "Split Right MIDI Per Note Channel 14 (0: Off, 1: On)"
      },
      {
        "number": 116,
        "group": "Split Right",
        "name": "MIDI Per Note Channel 15",
        "kind": "toggle",
        "min": 0,
        "max": 1,
        "options": [
          {
            "value": 0,
            "label": "Off"
          },
          {
            "value": 1,
            "label": "On"
          }
        ],
        "description": "Split Right MIDI Per Note Channel 15 (0: Off, 1: On)"
      },
      {
        "number": 117,
        "group": "Split Right",
        "name": "MIDI Per Note Channel 16",
        "kind": "toggle",
        "min": 0,
        "max": 1,
        "options": [
          {
            "value": 0,
            "label": "Off"
          },
          {
            "value": 1,
            "label": "On"
          }
        ],
        "description": "Split Right MIDI Per Note Channel 16 (0: Off, 1: On)"
      },
      {
        "number": 118,
        "group": "Split Right",
        "name": "MIDI Per Row Lowest Channel",
        "kind": "number",
        "min": 1,
        "max": 16,
        "description": "Split Right MIDI Per Row Lowest Channel"
      },
      {
        "number": 119,
        "group": "Split Right",
        "name": "MIDI Bend Range",
        "kind": "number",
        "min": 1,
        "max": 96,
        "description": "Split Right MIDI Bend Range"
      },
      {
        "number": 120,
        "group": "Split Right",
        "name": "Send X",
        "kind": "toggle",
        "min": 0,
        "max": 1,
        "options": [
          {
            "value": 0,
            "label": "Off"
          },
          {
            "value": 1,
            "label": "On"
          }
        ],
        "description": "Split Right Send X (0: Off, 1: On)"
      },
      {
        "number": 121,
        "group": "Split Right",
        "name": "Pitch Quantize",
        "kind": "toggle",
        "min": 0,
        "max": 1,
        "options": [
          {
            "value": 0,
            "label": "Off"
          },
          {
            "value": 1,
            "label": "On"
          }
        ],
        "description": "Split Right Pitch Quantize (0: Off, 1: On)"
      },
      {
        "number": 122,
        "group": "Split Right",
        "name": "Pitch Quantize Hold",
        "kind": "enum",
        "min": 0,
        "max": 3,
        "options": [
          {
            "value": 0,
            "label": "Off"
          },
          {
            "value": 1,
            "label": "Medium"
          },
          {
            "value": 2,
            "label": "Fast"
          },
          {
            "value": 3,
            "label": "Slow"
          }
        ],
        "description": "Split Right Pitch Quantize Hold (0: Off, 1: Medium, 2: Fast, 3: Slow)"
      },
      {
        "number": 123,
        "group": "Split Right",
        "name": "Pitch Reset On Release",
        "kind": "toggle",
        "min": 0,
        "max": 1,
        "options": [
          {
            "value": 0,
            "label": "Off"
          },
          {
            "value": 1,
            "label": "On"
          }
        ],
        "description": "Split Right Pitch Reset On Release (unprinted parameter, 0: Off, 1: On)"
      },
      {
        "number": 124,
        "group": "Split Right",
        "name": "Send Y",
        "kind": "toggle",
        "min": 0,
        "max": 1,
        "options": [
          {
            "value": 0,
            "label": "Off"
          },
          {
            "value": 1,
            "label": "On"
          }
        ],
        "description": "Split Right Send Y (0: Off, 1: On)"
      },
      {
        "number": 125,
        "group": "Split Right",
        "name": "MIDI CC For Y",
        "kind": "number",
        "min": 0,
        "max": 127,
        "note": "CC 1 or CC 74 are recommended, any CC is possible though",
        "description": "Split Right MIDI CC For Y (CC 1 or CC 74 are recommended, any CC is possible though)"
      },
      {
        "number": 126,
        "group": "Split Right",
        "name": "Relative Y",
        "kind": "toggle",
        "min": 0,
        "max": 1,
        "options": [
          {
            "value": 0,
            "label": "Off"
          },
          {
            "value": 1,
            "label": "On"
          }
        ],
        "description": "Split Right Relative Y (0: Off, 1: On)"
      },
      {
        "number": 127,
        "group": "Split Right",
        "name": "Send Z",
        "kind": "toggle",
        "min": 0,
        "max": 1,
        "options": [
          {
            "value": 0,
            "label": "Off"
          },
          {
            "value": 1,
            "label": "On"
          }
        ],
        "description": "Split Right Send Z (0: Off, 1: On)"
      },
      {
        "number": 128,
        "group": "Split Right",
        "name": "MIDI Expression For Z",
        "kind": "enum",
        "min": 0,
        "max": 2,
        "options": [
          {
            "value": 0,
            "label": "Poly Aftertouch"
          },
          {
            "value": 1,
            "label": "Channel Aftertouch"
          },
          {
            "value": 2,
            "label": "CC defined in #129"
          }
        ],
        "description": "Split Right MIDI Expression For Z (0: Poly Aftertouch, 1: Channel Aftertouch, 2: CC defined in #129)"
      },
      {
        "number": 129,
        "group": "Split Right",
        "name": "MIDI CC For Z",
        "kind": "number",
        "min": 0,
        "max": 127,
        "note": "CC 11 is recommended, any CC is possible though",
        "description": "Split Right MIDI CC For Z (CC 11 is recommended, any CC is possible though)"
      },
      {
        "number": 130,
        "group": "Split Right",
        "name": "Color Main",
        "kind": "enum",
        "min": 1,
        "max": 11,
        "options": [
          {
            "value": 1,
            "label": "Red"
          },
          {
            "value": 2,
            "label": "Yellow"
          },
          {
            "value": 3,
            "label": "Green"
          },
          {
            "value": 4,
            "label": "Cyan"
          },
          {
            "value": 5,
            "label": "Blue"
          },
          {
            "value": 6,
            "label": "Magenta"
          },
          {
            "value": 7,
            "label": "Off"
          },
          {
            "value": 8,
            "label": "White"
          },
          {
            "value": 9,
            "label": "Orange"
          },
          {
            "value": 10,
            "label": "Lime"
          },
          {
            "value": 11,
            "label": "Pink"
          }
        ],
        "description": "Split Right Color Main (see color value table below)"
      },
      {
        "number": 131,
        "group": "Split Right",
        "name": "Color Accent",
        "kind": "enum",
        "min": 1,
        "max": 11,
        "options": [
          {
            "value": 1,
            "label": "Red"
          },
          {
            "value": 2,
            "label": "Yellow"
          },
          {
            "value": 3,
            "label": "Green"
          },
          {
            "value": 4,
            "label": "Cyan"
          },
          {
            "value": 5,
            "label": "Blue"
          },
          {
            "value": 6,
            "label": "Magenta"
          },
          {
            "value": 7,
            "label": "Off"
          },
          {
            "value": 8,
            "label": "White"
          },
          {
            "value": 9,
            "label": "Orange"
          },
          {
            "value": 10,
            "label": "Lime"
          },
          {
            "value": 11,
            "label": "Pink"
          }
        ],
        "description": "Split Right Color Accent (see color value table below)"
      },
      {
        "number": 132,
        "group": "Split Right",
        "name": "Color Played",
        "kind": "enum",
        "min": 0,
        "max": 11,
        "options": [
          {
            "value": 0,
            "label": "as set in Note Lights settings"
          },
          {
            "value": 1,
            "label": "Red"
          },
          {
            "value": 2,
            "label": "Yellow"
          },
          {
            "value": 3,
            "label": "Green"
          },
          {
            "value": 4,
            "label": "Cyan"
          },
          {
            "value": 5,
            "label": "Blue"
          },
          {
            "value": 6,
            "label": "Magenta"
          },
          {
            "value": 7,
            "label": "Off"
          },
          {
            "value": 8,
            "label": "White"
          },
          {
            "value": 9,
            "label": "Orange"
          },
          {
            "value": 10,
            "label": "Lime"
          },
          {
            "value": 11,
            "label": "Pink"
          }
        ],
        "description": "Split Right Color Played (see color value table below)"
      },
      {
        "number": 133,
        "group": "Split Right",
        "name": "Color LowRow",
        "kind": "enum",
        "min": 1,
        "max": 11,
        "options": [
          {
            "value": 1,
            "label": "Red"
          },
          {
            "value": 2,
            "label": "Yellow"
          },
          {
            "value": 3,
            "label": "Green"
          },
          {
            "value": 4,
            "label": "Cyan"
          },
          {
            "value": 5,
            "label": "Blue"
          },
          {
            "value": 6,
            "label": "Magenta"
          },
          {
            "value": 7,
            "label": "Off"
          },
          {
            "value": 8,
            "label": "White"
          },
          {
            "value": 9,
            "label": "Orange"
          },
          {
            "value": 10,
            "label": "Lime"
          },
          {
            "value": 11,
            "label": "Pink"
          }
        ],
        "description": "Split Right Color LowRow (see color value table below)"
      },
      {
        "number": 134,
        "group": "Split Right",
        "name": "LowRow Mode",
        "kind": "enum",
        "min": 0,
        "max": 7,
        "options": [
          {
            "value": 0,
            "label": "Off"
          },
          {
            "value": 1,
            "label": "Sust"
          },
          {
            "value": 2,
            "label": "Restr"
          },
          {
            "value": 3,
            "label": "Strum"
          },
          {
            "value": 4,
            "label": "Arp"
          },
          {
            "value": 5,
            "label": "Bend"
          },
          {
            "value": 6,
            "label": "CC1"
          },
          {
            "value": 7,
            "label": "CC16-18"
          }
        ],
        "description": "Split Right LowRow Mode (0: Off, 1: Sust, 2: Restr, 3: Strum, 4: Arp, 5: Bend, 6: CC1, 7: CC16-18)"
      },
      {
        "number": 135,
        "group": "Split Right",
        "name": "Special",
        "kind": "enum",
        "min": 0,
        "max": 4,
        "options": [
          {
            "value": 0,
            "label": "Off"
          },
          {
            "value": 1,
            "label": "Arpeg"
          },
          {
            "value": 2,
            "label": "CC Faders"
          },
          {
            "value": 3,
            "label": "Strum"
          },
          {
            "value": 4,
            "label": "Sequencer"
          }
        ],
        "description": "Split Right Special (0: Off, 1: Arpeg, 2: CC Faders, 3: Strum, 4: Sequencer)"
      },
      {
        "number": 136,
        "group": "Split Right",
        "name": "Octave",
        "kind": "enum",
        "min": 0,
        "max": 10,
        "options": [
          {
            "value": 0,
            "label": "-5"
          },
          {
            "value": 1,
            "label": "-4"
          },
          {
            "value": 2,
            "label": "-3"
          },
          {
            "value": 3,
            "label": "-2"
          },
          {
            "value": 4,
            "label": "-1"
          },
          {
            "value": 5,
            "label": "0"
          },
          {
            "value": 6,
            "label": "+1"
          },
          {
            "value": 7,
            "label": "+2"
          },
          {
            "value": 8,
            "label": "+3"
          },
          {
            "value": 9,
            "label": "+4"
          },
          {
            "value": 10,
            "label": "+5"
          }
        ],
        "description": "Split Right Octave (0: -5, 1: -4, 2: -3, 3: -2, 4: -1, 5: 0, 6: +1, 7: +2, 8: +3, 9: +4, 10: +5)"
      },
      {
        "number": 137,
        "group": "Split Right",
        "name": "Transpose Pitch",
        "kind": "enum",
        "min": 0,
        "max": 14,
        "options": [
          {
            "value": 0,
            "label": "-7"
          },
          {
            "value": 1,
            "label": "-6"
          },
          {
            "value": 2,
            "label": "-5"
          },
          {
            "value": 3,
            "label": "-4"
          },
          {
            "value": 4,
            "label": "-3"
          },
          {
            "value": 5,
            "label": "-2"
          },
          {
            "value": 6,
            "label": "-1"
          },
          {
            "value": 7,
            "label": "0"
          },
          {
            "value": 8,
            "label": "+1"
          },
          {
            "value": 9,
            "label": "+2"
          },
          {
            "value": 10,
            "label": "+3"
          },
          {
            "value": 11,
            "label": "+4"
          },
          {
            "value": 12,
            "label": "+5"
          },
          {
            "value": 13,
            "label": "+6"
          },
          {
            "value": 14,
            "label": "+7"
          }
        ],
        "description": "Split Right Transpose Pitch (0-6: -7 to -1, 7: 0, 8-14: +1 to +7)"
      },
      {
        "number": 138,
        "group": "Split Right",
        "name": "Transpose Lights",
        "kind": "enum",
        "min": 0,
        "max": 14,
        "options": [
          {
            "value": 0,
            "label": "-7"
          },
          {
            "value": 1,
            "label": "-6"
          },
          {
            "value": 2,
            "label": "-5"
          },
          {
            "value": 3,
            "label": "-4"
          },
          {
            "value": 4,
            "label": "-3"
          },
          {
            "value": 5,
            "label": "-2"
          },
          {
            "value": 6,
            "label": "-1"
          },
          {
            "value": 7,
            "label": "0"
          },
          {
            "value": 8,
            "label": "+1"
          },
          {
            "value": 9,
            "label": "+2"
          },
          {
            "value": 10,
            "label": "+3"
          },
          {
            "value": 11,
            "label": "+4"
          },
          {
            "value": 12,
            "label": "+5"
          },
          {
            "value": 13,
            "label": "+6"
          },
          {
            "value": 14,
            "label": "+7"
          }
        ],
        "description": "Split Right Transpose Lights (0-6: -7 to -1, 7: 0, 8-14: +1 to +7)"
      },
      {
        "number": 139,
        "group": "Split Right",
        "name": "MIDI Expression For Y",
        "kind": "enum",
        "min": 0,
        "max": 2,
        "options": [
          {
            "value": 0,
            "label": "Poly Aftertouch"
          },
          {
            "value": 1,
            "label": "Channel Aftertouch"
          },
          {
            "value": 2,
            "label": "CC defined in #125"
          }
        ],
        "description": "Split Right MIDI Expression For Y (0: Poly Aftertouch, 1: Channel Aftertouch, 2: CC defined in #125)"
      },
      {
        "number": 140,
        "group": "Split Right",
        "name": "MIDI CC For Fader 1",
        "kind": "number",
        "min": 0,
        "max": 128,
        "note": "0-127: CC, 128: Channel Aftertouch",
        "description": "Split Right MIDI CC For Fader 1 (0-127: CC, 128: Channel Aftertouch)"
      },
      {
        "number": 141,
        "group": "Split Right",
        "name": "MIDI CC For Fader 2",
        "kind": "number",
        "min": 0,
        "max": 128,
        "note": "0-127: CC, 128: Channel Aftertouch",
        "description": "Split Right MIDI CC For Fader 2 (0-127: CC, 128: Channel Aftertouch)"
      },
      {
        "number": 142,
        "group": "Split Right",
        "name": "MIDI CC For Fader 3",
        "kind": "number",
        "min": 0,
        "max": 128,
        "note": "0-127: CC, 128: Channel Aftertouch",
        "description": "Split Right MIDI CC For Fader 3 (0-127: CC, 128: Channel Aftertouch)"
      },
      {
        "number": 143,
        "group": "Split Right",
        "name": "MIDI CC For Fader 4",
        "kind": "number",
        "min": 0,
        "max": 128,
        "note": "0-127: CC, 128: Channel Aftertouch",
        "description": "Split Right MIDI CC For Fader 4 (0-127: CC, 128: Channel Aftertouch)"
      },
      {
        "number": 144,
        "group": "Split Right",
        "name": "MIDI CC For Fader 5",
        "kind": "number",
        "min": 0,
        "max": 128,
        "note": "0-127: CC, 128: Channel Aftertouch",
        "description": "Split Right MIDI CC For Fader 5 (0-127: CC, 128: Channel Aftertouch)"
      },
      {
        "number": 145,
        "group": "Split Right",
        "name": "MIDI CC For Fader 6",
        "kind": "number",
        "min": 0,
        "max": 128,
        "note": "0-127: CC, 128: Channel Aftertouch",
        "description": "Split Right MIDI CC For Fader 6 (0-127: CC, 128: Channel Aftertouch)"
      },
      {
        "number": 146,
        "group": "Split Right",
        "name": "MIDI CC For Fader 7",
        "kind": "number",
        "min": 0,
        "max": 128,
        "note": "0-127: CC, 128: Channel Aftertouch",
        "description": "Split Right MIDI CC For Fader 7 (0-127: CC, 128: Channel Aftertouch)"
      },
      {
        "number": 147,
        "group": "Split Right",
        "name": "MIDI CC For Fader 8",
        "kind": "number",
        "min": 0,
        "max": 128,
        "note": "0-127: CC, 128: Channel Aftertouch",
        "description": "Split Right MIDI CC For Fader 8 (0-127: CC, 128: Channel Aftertouch)"
      },
      {
        "number": 148,
        "group": "Split Right",
        "name": "LowRow X Behavior",
        "kind": "toggle",
        "min": 0,
        "max": 1,
        "options": [
          {
            "value": 0,
            "label": "Hold"
          },
          {
            "value": 1,
            "label": "Fader"
          }
        ],
        "description": "Split Right LowRow X Behavior (0: Hold, 1: Fader)"
      },
      {
        "number": 149,
        "group": "Split Right",
        "name": "MIDI CC For LowRow X",
        "kind": "number",
        "min": 0,
        "max": 128,
        "note": "0-127: CC, 128: Channel Aftertouch",
        "description": "Split Right MIDI CC For LowRow X (0-127: CC, 128: Channel Aftertouch)"
      },
      {
        "number": 150,
        "group": "Split Right",
        "name": "LowRow XYZ Behavior",
        "kind": "toggle",
        "min": 0,
        "max": 1,
        "options": [
          {
            "value": 0,
            "label": "Hold"
          },
          {
            "value": 1,
            "label": "Fader"
          }
        ],
        "description": "Split Right LowRow XYZ Behavior (0: Hold, 1: Fader)"
      },
      {
        "number": 151,
        "group": "Split Right",
        "name": "MIDI CC For LowRow XYZ X",
        "kind": "number",
        "min": 0,
        "max": 128,
        "note": "0-127: CC, 128: Channel Aftertouch",
        "description": "Split Right MIDI CC For LowRow XYZ X (0-127: CC, 128: Channel Aftertouch)"
      },
      {
        "number": 152,
        "group": "Split Right",
        "name": "MIDI CC For LowRow XYZ Y",
        "kind": "number",
        "min": 0,
        "max": 128,
        "note": "0-127: CC, 128: Channel Aftertouch",
        "description": "Split Right MIDI CC For LowRow XYZ Y (0-127: CC, 128: Channel Aftertouch)"
      },
      {
        "number": 153,
        "group": "Split Right",
        "name": "MIDI CC For LowRow XYZ Z",
        "kind": "number",
        "min": 0,
        "max": 128,
        "note": "0-127: CC, 128: Channel Aftertouch",
        "description": "Split Right MIDI CC For LowRow XYZ Z (0-127: CC, 128: Channel Aftertouch)"
      },
      {
        "number": 154,
        "group": "Split Right",
        "name": "Minimum CC Value For Y",
        "kind": "number",
        "min": 0,
        "max": 127,
        "description": "Split Right Minimum CC Value For Y"
      },
      {
        "number": 155,
        "group": "Split Right",
        "name": "Maximum CC Value For Y",
        "kind": "number",
        "min": 0,
        "max": 127,
        "description": "Split Right Maximum CC Value For Y"
      },
      {
        "number": 156,
        "group": "Split Right",
        "name": "Minimum CC Value For Z",
        "kind": "number",
        "min": 0,
        "max": 127,
        "description": "Split Right Minimum CC Value For Z"
      },
      {
        "number": 157,
        "group": "Split Right",
        "name": "Maximum CC Value For Z",
        "kind": "number",
        "min": 0,
        "max": 127,
        "description": "Split Right Maximum CC Value For Z"
      },
      {
        "number": 158,
        "group": "Split Right",
        "name": "CC Value For Z in 14-bit",
        "kind": "number",
        "min": 0,
        "max": 1,
        "description": "Split Right CC Value For Z in 14-bit"
      },
      {
        "number": 159,
        "group": "Split Right",
        "name": "Initial Value For Relative Y",
        "kind": "number",
        "min": 0,
        "max": 127,
        "description": "Split Right Initial Value For Relative Y"
      },
      {
        "number": 160,
        "group": "Split Right",
        "name": "Channel Per Row MIDI Channel Order",
        "kind": "toggle",
        "min": 0,
        "max": 1,
        "options": [
          {
            "value": 0,
            "label": "Normal"
          },
          {
            "value": 1,
            "label": "Reversed"
          }
        ],
        "description": "Split Right Channel Per Row MIDI Channel Order (0: Normal, 1: Reversed)"
      },
      {
        "number": 161,
        "group": "Split Right",
        "name": "Touch Animation",
        "kind": "enum",
        "min": 0,
        "max": 14,
        "options": [
          {
            "value": 0,
            "label": "Same"
          },
          {
            "value": 1,
            "label": "Crosses"
          },
          {
            "value": 2,
            "label": "Circles"
          },
          {
            "value": 3,
            "label": "Squares"
          },
          {
            "value": 4,
            "label": "Diamonds"
          },
          {
            "value": 5,
            "label": "Stars"
          },
          {
            "value": 6,
            "label": "Sparkles"
          },
          {
            "value": 7,
            "label": "Curtains"
          },
          {
            "value": 8,
            "label": "Blinds"
          },
          {
            "value": 9,
            "label": "Targets"
          },
          {
            "value": 10,
            "label": "Up"
          },
          {
            "value": 11,
            "label": "Down"
          },
          {
            "value": 12,
            "label": "Left"
          },
          {
            "value": 13,
            "label": "Right"
          },
          {
            "value": 14,
            "label": "Orbits"
          }
        ],
        "description": "Split Right Touch Animation, see the touch animation values below"
      },
      {
        "number": 162,
        "group": "Split Right",
        "name": "Sequencer Toggle Play",
        "kind": "action",
        "min": 1,
        "max": 1,
        "description": "Split Right Sequencer Toggle Play"
      },
      {
        "number": 163,
        "group": "Split Right",
        "name": "Sequencer Previous Pattern",
        "kind": "action",
        "min": 1,
        "max": 1,
        "description": "Split Right Sequencer Previous Pattern"
      },
      {
        "number": 164,
        "group": "Split Right",
        "name": "Sequencer Next Pattern",
        "kind": "action",
        "min": 1,
        "max": 1,
        "description": "Split Right Sequencer Next Pattern"
      },
      {
        "number": 165,
        "group": "Split Right",
        "name": "Sequencer Select Pattern Number",
        "kind": "number",
        "min": 0,
        "max": 3,
        "description": "Split Right Sequencer Select Pattern Number"
      },
      {
        "number": 166,
        "group": "Split Right",
        "name": "Sequencer Toggle Mute",
        "kind": "action",
        "min": 1,
        "max": 1,
        "description": "Split Right Sequencer Toggle Mute"
      },
      {
        "number": 200,
        "group": "Global",
        "name": "Split Active",
        "kind": "toggle",
        "min": 0,
        "max": 1,
        "options": [
          {
            "value": 0,
            "label": "Inactive"
          },
          {
            "value": 1,
            "label": "Active"
          }
        ],
        "description": "Global Split Active (0: Inactive, 1: Active)"
      },
      {
        "number": 201,
        "group": "Global",
        "name": "Selected Split",
        "kind": "toggle",
        "min": 0,
        "max": 1,
        "options": [
          {
            "value": 0,
            "label": "Left Split"
          },
          {
            "value": 1,
            "label": "Right Split"
          }
        ],
        "description": "Global Selected Split (0: Left Split, 1: Right Split)"
      },
      {
        "number": 202,
        "group": "Global",
        "name": "Split Point Column",
        "kind": "number",
        "min": 2,
        "max": 25,
        "description": "Global Split Point Column"
      },
      {
        "number": 203,
        "group": "Global",
        "name": "Main Note Light C",
        "kind": "toggle",
        "min": 0,
        "max": 1,
        "options": [
          {
            "value": 0,
            "label": "Off"
          },
          {
            "value": 1,
            "label": "On"
          }
        ],
        "description": "Global Main Note Light C (0: Off, 1: On)"
      },
      {
        "number": 204,
        "group": "Global",
        "name": "Main Note Light C#",
        "kind": "toggle",
        "min": 0,
        "max": 1,
        "options": [
          {
            "value": 0,
            "label": "Off"
          },
          {
            "value": 1,
            "label": "On"
          }
        ],
        "description": "Global Main Note Light C# (0: Off, 1: On)"
      },
      {
        "number": 205,
        "group": "Global",
        "name": "Main Note Light D",
        "kind": "toggle",
        "min": 0,
        "max": 1,
        "options": [
          {
            "value": 0,
            "label": "Off"
          },
          {
            "value": 1,
            "label": "On"
          }
        ],
        "description": "Global Main Note Light D (0: Off, 1: On)"
      },
      {
        "number": 206,
        "group": "Global",
        "name": "Main Note Light D#",
        "kind": "toggle",
        "min": 0,
        "max": 1,
        "options": [
          {
            "value": 0,
            "label": "Off"
          },
          {
            "value": 1,
            "label": "On"
          }
        ],
        "description": "Global Main Note Light D# (0: Off, 1: On)"
      },
      {
        "number": 207,
        "group": "Global",
        "name": "Main Note Light E",
        "kind": "toggle",
        "min": 0,
        "max": 1,
        "options": [
          {
            "value": 0,
            "label": "Off"
          },
          {
            "value": 1,
            "label": "On"
          }
        ],
        "description": "Global Main Note Light E (0: Off, 1: On)"
      },
      {
        "number": 208,
        "group": "Global",
        "name": "Main Note Light F",
        "kind": "toggle",
        "min": 0,
        "max": 1,
        "options": [
          {
            "value": 0,
            "label": "Off"
          },
          {
            "value": 1,
            "label": "On"
          }
        ],
        "description": "Global Main Note Light F (0: Off, 1: On)"
      },
      {
        "number": 209,
        "group": "Global",
        "name": "Main Note Light F#",
        "kind": "toggle",
        "min": 0,
        "max": 1,
        "options": [
          {
            "value": 0,
            "label": "Off"
          },
          {
            "value": 1,
            "label": "On"
          }
        ],
        "description": "Global Main Note Light F# (0: Off, 1: On)"
      },
      {
        "number": 210,
        "group": "Global",
        "name": "Main Note Light G",
        "kind": "toggle",
        "min": 0,
        "max": 1,
        "options": [
          {
            "value": 0,
            "label": "Off"
          },
          {
            "value": 1,
            "label": "On"
          }
        ],
        "description": "Global Main Note Light G (0: Off, 1: On)"
      },
      {
        "number": 211,
        "group": "Global",
        "name": "Main Note Light G#",
        "kind": "toggle",
        "min": 0,
        "max": 1,
        "options": [
          {
            "value": 0,
            "label": "Off"
          },
          {
            "value": 1,
            "label": "On"
          }
        ],
        "description": "Global Main Note Light G# (0: Off, 1: On)"
      },
      {
        "number": 212,
        "group": "Global",
        "name": "Main Note Light A",
        "kind": "toggle",
        "min": 0,
        "max": 1,
        "options": [
          {
            "value": 0,
            "label": "Off"
          },
          {
            "value": 1,
            "label": "On"
          }
        ],
        "description": "Global Main Note Light A (0: Off, 1: On)"
      },
      {
        "number": 213,
        "group": "Global",
        "name": "Main Note Light A#",
        "kind": "toggle",
        "min": 0,
        "max": 1,
        "options": [
          {
            "value": 0,
            "label": "Off"
          },
          {
            "value": 1,
            "label": "On"
          }
        ],
        "description": "Global Main Note Light A# (0: Off, 1: On)"
      },
      {
        "number": 214,
        "group": "Global",
        "name": "Main Note Light B",
        "kind": "toggle",
        "min": 0,
        "max": 1,
        "options": [
          {
            "value": 0,
            "label": "Off"
          },
          {
            "value": 1,
            "label": "On"
          }
        ],
        "description": "Global Main Note Light B (0: Off, 1: On)"
      },
      {
        "number": 215,
        "group": "Global",
        "name": "Accent Note Light C",
        "kind": "toggle",
        "min": 0,
        "max": 1,
        "options": [
          {
            "value": 0,
            "label": "Off"
          },
          {
            "value": 1,
            "label": "On"
          }
        ],
        "description": "Global Accent Note Light C (0: Off, 1: On)"
      },
      {
        "number": 216,
        "group": "Global",
        "name": "Accent Note Light C#",
        "kind": "toggle",
        "min": 0,
        "max": 1,
        "options": [
          {
            "value": 0,
            "label": "Off"
          },
          {
            "value": 1,
            "label": "On"
          }
        ],
        "description": "Global Accent Note Light C# (0: Off, 1: On)"
      },
      {
        "number": 217,
        "group": "Global",
        "name": "Accent Note Light D",
        "kind": "toggle",
        "min": 0,
        "max": 1,
        "options": [
          {
            "value": 0,
            "label": "Off"
          },
          {
            "value": 1,
            "label": "On"
          }
        ],
        "description": "Global Accent Note Light D (0: Off, 1: On)"
      },
      {
        "number": 218,
        "group": "Global",
        "name": "Accent Note Light D#",
        "kind": "toggle",
        "min": 0,
        "max": 1,
        "options": [
          {
            "value": 0,
            "label": "Off"
          },
          {
            "value": 1,
            "label": "On"
          }
        ],
        "description": "Global Accent Note Light D# (0: Off, 1: On)"
      },
      {
        "number": 219,
        "group": "Global",
        "name": "Accent Note Light E",
        "kind": "toggle",
        "min": 0,
        "max": 1,
        "options": [
          {
            "value": 0,
            "label": "Off"
          },
          {
            "value": 1,
            "label": "On"
          }
        ],
        "description": "Global Accent Note Light E (0: Off, 1: On)"
      },
      {
        "number": 220,
        "group": "Global",
        "name": "Accent Note Light F",
        "kind": "toggle",
        "min": 0,
        "max": 1,
        "options": [
          {
            "value": 0,
            "label": "Off"
          },
          {
            "value": 1,
            "label": "On"
          }
        ],
        "description": "Global Accent Note Light F (0: Off, 1: On)"
      },
      {
        "number": 221,
        "group": "Global",
        "name": "Accent Note Light F#",
        "kind": "toggle",
        "min": 0,
        "max": 1,
        "options": [
          {
            "value": 0,
            "label": "Off"
          },
          {
            "value": 1,
            "label": "On"
          }
        ],
        "description": "Global Accent Note Light F# (0: Off, 1: On)"
      },
      {
        "number": 222,
        "group": "Global",
        "name": "Accent Note Light G",
        "kind": "toggle",
        "min": 0,
        "max": 1,
        "options": [
          {
            "value": 0,
            "label": "Off"
          },
          {
            "value": 1,
            "label": "On"
          }
        ],
        "description": "Global Accent Note Light G (0: Off, 1: On)"
      },
      {
        "number": 223,
        "group": "Global",
        "name": "Accent Note Light G#",
        "kind": "toggle",
        "min": 0,
        "max": 1,
        "options": [
          {
            "value": 0,
            "label": "Off"
          },
          {
            "value": 1,
            "label": "On"
          }
        ],
        "description": "Global Accent Note Light G# (0: Off, 1: On)"
      },
      {
        "number": 224,
        "group": "Global",
        "name": "Accent Note Light A",
        "kind": "toggle",
        "min": 0,
        "max": 1,
        "options": [
          {
            "value": 0,
            "label": "Off"
          },
          {
            "value": 1,
            "label": "On"
          }
        ],
        "description": "Global Accent Note Light A (0: Off, 1: On)"
      },
      {
        "number": 225,
        "group": "Global",
        "name": "Accent Note Light A#",
        "kind": "toggle",
        "min": 0,
        "max": 1,
        "options": [
          {
            "value": 0,
            "label": "Off"
          },
          {
            "value": 1,
            "label": "On"
          }
        ],
        "description": "Global Accent Note Light A# (0: Off, 1: On)"
      },
      {
        "number": 226,
        "group": "Global",
        "name": "Accent Note Light B",
        "kind": "toggle",
        "min": 0,
        "max": 1,
        "options": [
          {
            "value": 0,
            "label": "Off"
          },
          {
            "value": 1,
            "label": "On"
          }
        ],
        "description": "Global Accent Note Light B (0: Off, 1: On)"
      },
      {
        "number": 227,
        "group": "Global",
        "name": "Row Offset",
        "kind": "enum",
        "min": 0,
        "max": 127,
        "options": [
          {
            "value": 0,
            "label": "No overlap"
          },
          {
            "value": 3,
            "label": "Intervals 3"
          },
          {
            "value": 4,
            "label": "Intervals 4"
          },
          {
            "value": 5,
            "label": "Intervals 5"
          },
          {
            "value": 6,
            "label": "Intervals 6"
          },
          {
            "value": 7,
            "label": "Intervals 7"
          },
          {
            "value": 12,
            "label": "Intervals 12"
          },
          {
            "value": 13,
            "label": "Guitar"
          },
          {
            "value": 127,
            "label": "0 offset"
          }
        ],
        "description": "Global Row Offset (only supports, 0: No overlap, 3 4 5 6 7 12: Intervals, 13: Guitar, 127: 0 offset)"
      },
      {
        "number": 228,
        "group": "Global",
        "name": "Switch 1 Assignment",
        "kind": "enum",
        "min": 0,
        "max": 17,
        "options": [
          {
            "value": 0,
            "label": "Oct Down"
          },
          {
            "value": 1,
            "label": "Oct Up"
          },
          {
            "value": 2,
            "label": "Sustain"
          },
          {
            "value": 3,
            "label": "CC65"
          },
          {
            "value": 4,
            "label": "Arp"
          },
          {
            "value": 5,
            "label": "Alt Split"
          },
          {
            "value": 6,
            "label": "Auto Octave"
          },
          {
            "value": 7,
            "label": "Tap Tempo"
          },
          {
            "value": 8,
            "label": "Legato"
          },
          {
            "value": 9,
            "label": "Latch"
          },
          {
            "value": 10,
            "label": "Preset Up"
          },
          {
            "value": 11,
            "label": "Preset Down"
          },
          {
            "value": 12,
            "label": "Reverse Pitch X"
          },
          {
            "value": 13,
            "label": "Sequencer Play"
          },
          {
            "value": 14,
            "label": "Sequencer Previous"
          },
          {
            "value": 15,
            "label": "Sequencer Next"
          },
          {
            "value": 16,
            "label": "Send MIDI Clock"
          },
          {
            "value": 17,
            "label": "Sequencer Mute"
          }
        ],
        "description": "Global Switch 1 Assignment, see the switch assignment values below"
      },
      {
        "number": 229,
        "group": "Global",
        "name": "Switch 2 Assignment",
        "kind": "enum",
        "min": 0,
        "max": 17,
        "options": [
          {
            "value": 0,
            "label": "Oct Down"
          },
          {
            "value": 1,
            "label": "Oct Up"
          },
          {
            "value": 2,
            "label": "Sustain"
          },
          {
            "value": 3,
            "label": "CC65"
          },
          {
            "value": 4,
            "label": "Arp"
          },
          {
            "value": 5,
            "label": "Alt Split"
          },
          {
            "value": 6,
            "label": "Auto Octave"
          },
          {
            "value": 7,
            "label": "Tap Tempo"
          },
          {
            "value": 8,
            "label": "Legato"
          },
          {
            "value": 9,
            "label": "Latch"
          },
          {
            "value": 10,
            "label": "Preset Up"
          },
          {
            "value": 11,
            "label": "Preset Down"
          },
          {
            "value": 12,
            "label": "Reverse Pitch X"
          },
          {
            "value": 13,
            "label": "Sequencer Play"
          },
          {
            "value": 14,
            "label": "Sequencer Previous"
          },
          {
            "value": 15,
            "label": "Sequencer Next"
          },
          {
            "value": 16,
            "label": "Send MIDI Clock"
          },
          {
            "value": 17,
            "label": "Sequencer Mute"
          }
        ],
        "description": "Global Switch 2 Assignment, see the switch assignment values below"
      },
      {
        "number": 230,
        "group": "Global",
        "name": "Foot Left Assignment",
        "kind": "enum",
        "min": 0,
        "max": 17,
        "options": [
          {
            "value": 0,
            "label": "Oct Down"
          },
          {
            "value": 1,
            "label": "Oct Up"
          },
          {
            "value": 2,
            "label": "Sustain"
          },
          {
            "value": 3,
            "label": "CC65"
          },
          {
            "value": 4,
            "label": "Arp"
          },
          {
            "value": 5,
            "label": "Alt Split"
          },
          {
            "value": 6,
            "label": "Auto Octave"
          },
          {
            "value": 7,
            "label": "Tap Tempo"
          },
          {
            "value": 8,
            "label": "Legato"
          },
          {
            "value": 9,
            "label": "Latch"
          },
          {
            "value": 10,
            "label": "Preset Up"
          },
          {
            "value": 11,
            "label": "Preset Down"
          },
          {
            "value": 12,
            "label": "Reverse Pitch X"
          },
          {
            "value": 13,
            "label": "Sequencer Play"
          },
          {
            "value": 14,
            "label": "Sequencer Previous"
          },
          {
            "value": 15,
            "label": "Sequencer Next"
          },
          {
            "value": 16,
            "label": "Send MIDI Clock"
          },
          {
            "value": 17,
            "label": "Sequencer Mute"
          }
        ],
        "description": "Global Foot Left Assignment, see the switch assignment values below"
      },
      {
        "number": 231,
        "group": "Global",
        "name": "Foot Right Assignment",
        "kind": "enum",
        "min": 0,
        "max": 17,
        "options": [
          {
            "value": 0,
            "label": "Oct Down"
          },
          {
            "value": 1,
            "label": "Oct Up"
          },
          {
            "value": 2,
            "label": "Sustain"
          },
          {
            "value": 3,
            "label": "CC65"
          },
          {
            "value": 4,
            "label": "Arp"
          },
          {
            "value": 5,
            "label": "Alt Split"
          },
          {
            "value": 6,
            "label": "Auto Octave"
          },
          {
            "value": 7,
            "label": "Tap Tempo"
          },
          {
            "value": 8,
            "label": "Legato"
          },
          {
            "value": 9,
            "label": "Latch"
          },
          {
            "value": 10,
            "label": "Preset Up"
          },
          {
            "value": 11,
            "label": "Preset Down"
          },
          {
            "value": 12,
            "label": "Reverse Pitch X"
          },
          {
            "value": 13,
            "label": "Sequencer Play"
          },
          {
            "value": 14,
            "label": "Sequencer Previous"
          },
          {
            "value": 15,
            "label": "Sequencer Next"
          },
          {
            "value": 16,
            "label": "Send MIDI Clock"
          },
          {
            "value": 17,
            "label": "Sequencer Mute"
          }
        ],
        "description": "Global Foot Right Assignment, see the switch assignment values below"
      },
      {
        "number": 232,
        "group": "Global",
        "name": "Velocity Sensitivity",
        "kind": "enum",
        "min": 0,
        "max": 3,
        "options": [
          {
            "value": 0,
            "label": "Low"
          },
          {
            "value": 1,
            "label": "Medium"
          },
          {
            "value": 2,
            "label": "High"
          },
          {
            "value": 3,
            "label": "Fixed"
          }
        ],
        "description": "Global Velocity Sensitivity (0: Low, 1: Medium, 2: High, 3: Fixed)"
      },
      {
        "number": 233,
        "group": "Global",
        "name": "Pressure Sensitivity",
        "kind": "enum",
        "min": 0,
        "max": 2,
        "options": [
          {
            "value": 0,
            "label": "Low"
          },
          {
            "value": 1,
            "label": "Medium"
          },
          {
            "value": 2,
            "label": "High"
          }
        ],
        "description": "Global Pressure Sensitivity (0: Low, 1: Medium, 2: High)"
      },
      {
        "number": 234,
        "group": "Device",
        "name": "MIDI I/O",
        "kind": "toggle",
        "min": 0,
        "max": 1,
        "options": [
          {
            "value": 0,
            "label": "MIDI Jacks"
          },
          {
            "value": 1,
            "label": "USB"
          }
        ],
        "description": "Device MIDI I/O (0: MIDI Jacks, 1: USB)"
      },
      {
        "number": 235,
        "group": "Global",
        "name": "Arp Direction",
        "kind": "enum",
        "min": 0,
        "max": 4,
        "options": [
          {
            "value": 0,
            "label": "Up"
          },
          {
            "value": 1,
            "label": "Down"
          },
          {
            "value": 2,
            "label": "Up Down"
          },
          {
            "value": 3,
            "label": "Random"
          },
          {
            "value": 4,
            "label": "Replay All"
          }
        ],
        "description": "Global Arp Direction (0: Up, 1: Down, 2: Up Down, 3: Random, 4: Replay All)"
      },
      {
        "number": 236,
        "group": "Global",
        "name": "Arp Tempo Note Value",
        "kind": "enum",
        "min": 1,
        "max": 7,
        "options": [
          {
            "value": 1,
            "label": "8"
          },
          {
            "value": 2,
            "label": "8 Tri"
          },
          {
            "value": 3,
            "label": "16"
          },
          {
            "value": 4,
            "label": "16 Swing"
          },
          {
            "value": 5,
            "label": "16 Tri"
          },
          {
            "value": 6,
            "label": "32"
          },
          {
            "value": 7,
            "label": "32 Tri"
          }
        ],
        "description": "Global Arp Tempo Note Value (1: 8, 2: 8 Tri, 3: 16, 4: 16 Swing, 5: 16 Tri, 6: 32, 7: 32 Tri)"
      },
      {
        "number": 237,
        "group": "Global",
        "name": "Arp Octave Extension",
        "kind": "enum",
        "min": 0,
        "max": 2,
        "options": [
          {
            "value": 0,
            "label": "None"
          },
          {
            "value": 1,
            "label": "+1"
          },
          {
            "value": 2,
            "label": "+2"
          }
        ],
        "description": "Global Arp Octave Extension (0: None, 1: +1, 2: +2)"
      },
      {
        "number": 238,
        "group": "Global",
        "name": "Clock BPM",
        "kind": "number",
        "min": 1,
        "max": 360,
        "note": "applies when receiving no MIDI clock",
        "description": "Global Clock BPM (applies when receiving no MIDI clock)"
      },
      {
        "number": 239,
        "group": "Global",
        "name": "Switch 1 Both Splits",
        "kind": "toggle",
        "min": 0,
        "max": 1,
        "options": [
          {
            "value": 0,
            "label": "Off"
          },
          {
            "value": 1,
            "label": "On"
          }
        ],
        "description": "Global Switch 1 Both Splits (0: Off, 1: On)"
      },
      {
        "number": 240,
        "group": "Global",
        "name": "Switch 2 Both Splits",
        "kind": "toggle",
        "min": 0,
        "max": 1,
        "options": [
          {
            "value": 0,
            "label": "Off"
          },
          {
            "value": 1,
            "label": "On"
          }
        ],
        "description": "Global Switch 2 Both Splits (0: Off, 1: On)"
      },
      {
        "number": 241,
        "group": "Global",
        "name": "Foot Left Both Splits",
        "kind": "toggle",
        "min": 0,
        "max": 1,
        "options": [
          {
            "value": 0,
            "label": "Off"
          },
          {
            "value": 1,
            "label": "On"
          }
        ],
        "description": "Global Foot Left Both Splits (0: Off, 1: On)"
      },
      {
        "number": 242,
        "group": "Global",
        "name": "Foot Right Both Splits",
        "kind": "toggle",
        "min": 0,
        "max": 1,
        "options": [
          {
            "value": 0,
            "label": "Off"
          },
          {
            "value": 1,
            "label": "On"
          }
        ],
        "description": "Global Foot Right Both Splits (0: Off, 1: On)"
      },
      {
        "number": 243,
        "group": "Global",
        "name": "Settings Preset Load",
        "kind": "choice-action",
        "min": 0,
        "max": 5,
        "options": [
          {
            "value": 0,
            "label": "Preset 1"
          },
          {
            "value": 1,
            "label": "Preset 2"
          },
          {
            "value": 2,
            "label": "Preset 3"
          },
          {
            "value": 3,
            "label": "Preset 4"
          },
          {
            "value": 4,
            "label": "Preset 5"
          },
          {
            "value": 5,
            "label": "Preset 6"
          }
        ],
        "description": "Global Settings Preset Load"
      },
      {
        "number": 244,
        "group": "Global",
        "name": "Pressure Aftertouch",
        "kind": "toggle",
        "min": 0,
        "max": 1,
        "options": [
          {
            "value": 0,
            "label": "Off"
          },
          {
            "value": 1,
            "label": "On"
          }
        ],
        "description": "Global Pressure Aftertouch (0: Off, 1: On)"
      },
      {
        "number": 245,
        "group": "Device",
        "name": "User Firmware Mode",
        "kind": "toggle",
        "min": 0,
        "max": 1,
        "options": [
          {
            "value": 0,
            "label": "Off"
          },
          {
            "value": 1,
            "label": "On"
          }
        ],
        "description": "Device User Firmware Mode (0: Off, 1: On)"
      },
      {
        "number": 246,
        "group": "Device",
        "name": "Left Handed Operation",
        "kind": "toggle",
        "min": 0,
        "max": 1,
        "options": [
          {
            "value": 0,
            "label": "Off"
          },
          {
            "value": 1,
            "label": "On"
          }
        ],
        "description": "Device Left Handed Operation (0: Off, 1: On)"
      },
      {
        "number": 247,
        "group": "Global",
        "name": "Active Note Lights Preset",
        "kind": "number",
        "min": 0,
        "max": 11,
        "description": "Global Active Note Lights Preset"
      },
      {
        "number": 248,
        "group": "Global",
        "name": "MIDI CC For Switch CC65",
        "kind": "number",
        "min": 0,
        "max": 127,
        "note": "Changes the CC for all switches - Legacy option, see NRPN 255-258",
        "description": "Global MIDI CC For Switch CC65 (Changes the CC for all switches - Legacy option, see NRPN 255-258)"
      },
      {
        "number": 249,
        "group": "Global",
        "name": "Minimum Value For Velocity",
        "kind": "number",
        "min": 1,
        "max": 127,
        "description": "Global Minimum Value For Velocity"
      },
      {
        "number": 250,
        "group": "Global",
        "name": "Maximum Value For Velocity",
        "kind": "number",
        "min": 1,
        "max": 127,
        "description": "Global Maximum Value For Velocity"
      },
      {
        "number": 251,
        "group": "Global",
        "name": "Value For Fixed Velocity",
        "kind": "number",
        "min": 1,
        "max": 127,
        "description": "Global Value For Fixed Velocity"
      },
      {
        "number": 252,
        "group": "Device",
        "name": "Minimum Interval Between MIDI Bytes Over USB",
        "kind": "number",
        "min": 0,
        "max": 512,
        "description": "Device Minimum Interval Between MIDI Bytes Over USB"
      },
      {
        "number": 253,
        "group": "Global",
        "name": "Custom Row Offset Instead Of Octave",
        "kind": "number",
        "min": 0,
        "max": 33,
        "note": "0-32: -16-16 semitone intervals, 33: inverted Guitar",
        "description": "Global Custom Row Offset Instead Of Octave (0-32: -16-16 semitone intervals, 33: inverted Guitar)"
      },
      {
        "number": 254,
        "group": "Device",
        "name": "MIDI Through",
        "kind": "toggle",
        "min": 0,
        "max": 1,
        "options": [
          {
            "value": 0,
            "label": "Off"
          },
          {
            "value": 1,
            "label": "On"
          }
        ],
        "description": "Device MIDI Through (0: Off, 1: On)"
      },
      {
        "number": 255,
        "group": "Global",
        "name": "MIDI CC For Foot Left CC65",
        "kind": "number",
        "min": 0,
        "max": 127,
        "description": "Global MIDI CC For Foot Left CC65"
      },
      {
        "number": 256,
        "group": "Global",
        "name": "MIDI CC For Foot Right CC65",
        "kind": "number",
        "min": 0,
        "max": 127,
        "description": "Global MIDI CC For Foot Right CC65"
      },
      {
        "number": 257,
        "group": "Global",
        "name": "MIDI CC For Switch 1 CC65",
        "kind": "number",
        "min": 0,
        "max": 127,
        "description": "Global MIDI CC For Switch 1 CC65"
      },
      {
        "number": 258,
        "group": "Global",
        "name": "MIDI CC For Switch 2 CC65",
        "kind": "number",
        "min": 0,
        "max": 127,
        "description": "Global MIDI CC For Switch 2 CC65"
      },
      {
        "number": 259,
        "group": "Global",
        "name": "MIDI CC For Foot Left Sustain",
        "kind": "number",
        "min": 0,
        "max": 127,
        "description": "Global MIDI CC For Foot Left Sustain"
      },
      {
        "number": 260,
        "group": "Global",
        "name": "MIDI CC For Foot Right Sustain",
        "kind": "number",
        "min": 0,
        "max": 127,
        "description": "Global MIDI CC For Foot Right Sustain"
      },
      {
        "number": 261,
        "group": "Global",
        "name": "MIDI CC For Switch 1 Sustain",
        "kind": "number",
        "min": 0,
        "max": 127,
        "description": "Global MIDI CC For Switch 1 Sustain"
      },
      {
        "number": 262,
        "group": "Global",
        "name": "MIDI CC For Switch 2 Sustain",
        "kind": "number",
        "min": 0,
        "max": 127,
        "description": "Global MIDI CC For Switch 2 Sustain"
      },
      {
        "number": 263,
        "group": "Global",
        "name": "Note Number For Guitar Tuning Row 1",
        "kind": "number",
        "min": 0,
        "max": 127,
        "description": "Global Note Number For Guitar Tuning Row 1"
      },
      {
        "number": 264,
        "group": "Global",
        "name": "Note Number For Guitar Tuning Row 2",
        "kind": "number",
        "min": 0,
        "max": 127,
        "description": "Global Note Number For Guitar Tuning Row 2"
      },
      {
        "number": 265,
        "group": "Global",
        "name": "Note Number For Guitar Tuning Row 3",
        "kind": "number",
        "min": 0,
        "max": 127,
        "description": "Global Note Number For Guitar Tuning Row 3"
      },
      {
        "number": 266,
        "group": "Global",
        "name": "Note Number For Guitar Tuning Row 4",
        "kind": "number",
        "min": 0,
        "max": 127,
        "description": "Global Note Number For Guitar Tuning Row 4"
      },
      {
        "number": 267,
        "group": "Global",
        "name": "Note Number For Guitar Tuning Row 5",
        "kind": "number",
        "min": 0,
        "max": 127,
        "description": "Global Note Number For Guitar Tuning Row 5"
      },
      {
        "number": 268,
        "group": "Global",
        "name": "Note Number For Guitar Tuning Row 6",
        "kind": "number",
        "min": 0,
        "max": 127,
        "description": "Global Note Number For Guitar Tuning Row 6"
      },
      {
        "number": 269,
        "group": "Global",
        "name": "Note Number For Guitar Tuning Row 7",
        "kind": "number",
        "min": 0,
        "max": 127,
        "description": "Global Note Number For Guitar Tuning Row 7"
      },
      {
        "number": 270,
        "group": "Global",
        "name": "Note Number For Guitar Tuning Row 8",
        "kind": "number",
        "min": 0,
        "max": 127,
        "description": "Global Note Number For Guitar Tuning Row 8"
      },
      {
        "number": 271,
        "group": "Device",
        "name": "Serial Mode",
        "kind": "action",
        "min": 1,
        "max": 1,
        "note": "Takes the instrument off MIDI until OS Update is tapped on it or the updater returns it",
        "description": "Device Serial Mode (1: On, same as tapping OS Update; the updater or OS Update switch it off)"
      },
      {
        "number": 272,
        "group": "Device",
        "name": "Model",
        "kind": "readonly",
        "min": 0,
        "max": 0,
        "note": "read-only, 200 or 128",
        "description": "Device Model (read-only, 200 or 128)"
      },
      {
        "number": 273,
        "group": "Device",
        "name": "Firmware Version",
        "kind": "readonly",
        "min": 0,
        "max": 0,
        "note": "read-only, 240 for firmware 2.4.0",
        "description": "Device Firmware Version (read-only, 240 for firmware 2.4.0)"
      },
      {
        "number": 274,
        "group": "Device",
        "name": "Firmware Build",
        "kind": "readonly",
        "min": 0,
        "max": 0,
        "note": "read-only, 75 for the build the instrument shows as .075",
        "description": "Device Firmware Build (read-only, 75 for the build the instrument shows as .075)"
      },
      {
        "number": 275,
        "group": "Device",
        "name": "Calibrated",
        "kind": "readonly",
        "min": 0,
        "max": 0,
        "options": [
          {
            "value": 0,
            "label": "No"
          },
          {
            "value": 1,
            "label": "Yes"
          }
        ],
        "note": "(0: No, 1: Yes), read-only",
        "description": "Device Calibrated (0: No, 1: Yes), read-only"
      }
    ],
    "tables": {
      "color": [
        {
          "value": 0,
          "label": "as set in Note Lights settings"
        },
        {
          "value": 1,
          "label": "Red"
        },
        {
          "value": 2,
          "label": "Yellow"
        },
        {
          "value": 3,
          "label": "Green"
        },
        {
          "value": 4,
          "label": "Cyan"
        },
        {
          "value": 5,
          "label": "Blue"
        },
        {
          "value": 6,
          "label": "Magenta"
        },
        {
          "value": 7,
          "label": "Off"
        },
        {
          "value": 8,
          "label": "White"
        },
        {
          "value": 9,
          "label": "Orange"
        },
        {
          "value": 10,
          "label": "Lime"
        },
        {
          "value": 11,
          "label": "Pink"
        }
      ],
      "switchAssignment": [
        {
          "value": 0,
          "label": "Oct Down"
        },
        {
          "value": 1,
          "label": "Oct Up"
        },
        {
          "value": 2,
          "label": "Sustain"
        },
        {
          "value": 3,
          "label": "CC65"
        },
        {
          "value": 4,
          "label": "Arp"
        },
        {
          "value": 5,
          "label": "Alt Split"
        },
        {
          "value": 6,
          "label": "Auto Octave"
        },
        {
          "value": 7,
          "label": "Tap Tempo"
        },
        {
          "value": 8,
          "label": "Legato"
        },
        {
          "value": 9,
          "label": "Latch"
        },
        {
          "value": 10,
          "label": "Preset Up"
        },
        {
          "value": 11,
          "label": "Preset Down"
        },
        {
          "value": 12,
          "label": "Reverse Pitch X"
        },
        {
          "value": 13,
          "label": "Sequencer Play"
        },
        {
          "value": 14,
          "label": "Sequencer Previous"
        },
        {
          "value": 15,
          "label": "Sequencer Next"
        },
        {
          "value": 16,
          "label": "Send MIDI Clock"
        },
        {
          "value": 17,
          "label": "Sequencer Mute"
        }
      ],
      "touchAnimation": [
        {
          "value": 0,
          "label": "Same"
        },
        {
          "value": 1,
          "label": "Crosses"
        },
        {
          "value": 2,
          "label": "Circles"
        },
        {
          "value": 3,
          "label": "Squares"
        },
        {
          "value": 4,
          "label": "Diamonds"
        },
        {
          "value": 5,
          "label": "Stars"
        },
        {
          "value": 6,
          "label": "Sparkles"
        },
        {
          "value": 7,
          "label": "Curtains"
        },
        {
          "value": 8,
          "label": "Blinds"
        },
        {
          "value": 9,
          "label": "Targets"
        },
        {
          "value": 10,
          "label": "Up"
        },
        {
          "value": 11,
          "label": "Down"
        },
        {
          "value": 12,
          "label": "Left"
        },
        {
          "value": 13,
          "label": "Right"
        },
        {
          "value": 14,
          "label": "Orbits"
        }
      ]
    }
  };
});
