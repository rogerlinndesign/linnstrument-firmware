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
// LinnStrument User Firmware Mode over MIDI: message builders and a decoder for the touch
// stream, as specified in user_firmware_mode.md. No dependencies; works in a browser with Web
// MIDI and in Node. Every function here is covered by user_firmware_protocol.test.js.
(function (root, factory) {
  if (typeof module === "object" && module.exports) {
    module.exports = factory();
  } else {
    root.LinnUserFirmware = factory();
  }
})(typeof self !== "undefined" ? self : this, function () {
  "use strict";

  const NRPN_USER_FIRMWARE_MODE = 245;
  const NRPN_QUERY = 299;
  const CC_SLIDE = 9;
  const CC_X = 10;
  const CC_Y = 11;
  const CC_Z = 12;
  const CC_DECIMATION = 13;
  const CC_LED_COLUMN = 20;
  const CC_LED_ROW = 21;
  const CC_LED_COLOR = 22;
  const CC_SLIDE_LEAVING = 119;
  const ANNOUNCE_CHANNEL = 9;

  const COLORS = {
    default: 0, red: 1, yellow: 2, green: 3, cyan: 4, blue: 5, magenta: 6,
    black: 7, white: 8, orange: 9, lime: 10, pink: 11,
  };

  function columnsFor(model) {
    if (model === 128) return 17;
    if (model === 200) return 26;
    throw new Error("model must be 200 or 128");
  }

  function checkRow(row) {
    if (!Number.isInteger(row) || row < 0 || row > 7) throw new Error("row must be 0 to 7");
  }

  function checkColumn(col) {
    if (!Number.isInteger(col) || col < 0 || col > 25) throw new Error("column must be 0 to 25");
  }

  function cc(channel, number, value) {
    return [0xB0 | (channel - 1), number & 0x7F, value & 0x7F];
  }

  // An NRPN is four control changes; channel is 1 to 16.
  function nrpn(number, value, channel) {
    channel = channel || 1;
    return [
      cc(channel, 99, number >> 7),
      cc(channel, 98, number & 0x7F),
      cc(channel, 6, value >> 7),
      cc(channel, 38, value & 0x7F),
    ];
  }

  function enterUserFirmwareMode() { return nrpn(NRPN_USER_FIRMWARE_MODE, 1); }
  function leaveUserFirmwareMode() { return nrpn(NRPN_USER_FIRMWARE_MODE, 0); }
  function queryUserFirmwareMode() { return nrpn(NRPN_QUERY, NRPN_USER_FIRMWARE_MODE); }

  // axis is "x", "y", "z" or "slide"; the row's channel is row + 1.
  function enableAxis(row, axis, on) {
    checkRow(row);
    const number = { x: CC_X, y: CC_Y, z: CC_Z, slide: CC_SLIDE }[axis];
    if (number === undefined) throw new Error("axis must be x, y, z or slide");
    return [cc(row + 1, number, on ? 1 : 0)];
  }

  function setDecimation(milliseconds) {
    if (!Number.isInteger(milliseconds) || milliseconds < 0 || milliseconds > 127) {
      throw new Error("decimation must be 0 to 127 milliseconds");
    }
    return [cc(1, CC_DECIMATION, milliseconds)];
  }

  // color is 1 to 11 from COLORS; 0 switches the cell off.
  function setLed(col, row, color) {
    checkColumn(col);
    checkRow(row);
    if (!Number.isInteger(color) || color < 0 || color > 11) throw new Error("color must be 0 to 11");
    return [cc(1, CC_LED_COLUMN, col), cc(1, CC_LED_ROW, row), cc(1, CC_LED_COLOR, color)];
  }

  function clearLed(col, row) { return setLed(col, row, 0); }

  // Decodes the instrument's messages one at a time into events. Keeps the state an X value and
  // an NRPN need across messages. Returns an event object or null for anything else.
  function createDecoder() {
    const xLsb = {};
    const nrpnState = {};

    function nrpnEvent(channel, state) {
      if (state.numberMsb === undefined || state.numberLsb === undefined ||
          state.valueMsb === undefined || state.valueLsb === undefined) {
        return null;
      }
      const number = (state.numberMsb << 7) | state.numberLsb;
      const value = (state.valueMsb << 7) | state.valueLsb;
      nrpnState[channel] = {};
      if (number === NRPN_USER_FIRMWARE_MODE) {
        return { type: "mode", active: value === 1, channel: channel };
      }
      return { type: "nrpn", number: number, value: value, channel: channel };
    }

    return {
      decode: function (bytes) {
        if (!bytes || bytes.length < 3) return null;
        const status = bytes[0] & 0xF0;
        const channel = (bytes[0] & 0x0F) + 1;
        const row = channel - 1;
        const d1 = bytes[1];
        const d2 = bytes.length > 2 ? bytes[2] : 0;

        if (status === 0x90 && d2 > 0) {
          return { type: "press", col: d1, row: row, velocity: d2 };
        }
        if (status === 0x80 || (status === 0x90 && d2 === 0)) {
          return { type: "release", col: d1, row: row, velocity: d2 };
        }
        if (status === 0xA0) {
          return { type: "pressure", col: d1, row: row, value: d2 };
        }
        if (status !== 0xB0) return null;

        // CC 6 and 38 are both column 6's X and the NRPN data entry controllers; they belong to
        // an NRPN only while a number set by CC 99 and 98 is waiting for its value
        if (d1 === 101 || d1 === 100) {
          nrpnState[channel] = {};
          return null;
        }
        const pending = nrpnState[channel];
        if (d1 === 99 || d1 === 98 || ((d1 === 6 || d1 === 38) && pending && pending.numberLsb !== undefined)) {
          const state = nrpnState[channel] || (nrpnState[channel] = {});
          if (d1 === 99) state.numberMsb = d2;
          else if (d1 === 98) state.numberLsb = d2;
          else if (d1 === 6) state.valueMsb = d2;
          else state.valueLsb = d2;
          return d1 === 38 ? nrpnEvent(channel, state) : null;
        }

        // column 0, the switch column, never sends expression, so CC 0, 32 and 64 stay free
        if (d1 >= 33 && d1 <= 57) {
          xLsb[channel + ":" + (d1 - 32)] = d2;
          return null;
        }
        if (d1 >= 1 && d1 <= 25) {
          const key = channel + ":" + d1;
          const lsb = xLsb[key] || 0;
          return { type: "x", col: d1, row: row, value: (d2 << 7) | lsb };
        }
        if (d1 >= 65 && d1 <= 89) {
          return { type: "y", col: d1 - 64, row: row, value: d2 };
        }
        if (d1 === CC_SLIDE_LEAVING) {
          return { type: "slide", row: row, fromCol: d2 };
        }
        return null;
      },
    };
  }

  // Keeps the current state of every cell from the decoded events: which are pressed and
  // their latest velocity, pressure, x and y. apply() returns the cell an event changed, or null.
  function createSurfaceState(columns) {
    const cells = {};
    function cell(col, row) {
      const key = col + ":" + row;
      return cells[key] || (cells[key] = { col: col, row: row, pressed: false, velocity: 0, pressure: 0, x: null, y: null });
    }
    return {
      cell: cell,
      pressedCells: function () {
        return Object.keys(cells).map(function (k) { return cells[k]; }).filter(function (c) { return c.pressed; });
      },
      apply: function (event) {
        if (!event || event.col === undefined || event.col >= columns) return null;
        const c = cell(event.col, event.row);
        switch (event.type) {
          case "press":
            c.pressed = true;
            c.velocity = event.velocity;
            c.pressure = 0;
            c.x = null;
            c.y = null;
            return c;
          case "release":
            c.pressed = false;
            c.pressure = 0;
            return c;
          case "pressure":
            c.pressure = event.value;
            return c;
          case "x":
            c.x = event.value;
            return c;
          case "y":
            c.y = event.value;
            return c;
          default:
            return null;
        }
      },
    };
  }

  return {
    createSurfaceState: createSurfaceState,
    NRPN_USER_FIRMWARE_MODE: NRPN_USER_FIRMWARE_MODE,
    NRPN_QUERY: NRPN_QUERY,
    ANNOUNCE_CHANNEL: ANNOUNCE_CHANNEL,
    COLORS: COLORS,
    columnsFor: columnsFor,
    nrpn: nrpn,
    enterUserFirmwareMode: enterUserFirmwareMode,
    leaveUserFirmwareMode: leaveUserFirmwareMode,
    queryUserFirmwareMode: queryUserFirmwareMode,
    enableAxis: enableAxis,
    setDecimation: setDecimation,
    setLed: setLed,
    clearLed: clearLed,
    createDecoder: createDecoder,
  };
});
