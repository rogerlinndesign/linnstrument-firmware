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
// LinnStrument as a custom MIDI controller in User Firmware Mode: a layout of zones (pads,
// faders, XY pads and toggles) on the playing surface, and a runner that turns the instrument's
// touch stream into controller messages for a DAW and keeps the lights showing the state. No
// dependencies beyond user_firmware_protocol.js; works in a browser and in Node. Every function
// here is covered by surface_designer.test.js.
(function (root, factory) {
  if (typeof module === "object" && module.exports) {
    module.exports = factory(require("./user_firmware_protocol.js"));
  } else {
    root.LinnSurfaceDesigner = factory(root.LinnUserFirmware);
  }
})(typeof self !== "undefined" ? self : this, function (ufm) {
  "use strict";

  const TYPES = ["pad", "keys", "fader", "xy", "toggle"];
  const SCALES = {
    chromatic: [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11],
    major: [0, 2, 4, 5, 7, 9, 11],
    minor: [0, 2, 3, 5, 7, 8, 10],
    pentatonic: [0, 2, 4, 7, 9],
    blues: [0, 3, 5, 6, 7, 10],
  };
  const ROWS = 8;
  const X_UNIT = 170.625;
  const FORMAT = "linnstrument-surface-layout";

  const DEFAULTS = {
    pad: { color: ufm.COLORS.orange, activeColor: ufm.COLORS.white, note: 36, pressure: "poly", perCell: true },
    keys: { color: ufm.COLORS.green, activeColor: ufm.COLORS.white, rootColor: ufm.COLORS.blue, note: 48, rowInterval: 5, scale: "chromatic", pressure: "poly" },
    fader: { color: ufm.COLORS.green, activeColor: ufm.COLORS.green, cc: 20, initial: 0, bipolar: false },
    xy: { color: ufm.COLORS.blue, activeColor: ufm.COLORS.white, ccX: 20, ccY: 21, spring: false },
    toggle: { color: ufm.COLORS.red, activeColor: ufm.COLORS.lime, message: "cc", cc: 102, note: 36, program: 0, latch: true, perCell: true },
  };

  function createLayout(model, name) {
    return { format: FORMAT, model: model || 200, name: name || "Untitled", zones: [] };
  }

  function columnsOf(layout) { return ufm.columnsFor(layout.model); }

  function width(zone) { return zone.col1 - zone.col0 + 1; }
  function height(zone) { return zone.row1 - zone.row0 + 1; }
  function covers(zone, col, row) {
    return col >= zone.col0 && col <= zone.col1 && row >= zone.row0 && row <= zone.row1;
  }
  function overlap(a, b) {
    return a.col0 <= b.col1 && b.col0 <= a.col1 && a.row0 <= b.row1 && b.row0 <= a.row1;
  }
  function horizontal(zone) { return width(zone) >= height(zone); }

  // the cells of a zone in order, left to right then bottom to top, which is the order per cell
  // notes and controllers count in
  function cellIndex(zone, col, row) {
    return (row - zone.row0) * width(zone) + (col - zone.col0);
  }

  function isInt(v, lo, hi) { return Number.isInteger(v) && v >= lo && v <= hi; }

  // A zone with the fields its type needs; the rectangle is inclusive and in surface
  // coordinates, column 1 is the first playing column.
  function makeZone(fields) {
    if (!fields || typeof fields !== "object") throw new Error("a zone must be an object");
    const type = fields.type;
    if (TYPES.indexOf(type) < 0) throw new Error("type must be pad, keys, fader, xy or toggle");
    const zone = Object.assign({ id: fields.id || null, type: type, channel: 1 }, DEFAULTS[type]);
    Object.keys(fields).forEach(function (k) { if (fields[k] !== undefined) zone[k] = fields[k]; });
    const c0 = Math.min(fields.col0, fields.col1), c1 = Math.max(fields.col0, fields.col1);
    const r0 = Math.min(fields.row0, fields.row1), r1 = Math.max(fields.row0, fields.row1);
    zone.col0 = c0; zone.col1 = c1; zone.row0 = r0; zone.row1 = r1;
    return zone;
  }

  // null when the zone fits the layout, otherwise what is wrong with it
  function validateZone(layout, zone) {
    const columns = columnsOf(layout);
    if (TYPES.indexOf(zone.type) < 0) return "type must be pad, keys, fader, xy or toggle";
    if (!isInt(zone.col0, 1, columns - 1) || !isInt(zone.col1, 1, columns - 1) || zone.col0 > zone.col1) {
      return "columns must be 1 to " + (columns - 1);
    }
    if (!isInt(zone.row0, 0, ROWS - 1) || !isInt(zone.row1, 0, ROWS - 1) || zone.row0 > zone.row1) return "rows must be 0 to 7";
    if (!isInt(zone.channel, 1, 16)) return "channel must be 1 to 16";
    if (!isInt(zone.color, 0, 11) || !isInt(zone.activeColor, 0, 11)) return "colors must be 0 to 11";
    for (const flag of ["perCell", "latch", "bipolar", "spring"]) {
      if (zone[flag] !== undefined && typeof zone[flag] !== "boolean") return flag + " must be true or false";
    }
    const cells = width(zone) * height(zone);
    switch (zone.type) {
      case "pad":
        if (!isInt(zone.note, 0, 127)) return "note must be 0 to 127";
        if (zone.perCell && zone.note + cells - 1 > 127) return "per cell notes run past 127";
        if (["poly", "channel", "none"].indexOf(zone.pressure) < 0) return "pressure must be poly, channel or none";
        break;
      case "keys":
        if (!isInt(zone.note, 0, 127)) return "root note must be 0 to 127";
        if (!isInt(zone.rowInterval, 1, 24)) return "row interval must be 1 to 24 semitones";
        if (!SCALES[zone.scale]) return "scale must be one of " + Object.keys(SCALES).join(", ");
        if (!isInt(zone.rootColor, 0, 11)) return "colors must be 0 to 11";
        if (["poly", "channel", "none"].indexOf(zone.pressure) < 0) return "pressure must be poly, channel or none";
        break;
      case "fader":
        if (!isInt(zone.cc, 0, 127)) return "cc must be 0 to 127";
        if (!isInt(zone.initial, 0, 127)) return "initial value must be 0 to 127";
        break;
      case "xy":
        if (!isInt(zone.ccX, 0, 127) || !isInt(zone.ccY, 0, 127)) return "ccX and ccY must be 0 to 127";
        if (width(zone) < 2 || height(zone) < 2) return "an xy pad needs at least 2 columns and 2 rows";
        break;
      case "toggle":
        if (["cc", "note", "program"].indexOf(zone.message) < 0) return "message must be cc, note or program";
        if (!isInt(zone.cc, 0, 127) || !isInt(zone.note, 0, 127) || !isInt(zone.program, 0, 127)) return "numbers must be 0 to 127";
        if (zone.perCell && toggleNumber(zone) + cells - 1 > 127) return "per cell numbers run past 127";
        break;
    }
    const clash = layout.zones.find(function (other) { return other.id !== zone.id && overlap(other, zone); });
    if (clash) return "overlaps " + describe(clash);
    const mine = noteRange(zone);
    if (mine) {
      const same = layout.zones.find(function (other) {
        if (other.id === zone.id || other.channel !== zone.channel) return false;
        const theirs = noteRange(other);
        return theirs && mine[0] <= theirs[1] && theirs[0] <= mine[1];
      });
      if (same) return "sends notes another zone sends on channel " + zone.channel + ": " + describe(same);
    }
    return null;
  }

  // the notes a zone can send, lowest and highest, or null for one that sends none; two zones
  // sending one note on one channel would cut each other's notes in the DAW
  function noteRange(zone) {
    const cells = width(zone) * height(zone);
    switch (zone.type) {
      case "pad": return zone.perCell ? [zone.note, zone.note + cells - 1] : [zone.note, zone.note];
      case "keys": return [zone.note, Math.min(127, zone.note + (zone.col1 - zone.col0) + (zone.row1 - zone.row0) * zone.rowInterval)];
      case "toggle": return zone.message === "note" ? (zone.perCell ? [zone.note, zone.note + cells - 1] : [zone.note, zone.note]) : null;
    }
    return null;
  }

  function range(a, b) { return a === b ? "" + a : a + " to " + b; }

  // "Fader CC 20", "Pad grid 36 to 51": the zone's name in a list
  function label(zone) {
    const cells = width(zone) * height(zone);
    switch (zone.type) {
      case "pad": return zone.perCell && cells > 1 ? "Pad grid " + range(zone.note, zone.note + cells - 1) : "Pad " + zone.note;
      case "keys": return "Keys " + zone.scale + " from " + zone.note;
      case "fader": return "Fader CC " + zone.cc;
      case "xy": return "XY pad CC " + zone.ccX + "/" + zone.ccY;
      case "toggle": {
        const kind = zone.latch ? "Toggle" : "Button";
        const n = toggleNumber(zone);
        return (zone.perCell && cells > 1 ? kind + "s " : kind + " ") + TOGGLE_WORDS[zone.message] + " " + (zone.perCell && cells > 1 ? range(n, n + cells - 1) : n);
      }
    }
    return zone.type;
  }

  // "columns 1 to 8, rows 0 to 3" or "column 9, rows 0 to 7"
  function place(zone) {
    const cols = (zone.col0 === zone.col1 ? "column " : "columns ") + range(zone.col0, zone.col1);
    const rows = (zone.row0 === zone.row1 ? "row " : "rows ") + range(zone.row0, zone.row1);
    return cols + ", " + rows;
  }

  // the zones in reading order on the surface: top row first, left to right
  function ordered(layout) {
    return layout.zones.slice().sort(function (a, b) { return b.row1 - a.row1 || a.col0 - b.col0; });
  }

  // the number a toggle sends: its controller, note or program
  function toggleNumber(zone) {
    return zone.message === "note" ? zone.note : zone.message === "program" ? zone.program : zone.cc;
  }
  const TOGGLE_WORDS = { cc: "CC", note: "note", program: "program" };

  // the note a cell of a keys zone plays, or -1 past the top of the range
  function keyNote(zone, col, row) {
    const note = zone.note + (col - zone.col0) + (row - zone.row0) * zone.rowInterval;
    return note > 127 ? -1 : note;
  }
  function keyDegree(zone, note) { return ((note - zone.note) % 12 + 12) % 12; }
  function inScale(zone, note) { return SCALES[zone.scale].indexOf(keyDegree(zone, note)) >= 0; }

  function describe(zone) {
    const place = zone.col0 === zone.col1 && zone.row0 === zone.row1 ? "at column " + zone.col0 + " row " + zone.row0
      : "columns " + zone.col0 + " to " + zone.col1 + ", rows " + zone.row0 + " to " + zone.row1;
    switch (zone.type) {
      case "pad": return "pad " + (zone.perCell && width(zone) * height(zone) > 1 ? "grid from note " + zone.note : "note " + zone.note) + " " + place;
      case "keys": return "keys from note " + zone.note + " in " + zone.scale + " " + place;
      case "fader": return "fader CC " + zone.cc + " " + place;
      case "xy": return "xy pad CC " + zone.ccX + "/" + zone.ccY + " " + place;
      case "toggle": return "toggle " + (zone.perCell ? "row from " : "") + TOGGLE_WORDS[zone.message] + " " + toggleNumber(zone) + " " + place;
    }
    return place;
  }

  let nextId = 1;
  function freshId(layout) {
    let id;
    do { id = "z" + (nextId++); } while (layout.zones.some(function (z) { return z.id === id; }));
    return id;
  }
  function addZone(layout, fields) {
    const zone = makeZone(fields);
    if (zone.id !== null && typeof zone.id !== "string") throw new Error("a zone id must be a string");
    if (!zone.id) zone.id = freshId(layout);
    if (layout.zones.some(function (z) { return z.id === zone.id; })) throw new Error("zone id " + zone.id + " is already used");
    const error = validateZone(layout, zone);
    if (error) throw new Error(error);
    layout.zones.push(zone);
    return zone;
  }

  function replaceZone(layout, id, fields) {
    const at = layout.zones.findIndex(function (z) { return z.id === id; });
    if (at < 0) throw new Error("no zone " + id);
    const zone = makeZone(Object.assign({}, fields, { id: id }));
    const error = validateZone(layout, zone);
    if (error) throw new Error(error);
    layout.zones[at] = zone;
    return zone;
  }

  function removeZone(layout, id) {
    layout.zones = layout.zones.filter(function (z) { return z.id !== id; });
  }

  function zoneAt(layout, col, row) {
    return layout.zones.find(function (z) { return covers(z, col, row); }) || null;
  }

  function serialize(layout) {
    return JSON.stringify(layout, null, 2);
  }

  // A layout from its JSON, checked zone by zone, so a file from elsewhere cannot break the page.
  function parse(text) {
    const data = typeof text === "string" ? JSON.parse(text) : text;
    if (!data || data.format !== FORMAT) throw new Error("not a LinnStrument surface layout");
    if (data.name !== undefined && typeof data.name !== "string") throw new Error("the name must be text");
    const layout = createLayout(data.model, data.name);
    if (layout.model !== 200 && layout.model !== 128) throw new Error("model must be 200 or 128");
    if (!Array.isArray(data.zones)) throw new Error("zones must be a list");
    data.zones.forEach(function (fields) { addZone(layout, fields); });
    return layout;
  }

  // Fits a layout made for the other model: zones past the last column are dropped, and so is
  // a zone that no longer works once cut at the edge, such as an xy pad left one column wide.
  function fitToModel(layout, model) {
    const fitted = createLayout(model, layout.name);
    const columns = ufm.columnsFor(model);
    layout.zones.forEach(function (z) {
      if (z.col0 >= columns) return;
      const cut = makeZone(Object.assign({}, z, { col1: Math.min(z.col1, columns - 1) }));
      if (!validateZone(fitted, cut)) fitted.zones.push(cut);
    });
    return fitted;
  }

  // A layout edited on a 128 after being cut from a 200 one, with what the 128 never showed
  // back: a zone cut at the edge and otherwise left in place regains its width, one deleted on
  // the 128 stays deleted, one the cut left out returns, or what lies past the edge of it when
  // the 128 edits took its place. Ids are only unique within a layout, so a zone added on the
  // 128 that took a hidden zone's id gets a new one.
  function widen(edited, wide) {
    if (edited.model !== 128 || wide.model !== 200) throw new Error("widen takes a 128 layout and the 200 one it was cut from");
    const last = ufm.columnsFor(128) - 1;
    const result = createLayout(200, edited.name);
    const was = {}, shown = {};
    wide.zones.forEach(function (z) { was[z.id] = z; });
    fitToModel(wide, 128).zones.forEach(function (z) { shown[z.id] = true; });
    const add = function (z) { try { addZone(result, z); return true; } catch (e) { return false; } };
    const taken = function (id) { return was[id] || result.zones.some(function (z) { return z.id === id; }); };
    edited.zones.forEach(function (z) {
      if (!was[z.id] || shown[z.id]) return add(z);
      let id;
      do { id = "z" + (nextId++); } while (taken(id));
      add(Object.assign({}, z, { id: id }));
    });
    edited.zones.forEach(function (z) {
      const w = shown[z.id] ? was[z.id] : null;
      if (w && w.col1 > last && z.col1 === last && z.col0 === w.col0 && z.row0 === w.row0 && z.row1 === w.row1) {
        try { replaceZone(result, z.id, Object.assign({}, z, { col1: w.col1 })); } catch (e) {}
      }
    });
    wide.zones.forEach(function (z) {
      if (!shown[z.id] && !add(z) && z.col0 <= last) add(Object.assign({}, z, { col0: last + 1 }));
    });
    return result;
  }

  // --- MIDI to the DAW

  function noteOn(channel, note, velocity) { return [0x90 | (channel - 1), note & 0x7F, velocity & 0x7F]; }
  function noteOff(channel, note) { return [0x80 | (channel - 1), note & 0x7F, 0]; }
  function polyPressure(channel, note, value) { return [0xA0 | (channel - 1), note & 0x7F, value & 0x7F]; }
  function channelPressure(channel, value) { return [0xD0 | (channel - 1), value & 0x7F]; }
  function control(channel, number, value) { return [0xB0 | (channel - 1), number & 0x7F, value & 0x7F]; }
  function programChange(channel, number) { return [0xC0 | (channel - 1), number & 0x7F]; }

  // What a zone sends for a DAW's MIDI learn: its first controller at the middle, or its first
  // note on and off, so the DAW can map it without a touch.
  function learnMessages(zone) {
    switch (zone.type) {
      case "pad": return [noteOn(zone.channel, zone.note, 100), noteOff(zone.channel, zone.note)];
      case "keys": return [noteOn(zone.channel, zone.note, 100), noteOff(zone.channel, zone.note)];
      case "fader": return [control(zone.channel, zone.cc, 64)];
      case "xy": return [control(zone.channel, zone.ccX, 64), control(zone.channel, zone.ccY, 64)];
      case "toggle": {
        const n = toggleNumber(zone);
        if (zone.message === "note") return [noteOn(zone.channel, n, 100), noteOff(zone.channel, n)];
        if (zone.message === "program") return [programChange(zone.channel, n)];
        return [control(zone.channel, n, 127), control(zone.channel, n, 0)];
      }
    }
    return [];
  }

  function to127(fraction) { return Math.max(0, Math.min(127, Math.round(fraction * 127))); }

  // where a touch is along a zone, 0 to 1, from the X value in surface units or the row and Y
  function alongColumns(zone, x) {
    const left = (zone.col0 - 1) * X_UNIT, right = zone.col1 * X_UNIT;
    return Math.max(0, Math.min(1, (x - left) / (right - left)));
  }
  function alongRows(zone, row, y) {
    return Math.max(0, Math.min(1, (row - zone.row0 + y / 127) / height(zone)));
  }

  // The live side: feed it the decoder's events, send what it returns. toInstrument are light
  // messages, toDaw the controller messages; frame() is the current light of every cell.
  // The layout may change while the runner lives: zones are read from it on every call and a
  // zone's state is made when it is first seen. A finger crossing from one cell to the next
  // arrives as a press on the new cell while the old one is still down, then a release of the
  // old one, so every zone tracks the cells it holds and only an empty zone counts as lifted.
  function createRunner(layout, initialControlLights) {
    const columns = columnsOf(layout);
    const state = {};
    let controlLights = [0, 0, 0, 0, 0, 0, 0, 0];
    if (initialControlLights) controlLights = initialControlLights.slice(0, ROWS).concat([0, 0, 0, 0, 0, 0, 0, 0]).slice(0, ROWS).map(function (c) { return isInt(c, 0, 11) ? c : 0; });
    function stateOf(z) {
      return state[z.id] || (state[z.id] = {
        type: z.type,
        held: {},        // cell key -> { note, channel } for pads and keys
        holders: {},     // "channel:note" -> how many held cells play it; cells can share a note
        pressures: {},   // cell key -> its latest pressure
        sent: {},        // note -> the aftertouch last sent for it
        fingers: {},     // cell key -> true for faders, xy pads and momentary buttons
        on: {}, onNote: {}, onCc: {}, onKind: {},
        pressureKind: z.pressure,
        value: z.type === "fader" ? z.initial : 0, valueX: 0.5, valueY: 0.5,
      });
    }
    function fingersDown(s) { return Object.keys(s.fingers).length > 0; }
    let shown = null;

    function frame() {
      const colors = [];
      for (let i = 0; i < columns * ROWS; i++) colors.push(0);
      const set = function (col, row, color) { colors[row * columns + col] = color; };
      for (let row = 0; row < ROWS; row++) set(0, row, controlLights[row]);
      layout.zones.forEach(function (z) {
        const s = stateOf(z);
        for (let row = z.row0; row <= z.row1; row++) {
          for (let col = z.col0; col <= z.col1; col++) {
            let color = z.color;
            switch (z.type) {
              case "pad":
                if (z.perCell ? s.held[col + ":" + row] : Object.keys(s.held).length > 0) color = z.activeColor;
                break;
              case "keys": {
                const note = keyNote(z, col, row);
                if (note < 0) color = 0;
                else if (s.held[col + ":" + row]) color = z.activeColor;
                else if (keyDegree(z, note) === 0) color = z.rootColor;
                else color = inScale(z, note) ? z.color : 0;
                break;
              }
              case "toggle":
                if (s.on[z.perCell ? cellIndex(z, col, row) : 0]) color = z.activeColor;
                break;
              case "fader": {
                const cells = horizontal(z) ? width(z) : height(z);
                const at = horizontal(z) ? col - z.col0 : row - z.row0;
                const level = Math.round(s.value / 127 * cells);
                let lit, edge;
                if (z.bipolar) {
                  // the fill grows from the middle towards the finger, both ways
                  const middle = Math.floor(cells / 2);
                  lit = level >= middle ? at >= middle && at < Math.max(level, middle + 1) : at >= level && at < middle;
                  edge = level >= middle ? Math.max(level - 1, middle) : level;
                } else {
                  lit = at < level;
                  edge = level - 1;
                }
                color = lit ? z.color : 0;
                if (fingersDown(s) && at === Math.max(0, edge)) color = z.activeColor;
                break;
              }
              case "xy": {
                const tc = z.col0 + Math.min(width(z) - 1, Math.floor(s.valueX * width(z)));
                const tr = z.row0 + Math.min(height(z) - 1, Math.floor(s.valueY * height(z)));
                color = col === tc && row === tr ? z.activeColor : (col === tc || row === tr) ? z.color : 0;
                break;
              }
            }
            set(col, row, color);
          }
        }
      });
      return colors;
    }

    // the lights that differ from what the instrument shows, or everything the first time
    function lightMessages() {
      const now = frame();
      let messages = [];
      if (!shown) {
        messages = ufm.paintRun(0, 0, now);
      } else {
        for (let i = 0; i < now.length; i++) {
          if (now[i] !== shown[i]) messages = messages.concat(ufm.setLed(i % columns, Math.floor(i / columns), now[i]));
        }
      }
      shown = now;
      return messages;
    }

    // the rows' axes, sent in full at a start and only where they changed afterwards
    let sentAxes = null;
    function axisMessages(all) {
      let messages = [];
      const need = {};
      layout.zones.forEach(function (z) {
        for (let row = z.row0; row <= z.row1; row++) {
          const axes = need[row] || (need[row] = {});
          if (z.type === "xy" || (z.type === "fader" && horizontal(z))) axes.x = true;
          if (z.type === "xy" || (z.type === "fader" && !horizontal(z))) axes.y = true;
          if ((z.type === "pad" || z.type === "keys") && z.pressure !== "none") axes.z = true;
        }
      });
      const now = {};
      for (let row = 0; row < ROWS; row++) {
        ["x", "y", "z"].forEach(function (axis) {
          const on = !!(need[row] && need[row][axis]);
          now[row + axis] = on;
          if (all || !sentAxes || sentAxes[row + axis] !== on) messages = messages.concat(ufm.enableAxis(row, axis, on));
        });
      }
      sentAxes = now;
      return messages;
    }

    // enter the mode, switch on the touch data each row's zones need, and paint the layout
    function start() {
      shown = null;
      return ufm.enterUserFirmwareMode().concat(axisMessages(true), lightMessages());
    }

    // the same without entering the mode, for a layout switched while running
    function resume() {
      shown = null;
      return axisMessages(true).concat(lightMessages());
    }

    // lets go of one held cell; the note goes off when no other cell of the zone plays it
    function holderKey(h) { return h.channel + ":" + h.note; }
    function releaseCell(s, key) {
      const h = s.held[key];
      delete s.held[key];
      delete s.pressures[key];
      if (--s.holders[holderKey(h)] <= 0) {
        delete s.holders[holderKey(h)];
        delete s.sent[holderKey(h)];
        return [noteOff(h.channel, h.note)];
      }
      return [];
    }

    // everything a zone's state still holds in the DAW goes off, and the state is dropped
    function releaseState(id) {
      const s = state[id];
      let releases = [];
      Object.keys(s.held).forEach(function (key) { releases = releases.concat(releaseCell(s, key)); });
      Object.keys(s.onNote).forEach(function (i) { if (s.on[i]) releases.push(noteOff(s.onNote[i].channel, s.onNote[i].note)); });
      Object.keys(s.onCc).forEach(function (i) { if (s.on[i]) releases.push(control(s.onCc[i].channel, s.onCc[i].number, 0)); });
      delete state[id];
      return releases;
    }

    // the note a cell of a note zone plays now, or -1
    function cellNote(z, col, row) {
      if (z.type === "keys") return keyNote(z, col, row);
      if (z.type === "pad") return z.perCell ? z.note + cellIndex(z, col, row) : z.note;
      return -1;
    }

    // After the layout changed while running: zones that are gone, or whose type changed,
    // release everything; a zone that was edited releases what no longer fits it, a held cell
    // outside its new rectangle or one that would now play another note or channel, and keeps
    // the rest; the axes follow the zones and the lights are brought up to date.
    function refresh() {
      let releases = [];
      Object.keys(state).forEach(function (id) {
        const z = layout.zones.find(function (zone) { return zone.id === id; });
        const s = state[id];
        if (!z || s.type !== z.type) { releases = releases.concat(releaseState(id)); return; }
        // a zone that sends no aftertouch forgets what its cells press, so switching it back
        // on starts from the next value, not a stale one
        if (z.pressure === "none") Object.keys(s.pressures).forEach(function (k) { s.pressures[k] = 0; });
        // a change of the aftertouch kind zeroes what was sent the old way
        if (s.pressureKind !== z.pressure) {
          Object.keys(s.sent).forEach(function (hk) {
            if (!s.sent[hk]) return;
            const held = Object.keys(s.held).map(function (k) { return s.held[k]; }).find(function (h) { return holderKey(h) === hk; });
            if (held) releases.push(s.pressureKind === "poly" ? polyPressure(held.channel, held.note, 0) : channelPressure(held.channel, 0));
          });
          s.sent = {};
          s.pressureKind = z.pressure;
        }
        Object.keys(s.held).forEach(function (key) {
          const parts = key.split(":");
          const col = parseInt(parts[0], 10), row = parseInt(parts[1], 10);
          const h = s.held[key];
          if (!covers(z, col, row) || cellNote(z, col, row) !== h.note || z.channel !== h.channel) releases = releases.concat(releaseCell(s, key));
        });
        // a note still held by other cells follows their pressure now
        if (z.pressure !== "none") {
          Object.keys(s.holders).forEach(function (hk) {
            const held = Object.keys(s.held).map(function (k) { return s.held[k]; }).find(function (h) { return holderKey(h) === hk; });
            if (held) releases = releases.concat(notePressure(z, s, held.note, held.channel));
          });
        }
        Object.keys(s.fingers).forEach(function (key) {
          const parts = key.split(":");
          const col = parseInt(parts[0], 10), row = parseInt(parts[1], 10);
          if (!covers(z, col, row)) { delete s.fingers[key]; return; }
          // a finger on a switch whose index moved with the edit counts for the new index
          if (z.type === "toggle") s.fingers[key] = z.perCell ? cellIndex(z, col, row) : 0;
        });
        if (z.type === "xy" && z.spring && !fingersDown(s)) releases = releases.concat(springBack(z, s));
        if (z.type === "toggle") {
          // what no longer matches the switch goes off; then a momentary switch comes back on
          // under any finger still on it, with its new message, number or channel
          Object.keys(s.on).forEach(function (i) {
            if (!s.on[i]) return;
            const index = parseInt(i, 10);
            const cells = width(z) * height(z);
            const held = Object.keys(s.fingers).some(function (k) { return s.fingers[k] === index; });
            const stale = (z.perCell ? index >= cells : index > 0) || s.onKind[i] !== z.message || (!z.latch && !held) ||
              (s.onNote[i] && (s.onNote[i].channel !== z.channel || s.onNote[i].note !== toggleNumber(z) + index)) ||
              (s.onCc[i] && (s.onCc[i].channel !== z.channel || s.onCc[i].number !== toggleNumber(z) + index));
            if (!stale) return;
            if (s.onNote[i]) releases.push(noteOff(s.onNote[i].channel, s.onNote[i].note));
            if (s.onCc[i]) releases.push(control(s.onCc[i].channel, s.onCc[i].number, 0));
            delete s.onNote[i]; delete s.onCc[i]; delete s.onKind[i]; s.on[i] = false;
          });
          if (!z.latch) {
            Object.keys(s.fingers).forEach(function (k) {
              const index = s.fingers[k];
              if (s.on[index]) return;
              const number = toggleNumber(z) + index;
              s.on[index] = true;
              s.onKind[index] = z.message;
              if (z.message === "note") { s.onNote[index] = { note: number, channel: z.channel }; releases.push(noteOn(z.channel, number, 100)); }
              else if (z.message === "program") releases.push(programChange(z.channel, number));
              else { s.onCc[index] = { number: number, channel: z.channel }; releases.push(control(z.channel, number, 127)); }
            });
          }
        }
      });
      return { toDaw: releases, toInstrument: axisMessages(false).concat(lightMessages()) };
    }

    // Every note still sounding goes off, from the state table rather than the layout, so a
    // zone removed or changed while its notes were held is released too.
    function stop() {
      let releases = [];
      Object.keys(state).forEach(function (id) { releases = releases.concat(releaseState(id)); });
      return { toDaw: releases, toInstrument: ufm.leaveUserFirmwareMode() };
    }

    function setControlLights(colors) {
      controlLights = colors.slice(0, ROWS).map(function (c) { return isInt(c, 0, 11) ? c : 0; });
      while (controlLights.length < ROWS) controlLights.push(0);
      return lightMessages();
    }

    // several cells can play one note (a big pad, two keys of a grid), so a note's aftertouch
    // is the strongest pressure among the cells holding it, sent only when that changes
    function notePressure(z, s, note, channel) {
      let strongest = 0;
      const hk = channel + ":" + note;
      Object.keys(s.pressures).forEach(function (k) {
        if (s.held[k] && holderKey(s.held[k]) === hk && s.pressures[k] > strongest) strongest = s.pressures[k];
      });
      const last = s.sent[hk] === undefined ? 0 : s.sent[hk];
      if (strongest === last) return [];
      s.sent[hk] = strongest;
      return [z.pressure === "poly" ? polyPressure(channel, note, strongest) : channelPressure(channel, strongest)];
    }

    function faderValue(z, s, row, x, y) {
      const fraction = horizontal(z) ? alongColumns(z, x) : alongRows(z, row, y);
      const value = to127(fraction);
      if (value === s.value) return [];
      s.value = value;
      return [control(z.channel, z.cc, value)];
    }

    function xyValue(z, s, row, x, y) {
      const out = [];
      if (x !== null) {
        const fx = alongColumns(z, x);
        if (to127(fx) !== to127(s.valueX)) out.push(control(z.channel, z.ccX, to127(fx)));
        s.valueX = fx;
      }
      if (y !== null) {
        const fy = alongRows(z, row, y);
        if (to127(fy) !== to127(s.valueY)) out.push(control(z.channel, z.ccY, to127(fy)));
        s.valueY = fy;
      }
      return out;
    }

    // the pad springs back to its middle, like a joystick
    function springBack(z, s) {
      const out = [];
      if (to127(s.valueX) !== 64) out.push(control(z.channel, z.ccX, 64));
      if (to127(s.valueY) !== 64) out.push(control(z.channel, z.ccY, 64));
      s.valueX = 0.5;
      s.valueY = 0.5;
      return out;
    }

    // A press on a control switch comes back as control: its row, for the page to act on; the
    // Global Settings switch at row 0 opens the firmware's panel instead and never arrives.
    function handle(event) {
      const empty = { toDaw: [], toInstrument: [] };
      if (!event || !Number.isInteger(event.col) || !Number.isInteger(event.row) || event.col < 0 || event.col >= columns || event.row < 0 || event.row >= ROWS) return empty;
      if (event.col === 0) return event.type === "press" ? { toDaw: [], toInstrument: [], control: event.row } : empty;
      const z = zoneAt(layout, event.col, event.row);
      if (!z) return empty;
      const s = stateOf(z);
      const key = event.col + ":" + event.row;
      let toDaw = [];
      switch (z.type) {
        case "pad":
        case "keys": {
          const note = cellNote(z, event.col, event.row);
          if (note < 0) break;
          if (event.type === "press") {
            if (s.held[key]) break;
            const h = { note: note, channel: z.channel };
            s.held[key] = h;
            s.pressures[key] = 0;
            s.holders[holderKey(h)] = (s.holders[holderKey(h)] || 0) + 1;
            if (s.holders[holderKey(h)] === 1) toDaw.push(noteOn(z.channel, note, event.velocity));
          } else if (event.type === "release" && s.held[key]) {
            const h = s.held[key];
            toDaw = releaseCell(s, key);
            if (s.holders[holderKey(h)] && z.pressure !== "none") toDaw = toDaw.concat(notePressure(z, s, h.note, h.channel));
          } else if (event.type === "pressure" && s.held[key] && z.pressure !== "none") {
            s.pressures[key] = event.value;
            toDaw = notePressure(z, s, s.held[key].note, s.held[key].channel);
          }
          break;
        }
        case "toggle": {
          const index = z.perCell ? cellIndex(z, event.col, event.row) : 0;
          const number = toggleNumber(z) + index;
          const emit = function (on) {
            if (!on) {
              // off lets go of whatever the switch holds, whichever message it was switched on with
              if (s.onNote[index]) toDaw.push(noteOff(s.onNote[index].channel, s.onNote[index].note));
              if (s.onCc[index]) toDaw.push(control(s.onCc[index].channel, s.onCc[index].number, 0));
              delete s.onNote[index]; delete s.onCc[index]; delete s.onKind[index];
              return;
            }
            s.onKind[index] = z.message;
            if (z.message === "note") { s.onNote[index] = { note: number, channel: z.channel }; toDaw.push(noteOn(z.channel, number, event.velocity || 100)); }
            else if (z.message === "program") toDaw.push(programChange(z.channel, number));
            else { s.onCc[index] = { number: number, channel: z.channel }; toDaw.push(control(z.channel, number, 127)); }
          };
          // fingers are counted per cell, so a press repeated for a cell still down, or a finger
          // crossing into the next cell of the same switch, is not another press
          const fingersOn = function () { return Object.keys(s.fingers).filter(function (k) { return s.fingers[k] === index; }).length; };
          if (event.type === "press") {
            const first = fingersOn() === 0;
            s.fingers[key] = index;
            if (!first) break;
            if (z.latch) { s.on[index] = !s.on[index]; emit(s.on[index]); }
            else { s.on[index] = true; emit(true); }
          } else if (event.type === "release" && s.fingers[key] !== undefined) {
            delete s.fingers[key];
            if (!z.latch && fingersOn() === 0 && s.on[index]) { s.on[index] = false; emit(false); }
          }
          break;
        }
        case "fader":
          // the value waits for the first X or Y of the touch, which the instrument always sends;
          // a position from a finger that was down before the run counts as a finger too
          if (event.type === "press") s.fingers[key] = true;
          else if (event.type === "release") delete s.fingers[key];
          else if (event.type === "x" && horizontal(z)) { s.fingers[key] = true; toDaw = faderValue(z, s, event.row, event.value, null); }
          else if (event.type === "y" && !horizontal(z)) { s.fingers[key] = true; toDaw = faderValue(z, s, event.row, null, event.value); }
          break;
        case "xy":
          if (event.type === "press") s.fingers[key] = true;
          else if (event.type === "x") { s.fingers[key] = true; toDaw = xyValue(z, s, event.row, event.value, null); }
          else if (event.type === "y") { s.fingers[key] = true; toDaw = xyValue(z, s, event.row, null, event.value); }
          else if (event.type === "release") {
            delete s.fingers[key];
            if (z.spring && !fingersDown(s)) toDaw = springBack(z, s);
          }
          break;
      }
      return { toDaw: toDaw, toInstrument: lightMessages() };
    }

    return { start: start, resume: resume, stop: stop, refresh: refresh, handle: handle, frame: frame, setControlLights: setControlLights };
  }

  // --- layouts to start from, built for the model's column count

  function presets(model) {
    const columns = ufm.columnsFor(model);
    const last = columns - 1;
    const list = [];

    const drums = createLayout(model, "Drums, faders and switches");
    addZone(drums, { type: "pad", col0: 1, col1: 8, row0: 0, row1: 3, note: 36, perCell: true, color: ufm.COLORS.orange });
    addZone(drums, { type: "toggle", col0: 1, col1: 8, row0: 7, row1: 7, cc: 102, perCell: true });
    addZone(drums, { type: "fader", col0: 1, col1: 8, row0: 5, row1: 5, cc: 7, color: ufm.COLORS.cyan });
    addZone(drums, { type: "fader", col0: 1, col1: 8, row0: 4, row1: 4, cc: 10, color: ufm.COLORS.magenta });
    for (let i = 0; i < 8 && 9 + i <= last; i++) {
      addZone(drums, { type: "fader", col0: 9 + i, col1: 9 + i, row0: 0, row1: 7, cc: 20 + i, color: ufm.COLORS.green });
    }
    if (last >= 24) addZone(drums, { type: "xy", col0: 17, col1: 24, row0: 0, row1: 7, ccX: 28, ccY: 29, color: ufm.COLORS.blue });
    list.push(drums);

    const pads = createLayout(model, "XY pads");
    const across = last >= 24 ? 3 : 2;
    const padWidth = Math.floor(last / across);
    let cc = 20;
    for (let r = 0; r < 2; r++) {
      for (let c = 0; c < across; c++) {
        const col0 = 1 + c * padWidth;
        addZone(pads, { type: "xy", col0: col0, col1: col0 + padWidth - 1, row0: r * 4, row1: r * 4 + 3,
                        ccX: cc, ccY: cc + 1, color: [ufm.COLORS.blue, ufm.COLORS.green, ufm.COLORS.magenta][(r * across + c) % 3] });
        cc += 2;
      }
    }
    list.push(pads);

    const dj = createLayout(model, "Two decks");
    addZone(dj, { type: "fader", col0: 1, col1: last, row0: 0, row1: 0, cc: 20, color: ufm.COLORS.cyan });
    addZone(dj, { type: "fader", col0: 1, col1: 1, row0: 1, row1: 7, cc: 21, color: ufm.COLORS.green });
    addZone(dj, { type: "fader", col0: last, col1: last, row0: 1, row1: 7, cc: 22, color: ufm.COLORS.green });
    addZone(dj, { type: "pad", col0: 2, col1: 5, row0: 1, row1: 2, note: 36, perCell: true, color: ufm.COLORS.orange });
    addZone(dj, { type: "pad", col0: last - 4, col1: last - 1, row0: 1, row1: 2, note: 44, perCell: true, color: ufm.COLORS.orange });
    addZone(dj, { type: "toggle", col0: 2, col1: 3, row0: 7, row1: 7, cc: 102, perCell: true, color: ufm.COLORS.red, activeColor: ufm.COLORS.lime });
    addZone(dj, { type: "toggle", col0: last - 2, col1: last - 1, row0: 7, row1: 7, cc: 104, perCell: true, color: ufm.COLORS.red, activeColor: ufm.COLORS.lime });
    addZone(dj, { type: "fader", col0: 2, col1: 5, row0: 4, row1: 4, cc: 23, color: ufm.COLORS.yellow });
    addZone(dj, { type: "fader", col0: last - 4, col1: last - 1, row0: 4, row1: 4, cc: 24, color: ufm.COLORS.yellow });
    list.push(dj);

    const keys = createLayout(model, "Keys and faders");
    addZone(keys, { type: "keys", col0: 1, col1: last - 8, row0: 0, row1: 7, note: 48, rowInterval: 5, scale: "major" });
    for (let i = 0; i < 8; i++) {
      addZone(keys, { type: "fader", col0: last - 7 + i, col1: last - 7 + i, row0: 0, row1: 7, cc: 20 + i, color: i < 4 ? ufm.COLORS.cyan : ufm.COLORS.magenta });
    }
    list.push(keys);

    // strips of two columns: volume and pan with mute and solo under them, each on its own channel
    const mixer = createLayout(model, "Mixer");
    const strips = Math.floor((last - (last >= 24 ? 1 : 0)) / 2);
    for (let i = 0; i < strips; i++) {
      const col = 1 + i * 2;
      addZone(mixer, { type: "fader", col0: col, col1: col, row0: 1, row1: 7, cc: 7, channel: i + 1, color: ufm.COLORS.green });
      addZone(mixer, { type: "fader", col0: col + 1, col1: col + 1, row0: 1, row1: 7, cc: 10, channel: i + 1, bipolar: true, initial: 64, color: ufm.COLORS.cyan });
      addZone(mixer, { type: "toggle", col0: col, col1: col, row0: 0, row1: 0, cc: 102, channel: i + 1, perCell: false, color: ufm.COLORS.red, activeColor: ufm.COLORS.white });
      addZone(mixer, { type: "toggle", col0: col + 1, col1: col + 1, row0: 0, row1: 0, cc: 103, channel: i + 1, perCell: false, color: ufm.COLORS.yellow, activeColor: ufm.COLORS.white });
    }
    if (last >= 24) addZone(mixer, { type: "fader", col0: last, col1: last, row0: 0, row1: 7, cc: 7, channel: 16, color: ufm.COLORS.orange });
    list.push(mixer);

    // two colors per row in a checkerboard, so neighbouring pads tell apart
    const big = createLayout(model, "Big pads");
    const pairs = [[ufm.COLORS.orange, ufm.COLORS.red], [ufm.COLORS.cyan, ufm.COLORS.blue],
                   [ufm.COLORS.yellow, ufm.COLORS.lime], [ufm.COLORS.pink, ufm.COLORS.magenta]];
    let note = 36;
    for (let r = 0; r < 4; r++) {
      for (let c = 0; c + 1 <= last - 1; c += 2) {
        addZone(big, { type: "pad", col0: 1 + c, col1: 2 + c, row0: r * 2, row1: r * 2 + 1, note: note++, perCell: false,
                       color: pairs[r][(r + c / 2) % 2] });
      }
    }
    if (last % 2 === 1) addZone(big, { type: "fader", col0: last, col1: last, row0: 0, row1: 7, cc: 7, color: ufm.COLORS.green });
    list.push(big);

    // 16 by 8 is 128 cells, one note each, for a clip launcher mapped by MIDI learn
    const clips = createLayout(model, "Clip grid");
    addZone(clips, { type: "toggle", col0: 1, col1: 16, row0: 0, row1: 7, message: "note", note: 0, latch: false, perCell: true,
                     color: ufm.COLORS.green, activeColor: ufm.COLORS.white });
    for (let i = 0; 17 + i <= last; i++) {
      addZone(clips, { type: "fader", col0: 17 + i, col1: 17 + i, row0: 0, row1: 7, cc: 20 + i, color: i < 8 ? ufm.COLORS.cyan : ufm.COLORS.orange });
    }
    list.push(clips);

    const macros = createLayout(model, "Eight macros");
    for (let row = 0; row < 8; row++) {
      addZone(macros, { type: "fader", col0: 1, col1: last, row0: row, row1: row, cc: 20 + row,
                        color: [ufm.COLORS.red, ufm.COLORS.orange, ufm.COLORS.yellow, ufm.COLORS.lime, ufm.COLORS.green, ufm.COLORS.cyan, ufm.COLORS.blue, ufm.COLORS.magenta][row] });
    }
    list.push(macros);

    const padsKeys = createLayout(model, "Pads and keys");
    addZone(padsKeys, { type: "pad", col0: 1, col1: 4, row0: 0, row1: 3, note: 36, perCell: true, color: ufm.COLORS.orange });
    addZone(padsKeys, { type: "toggle", col0: 1, col1: 4, row0: 5, row1: 5, cc: 102, perCell: true });
    addZone(padsKeys, { type: "fader", col0: 1, col1: 4, row0: 7, row1: 7, cc: 7, color: ufm.COLORS.cyan });
    addZone(padsKeys, { type: "keys", col0: 5, col1: last, row0: 0, row1: 7, note: 48, rowInterval: 5, scale: "pentatonic", channel: 2 });
    list.push(padsKeys);

    return list;
  }

  return {
    TYPES: TYPES,
    SCALES: SCALES,
    FORMAT: FORMAT,
    X_UNIT: X_UNIT,
    DEFAULTS: DEFAULTS,
    createLayout: createLayout,
    makeZone: makeZone,
    validateZone: validateZone,
    addZone: addZone,
    replaceZone: replaceZone,
    removeZone: removeZone,
    zoneAt: zoneAt,
    describe: describe,
    label: label,
    place: place,
    ordered: ordered,
    serialize: serialize,
    parse: parse,
    fitToModel: fitToModel,
    widen: widen,
    createRunner: createRunner,
    learnMessages: learnMessages,
    keyNote: keyNote,
    presets: presets,
  };
});
