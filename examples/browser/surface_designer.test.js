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
// Run with: node --test examples/browser/surface_designer.test.js
const test = require("node:test");
const assert = require("node:assert/strict");
const ufm = require("./user_firmware_protocol.js");
const designer = require("./surface_designer.js");

function layoutWith(zones, model) {
  const layout = designer.createLayout(model || 200, "test");
  zones.forEach((z) => designer.addZone(layout, z));
  return layout;
}

// the decoder's events, as the instrument would send them
const press = (col, row, velocity) => ({ type: "press", col, row, velocity: velocity || 100 });
const release = (col, row) => ({ type: "release", col, row, velocity: 64 });
const pressure = (col, row, value) => ({ type: "pressure", col, row, value });
const x = (col, row, value) => ({ type: "x", col, row, value });
const y = (col, row, value) => ({ type: "y", col, row, value });

test("a zone takes its type's defaults and a normalised rectangle", () => {
  const layout = layoutWith([{ type: "pad", col0: 5, col1: 2, row0: 3, row1: 1 }]);
  const z = layout.zones[0];
  assert.deepEqual([z.col0, z.col1, z.row0, z.row1], [2, 5, 1, 3]);
  assert.equal(z.note, 36);
  assert.equal(z.pressure, "poly");
  assert.equal(z.color, ufm.COLORS.orange);
  assert.equal(z.channel, 1);
  assert.match(z.id, /^z\d+$/);
});

test("zones are checked against the model's columns, the rows and each other", () => {
  const layout = layoutWith([{ type: "pad", col0: 1, col1: 4, row0: 0, row1: 1 }], 128);
  assert.throws(() => designer.addZone(layout, { type: "fader", col0: 1, col1: 17, row0: 2, row1: 2 }), /columns must be 1 to 16/);
  assert.throws(() => designer.addZone(layout, { type: "fader", col0: 0, col1: 3, row0: 2, row1: 2 }), /columns must be 1 to 16/);
  assert.throws(() => designer.addZone(layout, { type: "fader", col0: 1, col1: 3, row0: 7, row1: 8 }), /rows must be 0 to 7/);
  assert.throws(() => designer.addZone(layout, { type: "toggle", col0: 4, col1: 6, row0: 1, row1: 3 }), /overlaps pad grid from note 36/);
  assert.throws(() => designer.addZone(layout, { type: "xy", col0: 5, col1: 5, row0: 1, row1: 3 }), /at least 2 columns/);
  assert.throws(() => designer.addZone(layout, { type: "pad", col0: 5, col1: 8, row0: 0, row1: 7, note: 100, perCell: true }), /past 127/);
  assert.throws(() => designer.addZone(layout, { type: "pad", col0: 5, col1: 5, row0: 0, row1: 0, channel: 17 }), /channel/);
  assert.throws(() => designer.addZone(layout, { type: "knob", col0: 5, col1: 5, row0: 0, row1: 0 }), /type must be/);
  assert.equal(layout.zones.length, 1);
});

test("a zone can be replaced in place and removed, and found by cell", () => {
  const layout = layoutWith([{ type: "pad", col0: 1, col1: 2, row0: 0, row1: 0 }, { type: "toggle", col0: 4, col1: 4, row0: 0, row1: 0 }]);
  const id = layout.zones[0].id;
  designer.replaceZone(layout, id, { type: "fader", col0: 1, col1: 3, row0: 0, row1: 0, cc: 7 });
  assert.equal(layout.zones[0].id, id);
  assert.equal(layout.zones[0].type, "fader");
  assert.throws(() => designer.replaceZone(layout, id, { type: "fader", col0: 1, col1: 4, row0: 0, row1: 0 }), /overlaps toggle/);
  assert.equal(designer.zoneAt(layout, 3, 0).cc, 7);
  assert.equal(designer.zoneAt(layout, 3, 1), null);
  designer.removeZone(layout, id);
  assert.equal(layout.zones.length, 1);
});

test("a layout survives the JSON round trip and refuses foreign files", () => {
  const layout = layoutWith([{ type: "xy", col0: 1, col1: 4, row0: 0, row1: 3, ccX: 1, ccY: 2, channel: 3 }]);
  const text = designer.serialize(layout);
  const back = designer.parse(text);
  assert.deepEqual(back, layout);
  assert.throws(() => designer.parse("{}"), /not a LinnStrument surface layout/);
  assert.throws(() => designer.parse(JSON.stringify({ format: designer.FORMAT, model: 150, zones: [] })), /model/);
  assert.throws(() => designer.parse(JSON.stringify({ format: designer.FORMAT, model: 200, zones: [{ type: "pad", col0: 30, col1: 30, row0: 0, row1: 0 }] })), /columns/);
  // ids from a file stay unique against ids made afterwards
  const loaded = designer.parse(JSON.stringify({ format: designer.FORMAT, model: 200, zones: [{ id: "z1", type: "pad", col0: 1, col1: 1, row0: 0, row1: 0 }] }));
  designer.addZone(loaded, { type: "pad", col0: 2, col1: 2, row0: 0, row1: 0, note: 40 });
  assert.notEqual(loaded.zones[0].id, loaded.zones[1].id);
});

test("a 200 layout fitted to a 128 loses what lies past column 16", () => {
  const layout = layoutWith([{ type: "fader", col0: 1, col1: 25, row0: 0, row1: 0 }, { type: "pad", col0: 20, col1: 22, row0: 1, row1: 1 }]);
  const fitted = designer.fitToModel(layout, 128);
  assert.equal(fitted.model, 128);
  assert.equal(fitted.zones.length, 1);
  assert.equal(fitted.zones[0].col1, 16);
});

test("widening a 128 edit gives back what the 200 layout had past column 16", () => {
  const wide = layoutWith([
    { id: "f", type: "fader", col0: 1, col1: 25, row0: 0, row1: 0, cc: 7 },
    { id: "k", type: "keys", col0: 1, col1: 20, row0: 1, row1: 2 },
    { id: "m", type: "fader", col0: 10, col1: 25, row0: 3, row1: 3, cc: 8 },
    { id: "p", type: "pad", col0: 20, col1: 22, row0: 4, row1: 4 },
    { id: "d", type: "pad", col0: 14, col1: 18, row0: 5, row1: 5, note: 100 },
  ]);
  const edited = designer.fitToModel(wide, 128);
  assert.equal(edited.zones.length, 4);
  designer.replaceZone(edited, "f", Object.assign({}, edited.zones[0], { cc: 11 }));
  designer.replaceZone(edited, "m", Object.assign({}, edited.zones[2], { col0: 12 }));
  designer.removeZone(edited, "d");
  designer.addZone(edited, { id: "n", type: "pad", col0: 1, col1: 2, row0: 7, row1: 7, note: 40 });
  const back = designer.widen(edited, wide);
  const by = (id) => back.zones.find((z) => z.id === id);
  assert.equal(back.model, 200);
  assert.deepEqual([by("f").col1, by("f").cc], [25, 11]);
  assert.equal(by("k").col1, 20);
  assert.deepEqual([by("m").col0, by("m").col1], [12, 16]);
  assert.equal(by("d"), undefined);
  assert.deepEqual([by("p").col0, by("p").col1], [20, 22]);
  assert.equal(by("n").row0, 7);
  assert.equal(new Set(back.zones.map((z) => z.id)).size, back.zones.length);
  assert.throws(() => designer.widen(wide, wide), /128/);
});

test("widening keeps a zone whose regained width would clash, at the 128 width", () => {
  const wide = layoutWith([
    { id: "a", type: "keys", col0: 1, col1: 25, row0: 0, row1: 0, channel: 1, note: 48 },
    { id: "b", type: "pad", col0: 1, col1: 1, row0: 2, row1: 2, channel: 1, note: 100 },
  ]);
  const edited = designer.fitToModel(wide, 128);
  designer.replaceZone(edited, "b", Object.assign({}, edited.zones[1], { note: 70 }));
  const back = designer.widen(edited, wide);
  assert.equal(back.zones.find((z) => z.id === "a").col1, 16);
  assert.equal(back.zones.find((z) => z.id === "b").note, 70);
});

test("widening brings back a zone the cut had to drop, and renames a new zone that took a hidden id", () => {
  const wide = layoutWith([
    { id: "xy", type: "xy", col0: 16, col1: 19, row0: 0, row1: 1 },
    { id: "far", type: "pad", col0: 20, col1: 21, row0: 3, row1: 3, note: 50 },
    { id: "keep", type: "pad", col0: 1, col1: 2, row0: 3, row1: 3 },
    { id: "edge", type: "xy", col0: 16, col1: 20, row0: 6, row1: 7, channel: 2 },
  ]);
  const edited = designer.fitToModel(wide, 128);
  assert.deepEqual(edited.zones.map((z) => z.id), ["keep"]);
  designer.addZone(edited, { id: "far", type: "fader", col0: 1, col1: 4, row0: 5, row1: 5 });
  designer.addZone(edited, { type: "pad", col0: 16, col1: 16, row0: 7, row1: 7, channel: 3 });
  const back = designer.widen(edited, wide);
  const by = (id) => back.zones.find((z) => z.id === id);
  assert.deepEqual([by("xy").col0, by("xy").col1], [16, 19]);
  assert.deepEqual([by("far").type, by("far").col0], ["pad", 20]);
  const fader = back.zones.find((z) => z.type === "fader");
  assert.deepEqual([fader.col0, fader.col1, fader.row0], [1, 4, 5]);
  assert.deepEqual([by("edge").col0, by("edge").col1], [17, 20]);
  assert.equal(back.zones.length, 6);
});

test("starting the runner enters the mode, enables only the axes the rows need and paints the layout", () => {
  const layout = layoutWith([
    { type: "pad", col0: 1, col1: 2, row0: 0, row1: 0, pressure: "none" },
    { type: "fader", col0: 1, col1: 4, row0: 2, row1: 2 },
    { type: "fader", col0: 6, col1: 6, row0: 3, row1: 5 },
    { type: "xy", col0: 8, col1: 9, row0: 6, row1: 7 },
  ], 128);
  const messages = designer.createRunner(layout).start();
  assert.deepEqual(messages.slice(0, 4), ufm.enterUserFirmwareMode());
  const axes = messages.filter((m) => m[1] >= 10 && m[1] <= 12).map((m) => [(m[0] & 15), m[1], m[2]]);
  const on = axes.filter((a) => a[2] === 1).map((a) => "row " + a[0] + " cc " + a[1]);
  assert.deepEqual(on, ["row 2 cc 10", "row 3 cc 11", "row 4 cc 11", "row 5 cc 11", "row 6 cc 10", "row 6 cc 11", "row 7 cc 10", "row 7 cc 11"]);
  assert.equal(axes.length, 24);
  const paint = messages.filter((m) => m[1] === 25);
  assert.equal(paint.length, 17 * 8);
  assert.equal(paint[1][2], ufm.COLORS.orange);
  assert.equal(paint[0][2], 0);
});

test("a firmware without painted runs gets every cell lit one by one", () => {
  const layout = layoutWith([{ type: "pad", col0: 1, col1: 2, row0: 0, row1: 0 }], 128);
  const runner = designer.createRunner(layout, null, { eachCell: () => true });
  const messages = runner.start();
  assert.equal(messages.filter((m) => m[1] === 25).length, 0);
  const colors = messages.filter((m) => m[1] === 22);
  assert.equal(colors.length, 17 * 8);
  assert.deepEqual(messages.slice(messages.indexOf(colors[0]) - 2, messages.indexOf(colors[0]) + 1), ufm.setLed(0, 0, 0));
  assert.equal(colors[1][2], ufm.COLORS.orange);
  assert.deepEqual(runner.handle(press(1, 0)).toInstrument, ufm.setLed(1, 0, ufm.COLORS.white));
});

test("a run painted before the firmware was known to be old is repainted cell by cell", () => {
  const layout = layoutWith([{ type: "pad", col0: 1, col1: 2, row0: 0, row1: 0 }], 128);
  let old = false;
  const runner = designer.createRunner(layout, null, { eachCell: () => old });
  assert.equal(runner.start().filter((m) => m[1] === 25).length, 17 * 8);
  old = true;
  const repaint = runner.refresh().toInstrument;
  assert.equal(repaint.filter((m) => m[1] === 22).length, 17 * 8);
  assert.equal(runner.refresh().toInstrument.length, 0);
});

test("a pad grid plays one note per cell with pressure, and lights the held cell", () => {
  const layout = layoutWith([{ type: "pad", col0: 1, col1: 4, row0: 0, row1: 1, note: 36, perCell: true, channel: 10 }]);
  const runner = designer.createRunner(layout);
  runner.start();
  let out = runner.handle(press(3, 1, 90));
  assert.deepEqual(out.toDaw, [[0x99, 42, 90]]);
  assert.deepEqual(out.toInstrument, ufm.setLed(3, 1, ufm.COLORS.white));
  out = runner.handle(pressure(3, 1, 77));
  assert.deepEqual(out.toDaw, [[0xA9, 42, 77]]);
  assert.deepEqual(out.toInstrument, []);
  out = runner.handle(release(3, 1));
  assert.deepEqual(out.toDaw, [[0x89, 42, 0]]);
  assert.deepEqual(out.toInstrument, ufm.setLed(3, 1, ufm.COLORS.orange));
  assert.deepEqual(runner.handle(pressure(3, 1, 50)).toDaw, []);
});

test("a single big pad is one note until the last finger lifts, with channel pressure if asked", () => {
  const layout = layoutWith([{ type: "pad", col0: 1, col1: 3, row0: 0, row1: 2, note: 60, perCell: false, pressure: "channel" }]);
  const runner = designer.createRunner(layout);
  runner.start();
  assert.deepEqual(runner.handle(press(1, 0, 80)).toDaw, [[0x90, 60, 80]]);
  assert.deepEqual(runner.handle(press(2, 1, 90)).toDaw, []);
  assert.deepEqual(runner.handle(pressure(2, 1, 40)).toDaw, [[0xD0, 40]]);
  assert.deepEqual(runner.handle(release(1, 0)).toDaw, []);
  assert.equal(runner.frame()[1 * 26 + 2], ufm.COLORS.white);
  assert.deepEqual(runner.handle(release(2, 1)).toDaw, [[0x80, 60, 0]]);
  assert.equal(runner.frame()[1 * 26 + 2], ufm.COLORS.orange);
});

test("a big pad sends the strongest pressure of its cells, and only when it changes", () => {
  const layout = layoutWith([{ type: "pad", col0: 1, col1: 2, row0: 0, row1: 1, note: 60, perCell: false }]);
  const runner = designer.createRunner(layout);
  runner.start();
  runner.handle(press(1, 0, 80));
  runner.handle(press(2, 0, 90));
  assert.deepEqual(runner.handle(pressure(1, 0, 80)).toDaw, [[0xA0, 60, 80]]);
  assert.deepEqual(runner.handle(pressure(2, 0, 31)).toDaw, []);
  assert.deepEqual(runner.handle(pressure(1, 0, 82)).toDaw, [[0xA0, 60, 82]]);
  assert.deepEqual(runner.handle(pressure(2, 0, 30)).toDaw, []);
  // the strong cell lifts: the pad's pressure falls to the other cell's
  assert.deepEqual(runner.handle(release(1, 0)).toDaw, [[0xA0, 60, 30]]);
  assert.deepEqual(runner.handle(pressure(2, 0, 30)).toDaw, []);
  assert.deepEqual(runner.handle(release(2, 0)).toDaw, [[0x80, 60, 0]]);
});

test("a horizontal fader follows X across its cells and lights its level", () => {
  const layout = layoutWith([{ type: "fader", col0: 3, col1: 6, row0: 2, row1: 2, cc: 7, channel: 2 }]);
  const runner = designer.createRunner(layout);
  runner.start();
  assert.deepEqual(runner.frame().slice(2 * 26 + 3, 2 * 26 + 7), [0, 0, 0, 0]);
  // the press itself sends nothing: the first X of the touch follows at once
  let out = runner.handle(press(4, 2));
  assert.deepEqual(out.toDaw, []);
  // X in surface units, the fader spanning (3-1)*170.625 to 6*170.625
  out = runner.handle(x(4, 2, Math.round(6 * designer.X_UNIT)));
  assert.deepEqual(out.toDaw, [[0xB1, 7, 127]]);
  assert.deepEqual(runner.frame().slice(2 * 26 + 3, 2 * 26 + 7), [3, 3, 3, 3]);
  out = runner.handle(x(4, 2, Math.round(2 * designer.X_UNIT)));
  assert.deepEqual(out.toDaw, [[0xB1, 7, 0]]);
  assert.deepEqual(runner.handle(x(4, 2, Math.round(2 * designer.X_UNIT))).toDaw, []);
  // at zero the finger's cell still shows, in the finger color, until it lifts
  assert.deepEqual(runner.frame().slice(2 * 26 + 3, 2 * 26 + 7), [3, 0, 0, 0]);
  runner.handle(release(4, 2));
  assert.deepEqual(runner.frame().slice(2 * 26 + 3, 2 * 26 + 7), [0, 0, 0, 0]);
  assert.deepEqual(runner.handle(y(4, 2, 100)).toDaw, []);
});

test("a vertical fader follows the row and Y, and a touch in the top cell at full Y is 127", () => {
  const layout = layoutWith([{ type: "fader", col0: 2, col1: 2, row0: 0, row1: 3, cc: 20 }]);
  const runner = designer.createRunner(layout);
  runner.start();
  runner.handle(press(2, 3));
  assert.deepEqual(runner.handle(y(2, 3, 127)).toDaw, [[0xB0, 20, 127]]);
  assert.deepEqual(runner.handle(y(2, 1, 0)).toDaw, [[0xB0, 20, 32]]);
  const column = [0, 1, 2, 3].map((row) => runner.frame()[row * 26 + 2]);
  assert.deepEqual(column, [3, 0, 0, 0]);
  assert.deepEqual(runner.handle(x(2, 1, 300)).toDaw, []);
});

test("an xy pad sends two controllers and shows a crosshair", () => {
  const layout = layoutWith([{ type: "xy", col0: 1, col1: 4, row0: 0, row1: 3, ccX: 1, ccY: 2 }]);
  const runner = designer.createRunner(layout);
  runner.start();
  let out = runner.handle(press(4, 3));
  assert.deepEqual(out.toDaw, []);
  out = runner.handle(x(4, 3, Math.round(4 * designer.X_UNIT)));
  assert.deepEqual(out.toDaw, [[0xB0, 1, 127]]);
  out = runner.handle(y(4, 3, 127));
  assert.deepEqual(out.toDaw, [[0xB0, 2, 127]]);
  const frame = runner.frame();
  const lit = [];
  for (let row = 0; row < 4; row++) for (let col = 1; col <= 4; col++) if (frame[row * 26 + col]) lit.push(col + ":" + row + ":" + frame[row * 26 + col]);
  assert.deepEqual(lit, ["4:0:5", "4:1:5", "4:2:5", "1:3:5", "2:3:5", "3:3:5", "4:3:8"]);
});

test("toggles latch per cell and send 127 or 0 on consecutive controllers", () => {
  const layout = layoutWith([{ type: "toggle", col0: 1, col1: 4, row0: 7, row1: 7, cc: 102, perCell: true }]);
  const runner = designer.createRunner(layout);
  runner.start();
  assert.deepEqual(runner.handle(press(3, 7)).toDaw, [[0xB0, 104, 127]]);
  assert.equal(runner.frame()[7 * 26 + 3], ufm.COLORS.lime);
  assert.deepEqual(runner.handle(release(3, 7)).toDaw, []);
  assert.deepEqual(runner.handle(press(3, 7)).toDaw, [[0xB0, 104, 0]]);
  assert.equal(runner.frame()[7 * 26 + 3], ufm.COLORS.red);
});

test("a zone added or removed while the runner lives is painted and played at once", () => {
  const layout = layoutWith([{ type: "pad", col0: 1, col1: 1, row0: 0, row1: 0 }]);
  const runner = designer.createRunner(layout);
  runner.start();
  const added = designer.addZone(layout, { type: "toggle", col0: 3, col1: 3, row0: 0, row1: 0, cc: 110, perCell: false });
  assert.equal(runner.frame()[3], ufm.COLORS.red);
  assert.deepEqual(runner.handle(press(3, 0)).toDaw, [[0xB0, 110, 127]]);
  designer.removeZone(layout, added.id);
  assert.equal(runner.frame()[3], 0);
  assert.deepEqual(runner.handle(press(3, 0)).toDaw, []);
});

test("touches outside every zone and on the switch column do nothing", () => {
  const layout = layoutWith([{ type: "pad", col0: 2, col1: 2, row0: 2, row1: 2 }]);
  const runner = designer.createRunner(layout);
  runner.start();
  assert.deepEqual(runner.handle(press(5, 5)), { toDaw: [], toInstrument: [] });
  assert.deepEqual(runner.handle(press(0, 2)).toDaw, []);
  assert.deepEqual(runner.handle({ type: "slide", row: 2, fromCol: 2 }), { toDaw: [], toInstrument: [] });
});

test("stopping releases held notes and leaves the mode", () => {
  const layout = layoutWith([{ type: "pad", col0: 1, col1: 2, row0: 0, row1: 0, note: 40, perCell: true }]);
  const runner = designer.createRunner(layout);
  runner.start();
  runner.handle(press(1, 0));
  runner.handle(press(2, 0));
  const out = runner.stop();
  assert.deepEqual(out.toDaw, [[0x80, 40, 0], [0x80, 41, 0]]);
  assert.deepEqual(out.toInstrument, ufm.leaveUserFirmwareMode());
  assert.deepEqual(runner.handle(release(1, 0)).toDaw, []);
});

test("a keys zone is an isomorphic grid with scale and root colors and notes per cell", () => {
  const layout = layoutWith([{ type: "keys", col0: 1, col1: 6, row0: 0, row1: 1, note: 60, rowInterval: 5, scale: "major", channel: 2 }]);
  const z = layout.zones[0];
  assert.equal(designer.keyNote(z, 1, 0), 60);
  assert.equal(designer.keyNote(z, 3, 1), 67);
  const runner = designer.createRunner(layout);
  runner.start();
  const frame = runner.frame();
  const row0 = [1, 2, 3, 4, 5, 6].map((col) => frame[col]);
  // C D E F G A from C: the root blue, scale notes green, C# D# off
  assert.deepEqual(row0, [ufm.COLORS.blue, 0, ufm.COLORS.green, 0, ufm.COLORS.green, ufm.COLORS.green]);
  assert.deepEqual(runner.handle(press(3, 1, 70)).toDaw, [[0x91, 67, 70]]);
  assert.equal(runner.frame()[26 + 3], ufm.COLORS.white);
  assert.deepEqual(runner.handle(pressure(3, 1, 50)).toDaw, [[0xA1, 67, 50]]);
  assert.deepEqual(runner.handle(release(3, 1)).toDaw, [[0x81, 67, 0]]);
  assert.equal(runner.frame()[26 + 3], ufm.COLORS.green);
  // a grid that runs past note 127 leaves those cells dark and silent
  const high = layoutWith([{ type: "keys", col0: 1, col1: 2, row0: 0, row1: 0, note: 127, rowInterval: 5 }]);
  const r2 = designer.createRunner(high);
  r2.start();
  assert.equal(r2.frame()[2], 0);
  assert.deepEqual(r2.handle(press(2, 0)).toDaw, []);
  assert.throws(() => designer.addZone(layout, { type: "keys", col0: 8, col1: 9, row0: 0, row1: 0, scale: "dorian" }), /scale must be/);
});

test("a toggle can send notes or program changes, and a button is momentary", () => {
  const layout = layoutWith([
    { type: "toggle", col0: 1, col1: 1, row0: 0, row1: 0, message: "note", note: 60, latch: true, perCell: false },
    { type: "toggle", col0: 2, col1: 2, row0: 0, row1: 0, message: "program", program: 5, latch: true, perCell: false },
    { type: "toggle", col0: 3, col1: 4, row0: 0, row1: 0, message: "cc", cc: 102, latch: false, perCell: true },
  ]);
  const runner = designer.createRunner(layout);
  runner.start();
  assert.deepEqual(runner.handle(press(1, 0, 90)).toDaw, [[0x90, 60, 90]]);
  assert.deepEqual(runner.handle(release(1, 0)).toDaw, []);
  assert.deepEqual(runner.handle(press(1, 0, 90)).toDaw, [[0x80, 60, 0]]);
  assert.deepEqual(runner.handle(press(2, 0)).toDaw, [[0xC0, 5]]);
  runner.handle(release(2, 0));
  assert.deepEqual(runner.handle(press(2, 0)).toDaw, []);
  runner.handle(release(2, 0));
  assert.deepEqual(runner.handle(press(2, 0)).toDaw, [[0xC0, 5]]);
  assert.deepEqual(runner.handle(press(4, 0)).toDaw, [[0xB0, 103, 127]]);
  assert.equal(runner.frame()[4], ufm.COLORS.lime);
  assert.deepEqual(runner.handle(release(4, 0)).toDaw, [[0xB0, 103, 0]]);
  assert.equal(runner.frame()[4], ufm.COLORS.red);
  assert.equal(designer.label(layout.zones[2]), "Buttons CC 102 to 103");
  assert.equal(designer.label(layout.zones[0]), "Toggle note 60");
  runner.handle(release(1, 0));
  runner.handle(press(1, 0, 90));
  assert.deepEqual(runner.stop().toDaw, [[0x80, 60, 0]]);
});

test("a bipolar fader starts in the middle and fills from the middle", () => {
  const layout = layoutWith([{ type: "fader", col0: 1, col1: 8, row0: 0, row1: 0, cc: 10, bipolar: true, initial: 64 }]);
  const runner = designer.createRunner(layout);
  runner.start();
  assert.deepEqual(runner.frame().slice(1, 9), [0, 0, 0, 0, 3, 0, 0, 0]);
  runner.handle(press(8, 0));
  assert.deepEqual(runner.handle(x(8, 0, Math.round(8 * designer.X_UNIT))).toDaw, [[0xB0, 10, 127]]);
  assert.deepEqual(runner.frame().slice(1, 9), [0, 0, 0, 0, 3, 3, 3, 3]);
  assert.deepEqual(runner.handle(x(2, 0, Math.round(1 * designer.X_UNIT))).toDaw, [[0xB0, 10, 16]]);
  assert.deepEqual(runner.frame().slice(1, 9), [0, 3, 3, 3, 0, 0, 0, 0]);
});

test("a spring xy pad returns to its middle on release", () => {
  const layout = layoutWith([{ type: "xy", col0: 1, col1: 4, row0: 0, row1: 3, ccX: 1, ccY: 2, spring: true }]);
  const runner = designer.createRunner(layout);
  runner.start();
  runner.handle(press(4, 3));
  runner.handle(x(4, 3, Math.round(4 * designer.X_UNIT)));
  runner.handle(y(4, 3, 127));
  assert.deepEqual(runner.handle(release(4, 3)).toDaw, [[0xB0, 1, 64], [0xB0, 2, 64]]);
  assert.equal(runner.frame()[2 * 26 + 3], ufm.COLORS.white);
});

test("a finger crossing cells inside a fader keeps the gesture and its finger light", () => {
  const layout = layoutWith([{ type: "fader", col0: 1, col1: 4, row0: 2, row1: 2, cc: 7, activeColor: ufm.COLORS.white }]);
  const runner = designer.createRunner(layout);
  runner.start();
  runner.handle(press(1, 2));
  assert.deepEqual(runner.handle(x(1, 2, Math.round(0.9 * designer.X_UNIT))).toDaw, [[0xB0, 7, 29]]);
  // the next cell presses while the first is still down, then the first releases: no value
  // is sent for either, and the finger light stays on the fill's edge
  assert.deepEqual(runner.handle(press(2, 2)).toDaw, []);
  assert.deepEqual(runner.handle(release(1, 2)).toDaw, []);
  assert.deepEqual(runner.frame().slice(2 * 26 + 1, 2 * 26 + 5), [ufm.COLORS.white, 0, 0, 0]);
  assert.deepEqual(runner.handle(x(2, 2, Math.round(1.5 * designer.X_UNIT))).toDaw, [[0xB0, 7, 48]]);
  runner.handle(release(2, 2));
  assert.deepEqual(runner.frame().slice(2 * 26 + 1, 2 * 26 + 5), [3, 3, 0, 0]);
  // a fader held at zero still shows where the finger is
  runner.handle(press(1, 2));
  runner.handle(x(1, 2, 0));
  assert.equal(runner.frame()[2 * 26 + 1], ufm.COLORS.white);
});

test("a spring xy pad only springs back once every finger has left it", () => {
  const layout = layoutWith([{ type: "xy", col0: 1, col1: 4, row0: 0, row1: 3, ccX: 1, ccY: 2, spring: true }]);
  const runner = designer.createRunner(layout);
  runner.start();
  runner.handle(press(2, 1));
  runner.handle(x(2, 1, Math.round(2 * designer.X_UNIT) - 3));
  runner.handle(y(2, 1, 30));
  runner.handle(press(3, 1));
  assert.deepEqual(runner.handle(release(2, 1)).toDaw, []);
  assert.deepEqual(runner.handle(x(3, 1, Math.round(2.2 * designer.X_UNIT))).toDaw, [[0xB0, 1, 70]]);
  const out = runner.handle(release(3, 1)).toDaw;
  assert.deepEqual(out, [[0xB0, 1, 64], [0xB0, 2, 64]]);
});

test("two cells of a keys zone that share a note hold it until both lift", () => {
  const layout = layoutWith([{ type: "keys", col0: 1, col1: 6, row0: 0, row1: 1, note: 60, rowInterval: 5 }]);
  const runner = designer.createRunner(layout);
  runner.start();
  assert.deepEqual(runner.handle(press(6, 0, 90)).toDaw, [[0x90, 65, 90]]);
  assert.deepEqual(runner.handle(press(1, 1, 80)).toDaw, []);
  assert.deepEqual(runner.handle(release(6, 0)).toDaw, []);
  assert.equal(runner.frame()[26 + 1], ufm.COLORS.white);
  assert.deepEqual(runner.handle(release(1, 1)).toDaw, [[0x80, 65, 0]]);
  // a press repeated for a cell already held, after a lost release, is not a second note
  assert.deepEqual(runner.handle(press(1, 0)).toDaw, [[0x90, 60, 100]]);
  assert.deepEqual(runner.handle(press(1, 0)).toDaw, []);
  assert.deepEqual(runner.handle(release(1, 0)).toDaw, [[0x80, 60, 0]]);
});

test("a momentary button with two fingers switches off when the last one lifts", () => {
  const layout = layoutWith([{ type: "toggle", col0: 1, col1: 2, row0: 0, row1: 0, cc: 102, latch: false, perCell: false }]);
  const runner = designer.createRunner(layout);
  runner.start();
  assert.deepEqual(runner.handle(press(1, 0)).toDaw, [[0xB0, 102, 127]]);
  assert.deepEqual(runner.handle(press(2, 0)).toDaw, []);
  assert.deepEqual(runner.handle(release(1, 0)).toDaw, []);
  assert.deepEqual(runner.handle(release(2, 0)).toDaw, [[0xB0, 102, 0]]);
});

test("removing or stopping with notes held releases them, and a refresh keeps the rest", () => {
  const layout = layoutWith([
    { type: "pad", col0: 1, col1: 2, row0: 0, row1: 0, note: 60, perCell: true },
    { type: "toggle", col0: 4, col1: 4, row0: 0, row1: 0, cc: 102, perCell: false },
    { type: "fader", col0: 1, col1: 4, row0: 1, row1: 1, cc: 7, activeColor: ufm.COLORS.white },
  ]);
  const runner = designer.createRunner(layout);
  runner.start();
  runner.handle(press(1, 0));
  runner.handle(press(4, 0));
  runner.handle(press(1, 1));
  runner.handle(x(1, 1, Math.round(4 * designer.X_UNIT)));
  const padId = layout.zones[0].id;
  designer.removeZone(layout, padId);
  const refreshed = runner.refresh();
  assert.deepEqual(refreshed.toDaw, [[0x80, 60, 0]]);
  // row 0 no longer needs Z once the pad is gone; that is the only axis change sent
  const axes = refreshed.toInstrument.filter((m) => m[1] >= 10 && m[1] <= 12).map((m) => "row " + (m[0] & 15) + " cc " + m[1] + " " + m[2]);
  assert.deepEqual(axes, ["row 0 cc 12 0"]);
  // the toggle stays on and the fader keeps its level and its finger through the refresh
  assert.equal(runner.frame()[4], ufm.COLORS.lime);
  assert.deepEqual(runner.frame().slice(26 + 1, 26 + 5), [3, 3, 3, 8]);
  assert.deepEqual(runner.handle(press(1, 0)).toDaw, []);
  // a second refresh with nothing changed sends nothing at all
  const again = runner.refresh();
  assert.deepEqual(again.toDaw, []);
  assert.deepEqual(again.toInstrument, []);
  // a resume repaints everything and resends every axis, without entering the mode
  const resumed = runner.resume();
  assert.equal(resumed.filter((m) => m[1] === 99).length, 0);
  assert.equal(resumed.filter((m) => m[1] >= 10 && m[1] <= 12).length, 24);
  assert.equal(resumed.filter((m) => m[1] === 25).length, 26 * 8);
  // a big pad's pressure memory does not survive a stop
  const big = layoutWith([{ type: "pad", col0: 1, col1: 2, row0: 0, row1: 0, note: 60, perCell: false }]);
  const r2 = designer.createRunner(big);
  r2.start();
  r2.handle(press(1, 0));
  r2.handle(pressure(1, 0, 80));
  assert.deepEqual(r2.stop().toDaw, [[0x80, 60, 0]]);
  const painted = r2.start();
  assert.equal(painted.filter((m) => m[1] === 25).length, 26 * 8);
  r2.handle(press(2, 0));
  assert.deepEqual(r2.handle(pressure(2, 0, 20)).toDaw, [[0xA0, 60, 20]]);
  // the note's aftertouch memory goes with the note: after a note off, the same value is sent again
  r2.handle(pressure(2, 0, 80));
  r2.handle(release(2, 0));
  r2.handle(press(2, 0));
  assert.deepEqual(r2.handle(pressure(2, 0, 80)).toDaw, [[0xA0, 60, 80]]);
});

test("a zone edited while notes are held releases what no longer fits and keeps the rest", () => {
  // a channel change: both cells of a shared note go off on the old channel
  let layout = layoutWith([{ type: "keys", col0: 1, col1: 6, row0: 0, row1: 1, note: 60, rowInterval: 5 }]);
  let runner = designer.createRunner(layout);
  runner.start();
  runner.handle(press(6, 0, 90));
  runner.handle(press(1, 1, 80));
  designer.replaceZone(layout, layout.zones[0].id, Object.assign({}, layout.zones[0], { channel: 2 }));
  assert.deepEqual(runner.refresh().toDaw, [[0x80, 65, 0]]);
  assert.deepEqual(runner.handle(release(6, 0)).toDaw, []);
  assert.deepEqual(runner.handle(press(6, 0, 90)).toDaw, [[0x91, 65, 90]]);
  assert.deepEqual(runner.stop().toDaw, [[0x81, 65, 0]]);
  // a type change drops the zone's state, the note goes off and the fader starts at its own value
  layout = layoutWith([{ type: "pad", col0: 1, col1: 2, row0: 0, row1: 0, note: 60, perCell: true }]);
  runner = designer.createRunner(layout);
  runner.start();
  runner.handle(press(1, 0));
  designer.replaceZone(layout, layout.zones[0].id, { type: "fader", col0: 1, col1: 2, row0: 0, row1: 0, cc: 7, initial: 100 });
  assert.deepEqual(runner.refresh().toDaw, [[0x80, 60, 0]]);
  assert.deepEqual(runner.frame().slice(1, 3), [3, 3]);
  assert.deepEqual(runner.handle(release(1, 0)).toDaw, []);
  // a rectangle shrunk past a held cell releases it, and a note that moved cells is released too
  layout = layoutWith([{ type: "pad", col0: 1, col1: 4, row0: 0, row1: 0, note: 36, perCell: true }]);
  runner = designer.createRunner(layout);
  runner.start();
  runner.handle(press(4, 0));
  runner.handle(press(1, 0));
  designer.replaceZone(layout, layout.zones[0].id, { type: "pad", col0: 1, col1: 3, row0: 0, row1: 0, note: 37, perCell: true });
  assert.deepEqual(runner.refresh().toDaw, [[0x80, 39, 0], [0x80, 36, 0]]);
  assert.deepEqual(runner.handle(press(3, 0, 70)).toDaw, [[0x90, 39, 70]]);
  // a latched CC toggle that is on goes off when its zone is removed or its number changes
  layout = layoutWith([{ type: "toggle", col0: 1, col1: 1, row0: 0, row1: 0, cc: 102, perCell: false }]);
  runner = designer.createRunner(layout);
  runner.start();
  runner.handle(press(1, 0));
  designer.replaceZone(layout, layout.zones[0].id, Object.assign({}, layout.zones[0], { cc: 110 }));
  assert.deepEqual(runner.refresh().toDaw, [[0xB0, 102, 0]]);
  assert.equal(runner.frame()[1], ufm.COLORS.red);
  runner.handle(release(1, 0));
  runner.handle(press(1, 0));
  assert.deepEqual(runner.stop().toDaw, [[0xB0, 110, 0]]);
});

test("switches follow a change of their latch, message, channel or size while on", () => {
  // latch to momentary while on: it switches off, and the next press is a fresh press
  let layout = layoutWith([{ type: "toggle", col0: 1, col1: 1, row0: 0, row1: 0, message: "note", note: 60, latch: true, perCell: false }]);
  let runner = designer.createRunner(layout);
  runner.start();
  runner.handle(press(1, 0, 90));
  runner.handle(release(1, 0));
  designer.replaceZone(layout, layout.zones[0].id, Object.assign({}, layout.zones[0], { latch: false }));
  assert.deepEqual(runner.refresh().toDaw, [[0x80, 60, 0]]);
  assert.deepEqual(runner.handle(press(1, 0, 90)).toDaw, [[0x90, 60, 90]]);
  assert.deepEqual(runner.handle(release(1, 0)).toDaw, [[0x80, 60, 0]]);
  // momentary to latch while held: it stays on, the finger lifts quietly, the next press switches
  // it off
  runner.handle(press(1, 0, 90));
  designer.replaceZone(layout, layout.zones[0].id, Object.assign({}, layout.zones[0], { latch: true }));
  assert.deepEqual(runner.refresh().toDaw, []);
  assert.deepEqual(runner.handle(release(1, 0)).toDaw, []);
  assert.deepEqual(runner.handle(press(1, 0, 90)).toDaw, [[0x80, 60, 0]]);
  runner.handle(release(1, 0));
  // program to cc while on: nothing is on any more, the next press sends 127
  layout = layoutWith([{ type: "toggle", col0: 1, col1: 1, row0: 0, row1: 0, message: "program", program: 5, perCell: false }]);
  runner = designer.createRunner(layout);
  runner.start();
  assert.deepEqual(runner.handle(press(1, 0)).toDaw, [[0xC0, 5]]);
  designer.replaceZone(layout, layout.zones[0].id, Object.assign({}, layout.zones[0], { message: "cc", cc: 20 }));
  assert.deepEqual(runner.refresh().toDaw, []);
  assert.equal(runner.frame()[1], ufm.COLORS.red);
  runner.handle(release(1, 0));
  assert.deepEqual(runner.handle(press(1, 0)).toDaw, [[0xB0, 20, 127]]);
  // a cc switch moved to another channel goes off on the old one; a row shrunk past an on switch
  // too
  layout = layoutWith([{ type: "toggle", col0: 1, col1: 4, row0: 0, row1: 0, cc: 102, perCell: true }]);
  runner = designer.createRunner(layout);
  runner.start();
  runner.handle(press(4, 0));
  runner.handle(press(1, 0));
  designer.replaceZone(layout, layout.zones[0].id, Object.assign({}, layout.zones[0], { col1: 3, channel: 2 }));
  assert.deepEqual(runner.refresh().toDaw, [[0xB0, 102, 0], [0xB0, 105, 0]]);
  runner.handle(release(4, 0));
  runner.handle(release(1, 0));
  assert.deepEqual(runner.handle(press(1, 0)).toDaw, [[0xB1, 102, 127]]);
  // a momentary button moved to another channel while held: off on the old channel, on again on
  // the new one
  layout = layoutWith([{ type: "toggle", col0: 1, col1: 2, row0: 0, row1: 0, message: "note", note: 60, latch: false, perCell: false }]);
  runner = designer.createRunner(layout);
  runner.start();
  runner.handle(press(1, 0, 90));
  designer.replaceZone(layout, layout.zones[0].id, Object.assign({}, layout.zones[0], { channel: 3 }));
  assert.deepEqual(runner.refresh().toDaw, [[0x80, 60, 0], [0x92, 60, 100]]);
  assert.deepEqual(runner.handle(press(2, 0, 90)).toDaw, []);
  assert.deepEqual(runner.handle(release(1, 0)).toDaw, []);
  assert.deepEqual(runner.handle(release(2, 0)).toDaw, [[0x82, 60, 0]]);
});

test("a button whose cell is re-indexed by an edit lets go, and a latching switch flips once per crossing", () => {
  let layout = layoutWith([{ type: "toggle", col0: 1, col1: 2, row0: 0, row1: 0, cc: 102, latch: false, perCell: false }]);
  let runner = designer.createRunner(layout);
  runner.start();
  assert.deepEqual(runner.handle(press(2, 0)).toDaw, [[0xB0, 102, 127]]);
  designer.replaceZone(layout, layout.zones[0].id, Object.assign({}, layout.zones[0], { perCell: true }));
  // 102 has no finger and goes off; cell 2 is now switch 103, which the finger holds and switches
  // on
  assert.deepEqual(runner.refresh().toDaw, [[0xB0, 102, 0], [0xB0, 103, 127]]);
  assert.deepEqual(runner.handle(release(2, 0)).toDaw, [[0xB0, 103, 0]]);
  assert.deepEqual(runner.handle(press(2, 0)).toDaw, [[0xB0, 103, 127]]);
  assert.deepEqual(runner.stop().toDaw, [[0xB0, 103, 0]]);
  // a finger crossing into the next cell of a latching switch does not flip it back
  layout = layoutWith([{ type: "toggle", col0: 1, col1: 2, row0: 0, row1: 1, cc: 102, latch: true, perCell: false }]);
  runner = designer.createRunner(layout);
  runner.start();
  assert.deepEqual(runner.handle(press(1, 0)).toDaw, [[0xB0, 102, 127]]);
  assert.deepEqual(runner.handle(press(2, 0)).toDaw, []);
  assert.deepEqual(runner.handle(release(1, 0)).toDaw, []);
  assert.deepEqual(runner.handle(release(2, 0)).toDaw, []);
  assert.deepEqual(runner.handle(press(1, 0)).toDaw, [[0xB0, 102, 0]]);
});

test("a layout refuses two zones that send one note on one channel", () => {
  const layout = layoutWith([{ type: "pad", col0: 1, col1: 1, row0: 0, row1: 0, note: 60, perCell: true }]);
  assert.throws(() => designer.addZone(layout, { type: "toggle", col0: 3, col1: 3, row0: 0, row1: 0, message: "note", note: 60, perCell: false }), /sends notes another zone sends on channel 1/);
  assert.throws(() => designer.addZone(layout, { type: "keys", col0: 3, col1: 4, row0: 0, row1: 0, note: 59 }), /sends notes/);
  designer.addZone(layout, { type: "pad", col0: 1, col1: 4, row0: 1, row1: 1, note: 36, perCell: true });
  assert.throws(() => designer.addZone(layout, { type: "pad", col0: 6, col1: 6, row0: 1, row1: 1, note: 39, perCell: false }), /sends notes/);
  // another channel, a CC switch or a fader on the same cells' numbers are fine
  designer.addZone(layout, { type: "pad", col0: 6, col1: 6, row0: 1, row1: 1, note: 39, perCell: false, channel: 2 });
  designer.addZone(layout, { type: "toggle", col0: 8, col1: 8, row0: 1, row1: 1, cc: 60, perCell: false });
  assert.equal(layout.zones.length, 4);
});

test("a finger the run never saw pressed still counts, and a moved momentary finger switches its new index on", () => {
  // a position from a finger that was down before Run: no spring on an unrelated refresh
  let layout = layoutWith([{ type: "xy", col0: 1, col1: 2, row0: 0, row1: 1, ccX: 20, ccY: 21, spring: true, channel: 3 }]);
  let runner = designer.createRunner(layout);
  runner.start();
  assert.deepEqual(runner.handle(y(1, 1, 78)).toDaw, [[0xB2, 21, 103]]);
  assert.deepEqual(runner.refresh().toDaw, []);
  assert.deepEqual(runner.handle(release(1, 1)).toDaw, [[0xB2, 21, 64]]);
  // a momentary button held on cell 2, split per cell: switch 103 comes on under the finger
  layout = layoutWith([{ type: "toggle", col0: 1, col1: 2, row0: 0, row1: 0, cc: 102, latch: false, perCell: false }]);
  runner = designer.createRunner(layout);
  runner.start();
  runner.handle(press(2, 0));
  designer.replaceZone(layout, layout.zones[0].id, Object.assign({}, layout.zones[0], { perCell: true }));
  assert.deepEqual(runner.refresh().toDaw, [[0xB0, 102, 0], [0xB0, 103, 127]]);
  assert.deepEqual(runner.handle(release(2, 0)).toDaw, [[0xB0, 103, 0]]);
  // a press on the control column with a row that is not a row is ignored
  assert.deepEqual(runner.handle({ type: "press", col: 0, row: 9, velocity: 1 }), { toDaw: [], toInstrument: [] });
  assert.deepEqual(runner.handle({ type: "press", col: 0, row: 3, velocity: 1 }).control, 3);
});

test("a spring pad shrunk past its finger springs back, and a silenced pad forgets its pressure", () => {
  let layout = layoutWith([{ type: "xy", col0: 1, col1: 4, row0: 0, row1: 3, ccX: 20, ccY: 21, spring: true }]);
  let runner = designer.createRunner(layout);
  runner.start();
  runner.handle(press(4, 3));
  assert.deepEqual(runner.handle(x(4, 3, 600)).toDaw, [[0xB0, 20, 112]]);
  assert.deepEqual(runner.handle(y(4, 3, 120)).toDaw, [[0xB0, 21, 125]]);
  designer.replaceZone(layout, layout.zones[0].id, Object.assign({}, layout.zones[0], { col1: 3, row1: 2 }));
  assert.deepEqual(runner.refresh().toDaw, [[0xB0, 20, 64], [0xB0, 21, 64]]);
  assert.deepEqual(runner.handle(release(4, 3)).toDaw, []);
  assert.equal(runner.frame()[26 + 2], ufm.COLORS.white);
  layout = layoutWith([{ type: "pad", col0: 1, col1: 1, row0: 0, row1: 0, note: 60, perCell: true, pressure: "none" }]);
  runner = designer.createRunner(layout);
  runner.start();
  runner.handle(press(1, 0));
  assert.deepEqual(runner.handle(pressure(1, 0, 40)).toDaw, []);
  designer.replaceZone(layout, layout.zones[0].id, Object.assign({}, layout.zones[0], { pressure: "poly" }));
  assert.deepEqual(runner.refresh().toDaw, []);
  assert.deepEqual(runner.handle(pressure(1, 0, 40)).toDaw, [[0xA0, 60, 40]]);
  // events that do not name a whole cell are ignored
  assert.deepEqual(runner.handle({ type: "press", col: 2.5, row: 0, velocity: 1 }), { toDaw: [], toInstrument: [] });
});

test("a refresh resends the aftertouch of a note another cell still holds, and zeroes a changed kind", () => {
  let layout = layoutWith([{ type: "keys", col0: 1, col1: 6, row0: 0, row1: 1, note: 60, rowInterval: 5 }]);
  let runner = designer.createRunner(layout);
  runner.start();
  runner.handle(press(6, 0));
  runner.handle(press(1, 1));
  runner.handle(pressure(1, 1, 90));
  // a new row interval moves (1,1) to another note, and (6,0) still holds 65 at pressure 0
  designer.replaceZone(layout, layout.zones[0].id, Object.assign({}, layout.zones[0], { rowInterval: 7 }));
  assert.deepEqual(runner.refresh().toDaw, [[0xA0, 65, 0]]);
  // poly to channel aftertouch while held: the poly value is zeroed, the next value goes out as
  // channel pressure
  layout = layoutWith([{ type: "pad", col0: 1, col1: 1, row0: 0, row1: 0, note: 60, perCell: true }]);
  runner = designer.createRunner(layout);
  runner.start();
  runner.handle(press(1, 0));
  runner.handle(pressure(1, 0, 80));
  designer.replaceZone(layout, layout.zones[0].id, Object.assign({}, layout.zones[0], { pressure: "channel" }));
  assert.deepEqual(runner.refresh().toDaw, [[0xA0, 60, 0], [0xD0, 80]]);
  assert.deepEqual(runner.handle(pressure(1, 0, 70)).toDaw, [[0xD0, 70]]);
  // a fader shrunk past its finger: the finger light goes, the zone's old cells are cleared
  layout = layoutWith([{ type: "fader", col0: 1, col1: 4, row0: 0, row1: 0, cc: 7, activeColor: ufm.COLORS.white }]);
  runner = designer.createRunner(layout);
  runner.start();
  runner.handle(press(4, 0));
  runner.handle(x(4, 0, Math.round(4 * designer.X_UNIT)));
  designer.replaceZone(layout, layout.zones[0].id, Object.assign({}, layout.zones[0], { col1: 3 }));
  const out = runner.refresh();
  assert.deepEqual(runner.frame().slice(1, 5), [3, 3, 3, 0]);
  assert.deepEqual(out.toInstrument.filter((m) => m[1] === 22).map((m) => m[2]), [0]);
});

test("control lights survive a refresh and a restart", () => {
  const layout = layoutWith([{ type: "pad", col0: 1, col1: 1, row0: 0, row1: 0 }]);
  const runner = designer.createRunner(layout);
  runner.start();
  runner.setControlLights([0, 5, 8]);
  designer.addZone(layout, { type: "pad", col0: 2, col1: 2, row0: 0, row1: 0, note: 40 });
  const out = runner.refresh();
  assert.equal(out.toInstrument.filter((m) => m[1] === 20 && m[2] === 0).length, 0);
  assert.equal(runner.frame()[26], ufm.COLORS.blue);
  const again = designer.createRunner(layout, [0, 5, 8]);
  const paint = again.start().filter((m) => m[1] === 25);
  assert.equal(paint[26][2], ufm.COLORS.blue);
  assert.equal(paint[52][2], ufm.COLORS.white);
});

test("two keys on one note share one aftertouch, the strongest", () => {
  const layout = layoutWith([{ type: "keys", col0: 1, col1: 6, row0: 0, row1: 1, note: 60, rowInterval: 5 }]);
  const runner = designer.createRunner(layout);
  runner.start();
  runner.handle(press(6, 0));
  runner.handle(press(1, 1));
  assert.deepEqual(runner.handle(pressure(6, 0, 100)).toDaw, [[0xA0, 65, 100]]);
  assert.deepEqual(runner.handle(pressure(1, 1, 10)).toDaw, []);
  assert.deepEqual(runner.handle(release(6, 0)).toDaw, [[0xA0, 65, 10]]);
  assert.deepEqual(runner.handle(release(1, 1)).toDaw, [[0x80, 65, 0]]);
  // channel aftertouch and no aftertouch
  const chan = layoutWith([{ type: "keys", col0: 1, col1: 2, row0: 0, row1: 0, note: 60, pressure: "channel", channel: 3 },
                           { type: "pad", col0: 4, col1: 4, row0: 0, row1: 0, note: 40, pressure: "none" }]);
  const r2 = designer.createRunner(chan);
  r2.start();
  r2.handle(press(1, 0));
  assert.deepEqual(r2.handle(pressure(1, 0, 33)).toDaw, [[0xD2, 33]]);
  r2.handle(press(4, 0));
  assert.deepEqual(r2.handle(pressure(4, 0, 33)).toDaw, []);
});

test("buttons count fingers per cell, so a repeated press does not stick", () => {
  const layout = layoutWith([{ type: "toggle", col0: 1, col1: 2, row0: 0, row1: 0, message: "note", note: 60, latch: false, perCell: false }]);
  const runner = designer.createRunner(layout);
  runner.start();
  assert.deepEqual(runner.handle(press(1, 0, 90)).toDaw, [[0x90, 60, 90]]);
  assert.deepEqual(runner.handle(press(1, 0, 90)).toDaw, []);
  assert.deepEqual(runner.handle(release(1, 0)).toDaw, [[0x80, 60, 0]]);
  assert.equal(runner.frame()[1], ufm.COLORS.red);
  assert.deepEqual(designer.learnMessages(layout.zones[0]), [[0x90, 60, 100], [0x80, 60, 0]]);
  assert.deepEqual(designer.learnMessages(designer.makeZone({ type: "keys", col0: 1, col1: 2, row0: 0, row1: 0, note: 48 })), [[0x90, 48, 100], [0x80, 48, 0]]);
});

test("flags in a file must be real booleans and zones real objects", () => {
  const file = (zone) => JSON.stringify({ format: designer.FORMAT, model: 200, zones: [zone] });
  assert.throws(() => designer.parse(file({ type: "pad", col0: 1, col1: 1, row0: 0, row1: 0, perCell: "false" })), /perCell must be true or false/);
  assert.throws(() => designer.parse(file({ type: "fader", col0: 1, col1: 3, row0: 0, row1: 0, bipolar: "no" })), /bipolar must be/);
  assert.throws(() => designer.parse(file(null)), /must be an object/);
  assert.throws(() => designer.parse(JSON.stringify({ format: designer.FORMAT, model: 200, name: { a: 1 }, zones: [] })), /name must be text/);
});

test("fader and xy lights at the ends: finger edge at 127, odd bipolar counts, the crosshair corner", () => {
  const layout = layoutWith([
    { type: "fader", col0: 1, col1: 4, row0: 0, row1: 0, cc: 7, activeColor: ufm.COLORS.white },
    { type: "fader", col0: 1, col1: 7, row0: 1, row1: 1, cc: 8, bipolar: true, initial: 64, activeColor: ufm.COLORS.white },
    { type: "xy", col0: 1, col1: 4, row0: 2, row1: 5, ccX: 1, ccY: 2 },
  ]);
  const runner = designer.createRunner(layout);
  runner.start();
  runner.handle(press(4, 0));
  runner.handle(x(4, 0, Math.round(4 * designer.X_UNIT)));
  assert.deepEqual(runner.frame().slice(1, 5), [3, 3, 3, 8]);
  assert.deepEqual(runner.frame().slice(26 + 1, 26 + 8), [0, 0, 0, 3, 0, 0, 0]);
  runner.handle(press(7, 1));
  runner.handle(x(7, 1, Math.round(7 * designer.X_UNIT)));
  assert.deepEqual(runner.frame().slice(26 + 1, 26 + 8), [0, 0, 0, 3, 3, 3, 8]);
  runner.handle(x(1, 1, 0));
  assert.deepEqual(runner.frame().slice(26 + 1, 26 + 8), [8, 3, 3, 0, 0, 0, 0]);
  runner.handle(press(1, 2));
  runner.handle(x(1, 2, 0));
  runner.handle(y(1, 2, 0));
  const f = runner.frame();
  assert.equal(f[2 * 26 + 1], ufm.COLORS.white);
  assert.equal(f[2 * 26 + 4], ufm.COLORS.blue);
  assert.equal(f[5 * 26 + 1], ufm.COLORS.blue);
  assert.equal(f[5 * 26 + 4], 0);
});

test("ids must be unique strings and a fit drops a zone the edge breaks", () => {
  const two = { format: designer.FORMAT, model: 200, zones: [
    { id: "a", type: "pad", col0: 1, col1: 1, row0: 0, row1: 0 }, { id: "a", type: "pad", col0: 3, col1: 3, row0: 0, row1: 0 }] };
  assert.throws(() => designer.parse(JSON.stringify(two)), /already used/);
  assert.throws(() => designer.parse(JSON.stringify({ format: designer.FORMAT, model: 200, zones: [{ id: 7, type: "pad", col0: 1, col1: 1, row0: 0, row1: 0 }] })), /string/);
  const layout = layoutWith([{ type: "xy", col0: 16, col1: 18, row0: 0, row1: 3 }, { type: "pad", col0: 1, col1: 1, row0: 0, row1: 0 }]);
  const fitted = designer.fitToModel(layout, 128);
  assert.equal(fitted.zones.length, 1);
  assert.equal(fitted.zones[0].type, "pad");
});

test("light messages address the right cell on a 128", () => {
  const layout = layoutWith([{ type: "pad", col0: 16, col1: 16, row0: 7, row1: 7, perCell: true }], 128);
  const runner = designer.createRunner(layout);
  runner.start();
  assert.deepEqual(runner.handle(press(16, 7)).toInstrument, ufm.setLed(16, 7, ufm.COLORS.white));
});

test("learn messages identify a zone to a DAW without a touch", () => {
  assert.deepEqual(designer.learnMessages(designer.makeZone({ type: "fader", col0: 1, col1: 2, row0: 0, row1: 0, cc: 7, channel: 3 })), [[0xB2, 7, 64]]);
  assert.deepEqual(designer.learnMessages(designer.makeZone({ type: "xy", col0: 1, col1: 2, row0: 0, row1: 1, ccX: 1, ccY: 2 })), [[0xB0, 1, 64], [0xB0, 2, 64]]);
  assert.deepEqual(designer.learnMessages(designer.makeZone({ type: "pad", col0: 1, col1: 2, row0: 0, row1: 1, note: 36 })), [[0x90, 36, 100], [0x80, 36, 0]]);
  assert.deepEqual(designer.learnMessages(designer.makeZone({ type: "toggle", col0: 1, col1: 1, row0: 0, row1: 0, message: "program", program: 9 })), [[0xC0, 9]]);
  assert.deepEqual(designer.learnMessages(designer.makeZone({ type: "toggle", col0: 1, col1: 1, row0: 0, row1: 0, cc: 102 })), [[0xB0, 102, 127], [0xB0, 102, 0]]);
});

test("the control column lights what the page asks and reports its switches", () => {
  const layout = layoutWith([{ type: "pad", col0: 1, col1: 1, row0: 0, row1: 0 }]);
  const runner = designer.createRunner(layout);
  runner.start();
  const messages = runner.setControlLights([0, ufm.COLORS.blue, ufm.COLORS.white]);
  assert.deepEqual(messages, ufm.setLed(0, 1, ufm.COLORS.blue).concat(ufm.setLed(0, 2, ufm.COLORS.white)));
  assert.equal(runner.frame()[26 + 0], ufm.COLORS.blue);
  assert.equal(runner.frame()[3 * 26], 0);
  assert.deepEqual(runner.handle(press(0, 2)), { toDaw: [], toInstrument: [], control: 2 });
  assert.deepEqual(runner.handle(release(0, 2)), { toDaw: [], toInstrument: [] });
});

test("the shipped presets are valid for both models and use every zone type", () => {
  [200, 128].forEach((model) => {
    const list = designer.presets(model);
    assert.equal(list.length, 9);
    list.forEach((layout) => {
      assert.equal(layout.model, model);
      assert.deepEqual(designer.parse(designer.serialize(layout)), layout);
    });
    const types = new Set(list.flatMap((l) => l.zones.map((z) => z.type)));
    assert.deepEqual([...types].sort(), ["fader", "keys", "pad", "toggle", "xy"]);
  });
  assert.equal(designer.presets(200)[0].zones.filter((z) => z.type === "xy").length, 1);
  assert.equal(designer.presets(128)[0].zones.filter((z) => z.type === "xy").length, 0);
  const names = designer.presets(128).map((l) => l.name);
  assert.deepEqual(names, ["Drums, faders and switches", "XY pads", "Two decks", "Keys and faders", "Mixer", "Big pads", "Clip grid", "Eight macros", "Pads and keys"]);
  // the clip grid covers every note once on both models, the mixer fills the columns it has
  [200, 128].forEach((model) => {
    const clips = designer.presets(model)[6].zones[0];
    assert.equal(clips.note + 16 * 8 - 1, 127);
    const mixer = designer.presets(model)[4];
    assert.equal(mixer.zones.filter((z) => z.type === "fader" && z.cc === 7).length, model === 200 ? 13 : 8);
    assert.equal(designer.presets(model)[5].zones.filter((z) => z.type === "pad").length, model === 200 ? 48 : 32);
  });
});

test("label, place and ordered give the list its rows", () => {
  const layout = layoutWith([
    { type: "fader", col0: 9, col1: 9, row0: 0, row1: 7, cc: 20 },
    { type: "pad", col0: 1, col1: 4, row0: 0, row1: 1, note: 36, perCell: true },
    { type: "toggle", col0: 1, col1: 4, row0: 7, row1: 7, cc: 102, perCell: true },
    { type: "pad", col0: 6, col1: 6, row0: 7, row1: 7, note: 60, perCell: true },
    { type: "xy", col0: 10, col1: 12, row0: 0, row1: 2, ccX: 1, ccY: 2 },
  ]);
  assert.deepEqual(designer.ordered(layout).map(designer.label),
    ["Toggles CC 102 to 105", "Pad 60", "Fader CC 20", "XY pad CC 1/2", "Pad grid 36 to 43"]);
  assert.equal(designer.place(layout.zones[0]), "column 9, rows 0 to 7");
  assert.equal(designer.place(layout.zones[1]), "columns 1 to 4, rows 0 to 1");
  assert.equal(designer.place(layout.zones[3]), "column 6, row 7");
});

test("describe names a zone the way the page lists it", () => {
  assert.equal(designer.describe(designer.makeZone({ type: "pad", col0: 1, col1: 4, row0: 0, row1: 1, note: 36, perCell: true })), "pad grid from note 36 columns 1 to 4, rows 0 to 1");
  assert.equal(designer.describe(designer.makeZone({ type: "toggle", col0: 3, col1: 3, row0: 7, row1: 7, cc: 64, perCell: false })), "toggle CC 64 at column 3 row 7");
});
