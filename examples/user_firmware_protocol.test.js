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
// Run with: node --test examples/user_firmware_protocol.test.js
const test = require("node:test");
const assert = require("node:assert/strict");
const ufm = require("./user_firmware_protocol.js");

test("entering the mode is NRPN 245 value 1 as four control changes on channel 1", () => {
  assert.deepEqual(ufm.enterUserFirmwareMode(), [
    [0xB0, 99, 1], [0xB0, 98, 117], [0xB0, 6, 0], [0xB0, 38, 1],
  ]);
});

test("leaving the mode sends value 0", () => {
  assert.deepEqual(ufm.leaveUserFirmwareMode()[3], [0xB0, 38, 0]);
});

test("the query asks NRPN 299 for parameter 245", () => {
  assert.deepEqual(ufm.queryUserFirmwareMode(), [
    [0xB0, 99, 2], [0xB0, 98, 43], [0xB0, 6, 1], [0xB0, 38, 117],
  ]);
});

test("an axis is enabled with CC 9 to 12 on the row's channel", () => {
  assert.deepEqual(ufm.enableAxis(3, "x", true), [[0xB3, 10, 1]]);
  assert.deepEqual(ufm.enableAxis(0, "y", true), [[0xB0, 11, 1]]);
  assert.deepEqual(ufm.enableAxis(7, "z", false), [[0xB7, 12, 0]]);
  assert.deepEqual(ufm.enableAxis(2, "slide", true), [[0xB2, 9, 1]]);
  assert.throws(() => ufm.enableAxis(8, "x", true));
  assert.throws(() => ufm.enableAxis(1, "w", true));
});

test("decimation is CC 13 in milliseconds", () => {
  assert.deepEqual(ufm.setDecimation(50), [[0xB0, 13, 50]]);
  assert.throws(() => ufm.setDecimation(200));
});

test("a light is CC 20, 21, 22 with the column, row and color", () => {
  assert.deepEqual(ufm.setLed(5, 3, ufm.COLORS.green), [[0xB0, 20, 5], [0xB0, 21, 3], [0xB0, 22, 3]]);
  assert.deepEqual(ufm.clearLed(5, 3)[2], [0xB0, 22, 0]);
  assert.throws(() => ufm.setLed(26, 0, 1));
  assert.throws(() => ufm.setLed(1, 0, 12));
});

test("the models have 26 and 17 columns including the switch column", () => {
  assert.equal(ufm.columnsFor(200), 26);
  assert.equal(ufm.columnsFor(128), 17);
  assert.throws(() => ufm.columnsFor(100));
});

test("a note on is a press with the column and the row", () => {
  const d = ufm.createDecoder();
  assert.deepEqual(d.decode([0x93, 5, 100]), { type: "press", col: 5, row: 3, velocity: 100 });
});

test("a note off carries the release velocity", () => {
  const d = ufm.createDecoder();
  assert.deepEqual(d.decode([0x83, 5, 127]), { type: "release", col: 5, row: 3, velocity: 127 });
  assert.deepEqual(d.decode([0x93, 5, 0]), { type: "release", col: 5, row: 3, velocity: 0 });
});

test("poly pressure is Z", () => {
  const d = ufm.createDecoder();
  assert.deepEqual(d.decode([0xA0, 12, 64]), { type: "pressure", col: 12, row: 0, value: 64 });
});

test("X arrives as the LSB on CC 32 plus the column, then the MSB on the column", () => {
  const d = ufm.createDecoder();
  assert.equal(d.decode([0xB3, 37, 0x44]), null);
  assert.deepEqual(d.decode([0xB3, 5, 0x0F]), { type: "x", col: 5, row: 3, value: (0x0F << 7) | 0x44 });
});

test("X values are kept per row and column", () => {
  const d = ufm.createDecoder();
  d.decode([0xB3, 37, 1]);
  d.decode([0xB2, 37, 2]);
  assert.equal(d.decode([0xB3, 5, 0]).value, 1);
  assert.equal(d.decode([0xB2, 5, 0]).value, 2);
});

test("Y is CC 64 plus the column", () => {
  const d = ufm.createDecoder();
  assert.deepEqual(d.decode([0xB3, 69, 100]), { type: "y", col: 5, row: 3, value: 100 });
});

test("CC 119 announces the column a slide is leaving", () => {
  const d = ufm.createDecoder();
  assert.deepEqual(d.decode([0xB3, 119, 5]), { type: "slide", row: 3, fromCol: 5 });
});

test("a slide is CC 119, a note on for the new cell, then a note off for the old one", () => {
  const d = ufm.createDecoder();
  const events = [[0xB3, 119, 5], [0x93, 6, 100], [0x83, 5, 6]].map((m) => d.decode(m));
  assert.deepEqual(events.map((e) => e.type), ["slide", "press", "release"]);
  assert.equal(events[1].velocity, 100);
  assert.equal(events[2].velocity, 6);
});

test("the instrument announces mode changes with NRPN 245 on channel 9", () => {
  const d = ufm.createDecoder();
  const messages = [[0xB8, 99, 1], [0xB8, 98, 117], [0xB8, 6, 0], [0xB8, 38, 1], [0xB8, 101, 127], [0xB8, 100, 127]];
  const events = messages.map((m) => d.decode(m)).filter(Boolean);
  assert.deepEqual(events, [{ type: "mode", active: true, channel: 9 }]);
});

test("other NRPN replies come through with their number and value", () => {
  const d = ufm.createDecoder();
  const events = [[0xB0, 99, 1], [0xB0, 98, 118], [0xB0, 6, 0], [0xB0, 38, 1]].map((m) => d.decode(m)).filter(Boolean);
  assert.deepEqual(events, [{ type: "nrpn", number: 246, value: 1, channel: 1 }]);
});

test("column 6's X shares CC 6 and 38 with NRPN data entry and is told apart by context", () => {
  const d = ufm.createDecoder();
  assert.equal(d.decode([0xB3, 38, 0x10]), null);
  assert.deepEqual(d.decode([0xB3, 6, 0x05]), { type: "x", col: 6, row: 3, value: (0x05 << 7) | 0x10 });
  d.decode([0xB3, 99, 1]);
  d.decode([0xB3, 98, 117]);
  assert.equal(d.decode([0xB3, 6, 0]), null);
  assert.deepEqual(d.decode([0xB3, 38, 1]), { type: "mode", active: true, channel: 4 });
  assert.deepEqual(d.decode([0xB3, 6, 0x05]), { type: "x", col: 6, row: 3, value: (0x05 << 7) | 0x10 });
});

test("the surface state follows presses, expression and releases", () => {
  const s = ufm.createSurfaceState(26);
  assert.equal(s.apply({ type: "press", col: 5, row: 3, velocity: 90 }).pressed, true);
  s.apply({ type: "pressure", col: 5, row: 3, value: 40 });
  s.apply({ type: "x", col: 5, row: 3, value: 1000 });
  s.apply({ type: "y", col: 5, row: 3, value: 64 });
  assert.deepEqual(s.cell(5, 3), { col: 5, row: 3, pressed: true, velocity: 90, pressure: 40, x: 1000, y: 64 });
  assert.equal(s.pressedCells().length, 1);
  s.apply({ type: "release", col: 5, row: 3, velocity: 127 });
  assert.equal(s.cell(5, 3).pressed, false);
  assert.equal(s.pressedCells().length, 0);
});

test("the surface state ignores columns beyond the model and events without a cell", () => {
  const s = ufm.createSurfaceState(17);
  assert.equal(s.apply({ type: "press", col: 20, row: 0, velocity: 90 }), null);
  assert.equal(s.apply({ type: "mode", active: true, channel: 9 }), null);
  assert.equal(s.apply({ type: "slide", row: 3, fromCol: 5 }), null);
  assert.equal(s.apply(null), null);
});

test("unrelated and truncated messages decode to nothing", () => {
  const d = ufm.createDecoder();
  assert.equal(d.decode([0xB0, 64, 127]), null);
  assert.equal(d.decode([0xE0, 0, 64]), null);
  assert.equal(d.decode([0xF8]), null);
  assert.equal(d.decode([0x93, 5]), null);
  assert.equal(d.decode([]), null);
});

test("the null RPN that ends a reply closes the NRPN, so column 6's X decodes again", () => {
  const d = ufm.createDecoder();
  d.decode([0xB3, 99, 1]);
  d.decode([0xB3, 98, 117]);
  assert.equal(d.decode([0xB3, 101, 127]), null);
  assert.equal(d.decode([0xB3, 100, 127]), null);
  assert.equal(d.decode([0xB3, 38, 0x10]), null);
  assert.deepEqual(d.decode([0xB3, 6, 0x05]), { type: "x", col: 6, row: 3, value: (0x05 << 7) | 0x10 });
});

test("Web MIDI's typed arrays decode like plain arrays", () => {
  const d = ufm.createDecoder();
  assert.deepEqual(d.decode(new Uint8Array([0x93, 5, 100])), { type: "press", col: 5, row: 3, velocity: 100 });
});

test("the model query asks NRPN 299 for parameter 272, the version query for 273 and 274", () => {
  assert.deepEqual(ufm.queryModel(), [
    [0xB0, 99, 2], [0xB0, 98, 43], [0xB0, 6, 2], [0xB0, 38, 16],
  ]);
  const version = ufm.queryVersion();
  assert.equal(version.length, 8);
  assert.deepEqual(version[3], [0xB0, 38, 17]);
  assert.deepEqual(version[7], [0xB0, 38, 18]);
});

test("a run of lights is CC 20 and 21 for the first cell, then one CC 25 per color", () => {
  assert.deepEqual(ufm.paintRun(24, 3, [1, 2, 3]), [
    [0xB0, 20, 24], [0xB0, 21, 3], [0xB0, 25, 1], [0xB0, 25, 2], [0xB0, 25, 3],
  ]);
  assert.throws(() => ufm.paintRun(0, 0, [12]));
  assert.throws(() => ufm.paintRun(26, 0, [1]));
});

test("text is drawn with the cell, CC 26 for the font, a cleared text, one CC 27 per character and CC 28", () => {
  assert.deepEqual(ufm.drawText(3, 0, "big", ufm.COLORS.green, "Hi"), [
    [0xB0, 20, 3], [0xB0, 21, 0], [0xB0, 26, 2],
    [0xB0, 27, 0], [0xB0, 27, 72], [0xB0, 27, 105],
    [0xB0, 28, 3],
  ]);
  assert.deepEqual(ufm.drawText(1, 4, 3, 1, "A")[2], [0xB0, 26, 3]);
});

test("scrolling text needs only the row and ends with CC 29", () => {
  const messages = ufm.scrollText(2, "condensed", ufm.COLORS.blue, "GO");
  assert.deepEqual(messages[0], [0xB0, 21, 2]);
  assert.deepEqual(messages[1], [0xB0, 26, 3]);
  assert.deepEqual(messages[messages.length - 1], [0xB0, 29, 5]);
  assert.equal(messages.length, 6);
});

test("text is 1 to 32 ASCII characters in a known font", () => {
  assert.throws(() => ufm.drawText(1, 0, "big", 1, ""));
  assert.throws(() => ufm.drawText(1, 0, "big", 1, "a".repeat(33)));
  assert.throws(() => ufm.drawText(1, 0, "big", 1, "café"));
  assert.throws(() => ufm.drawText(1, 0, "huge", 1, "a"));
  assert.throws(() => ufm.drawText(1, 0, 4, 1, "a"));
  assert.equal(ufm.scrollText(0, "tiny", 1, "a".repeat(32)).length, 36);
});

test("the model reply decodes with its column count, the version and build replies with their numbers", () => {
  const decoder = ufm.createDecoder();
  const reply = (number, value) => {
    decoder.decode([0xB0, 99, number >> 7]);
    decoder.decode([0xB0, 98, number & 0x7F]);
    decoder.decode([0xB0, 6, value >> 7]);
    return decoder.decode([0xB0, 38, value & 0x7F]);
  };
  assert.deepEqual(reply(272, 200), { type: "model", model: 200, columns: 26, channel: 1 });
  assert.deepEqual(reply(272, 128), { type: "model", model: 128, columns: 17, channel: 1 });
  assert.deepEqual(reply(273, 234), { type: "version", version: 234, channel: 1 });
  assert.deepEqual(reply(274, 74), { type: "build", build: 74, channel: 1 });
  assert.deepEqual(ufm.queryCalibrated()[3], [0xB0, 38, 19]);
  assert.deepEqual(reply(275, 0), { type: "calibrated", calibrated: false, channel: 1 });
  assert.deepEqual(reply(275, 1), { type: "calibrated", calibrated: true, channel: 1 });
  assert.equal(reply(272, 7).type, "nrpn");
});
