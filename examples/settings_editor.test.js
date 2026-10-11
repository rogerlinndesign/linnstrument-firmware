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
// Run with: node --test examples/settings_editor.test.js
const test = require("node:test");
const assert = require("node:assert/strict");
const editor = require("./settings_editor.js");
const table = require("./nrpn_parameters.js");

const by = {};
table.parameters.forEach((p) => { by[p.number] = p; });

test("the generated table holds every NRPN of midi.md but the query itself", () => {
  assert.equal(table.parameters.length, 210);
  assert.equal(by[299], undefined);
  assert.equal(table.parameters.filter((p) => p.group === "Split Left").length, 67);
  assert.equal(table.parameters.filter((p) => p.group === "Split Right").length, 67);
  assert.equal(table.tables.color.length, 12);
  assert.equal(table.tables.switchAssignment.length, 18);
  assert.equal(table.tables.touchAnimation.length, 15);
});

test("enumerations, toggles, numbers, actions and read-only parameters are told apart", () => {
  assert.deepEqual(by[0].options.map((o) => o.label), ["One Channel", "Channel Per Note", "Channel Per Row"]);
  assert.equal(by[0].kind, "enum");
  assert.equal(by[2].kind, "toggle");
  assert.equal(by[2].name, "MIDI Per Note Channel 1");
  assert.equal(by[19].kind, "number");
  assert.deepEqual([by[19].min, by[19].max], [1, 96]);
  assert.equal(by[62].kind, "action");
  assert.equal(by[272].kind, "readonly");
  assert.equal(by[272].name, "Model");
  assert.equal(by[243].kind, "choice-action");
  assert.equal(by[243].options.length, 6);
});

test("ranges inside an enumeration expand, sparse ones keep their values, wide ones stay numbers", () => {
  assert.deepEqual(by[37].options.map((o) => o.label).slice(0, 8), ["-7", "-6", "-5", "-4", "-3", "-2", "-1", "0"]);
  assert.equal(by[37].options[14].label, "+7");
  assert.deepEqual(by[227].options.map((o) => o.value), [0, 3, 4, 5, 6, 7, 12, 13, 127]);
  assert.equal(by[40].kind, "number");
  assert.match(by[40].note, /Channel Aftertouch/);
});

test("the value tables feed the color, animation and switch parameters within their ranges", () => {
  assert.deepEqual(by[30].options.map((o) => o.value), [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11]);
  assert.equal(by[32].options.length, 12);
  assert.equal(by[32].options[0].label, "as set in Note Lights settings");
  assert.equal(by[61].options.length, 15);
  assert.equal(by[228].options[17].label, "Sequencer Mute");
});

test("a query is NRPN 299 with the parameter as its value, a write adds the null RPN", () => {
  assert.deepEqual(editor.queryMessages([245]), [[0xB0, 99, 2], [0xB0, 98, 43], [0xB0, 6, 1], [0xB0, 38, 117]]);
  assert.equal(editor.queryMessages([0, 1, 2]).length, 12);
  assert.deepEqual(editor.setMessages(202, 12), [
    [0xB0, 99, 1], [0xB0, 98, 74], [0xB0, 6, 0], [0xB0, 38, 12], [0xB0, 101, 127], [0xB0, 100, 127],
  ]);
  assert.throws(() => editor.setMessages(0, -1));
  assert.throws(() => editor.setMessages(20000, 0));
});

test("every parameter but the actions is queried", () => {
  const numbers = editor.queryable().map((p) => p.number);
  assert.equal(numbers.length, 201);
  assert.ok(!numbers.includes(62) && !numbers.includes(271));
  assert.ok(numbers.includes(243) && numbers.includes(272));
});

test("the decoder's named replies and plain NRPN replies both become a number and a value", () => {
  assert.deepEqual(editor.replyValue({ type: "nrpn", number: 202, value: 12, channel: 1 }), { number: 202, value: 12 });
  assert.deepEqual(editor.replyValue({ type: "mode", active: true, channel: 9 }), { number: 245, value: 1 });
  assert.deepEqual(editor.replyValue({ type: "model", model: 128, columns: 17, channel: 1 }), { number: 272, value: 128 });
  assert.deepEqual(editor.replyValue({ type: "version", version: 234, channel: 1 }), { number: 273, value: 234 });
  assert.deepEqual(editor.replyValue({ type: "calibrated", calibrated: false, channel: 1 }), { number: 275, value: 0 });
  assert.equal(editor.formatValue(by[275], 1), "Yes");
  assert.equal(by[275].kind, "readonly");
  assert.equal(editor.replyValue({ type: "press", col: 1, row: 0, velocity: 1 }), null);
});

test("values clamp to the parameter's range and read back as their labels", () => {
  assert.equal(editor.clampValue(by[19], 200), 96);
  assert.equal(editor.clampValue(by[19], 0), 1);
  assert.equal(editor.clampValue(by[19], NaN), 1);
  assert.equal(editor.formatValue(by[0], 2), "Channel Per Row");
  assert.equal(editor.formatValue(by[19], 12), "12");
  assert.equal(editor.formatValue(by[243], 3), "Preset 4");
});

test("the form pairs the splits, folds runs of toggles and keeps the actions apart", () => {
  const sections = editor.sections();
  assert.deepEqual(sections.map((s) => s.id), ["global", "splits", "device", "actions"]);
  const splits = sections[1].rows;
  assert.equal(splits[0].name, "MIDI Mode");
  assert.equal(splits[0].left.parameter.number, 0);
  assert.equal(splits[0].right.parameter.number, 100);
  const channels = splits.find((r) => r.name === "MIDI Per Note Channel");
  assert.equal(channels.kind, "toggles");
  assert.equal(channels.left.items.length, 16);
  assert.equal(channels.right.items[15].parameter.number, 117);
  const lights = sections[0].rows.find((r) => r.name === "Main Note Light");
  assert.deepEqual(lights.items.map((i) => i.label).slice(0, 3), ["C", "C#", "D"]);
  assert.ok(sections[3].rows.some((r) => r.parameter.number === 243));
  assert.ok(!sections[0].rows.some((r) => r.parameter && r.parameter.number === 243));
});

test("the search matches names, numbers and descriptions", () => {
  const sections = editor.sections();
  const row = sections[0].rows.find((r) => r.name === "Split Point Column");
  assert.ok(editor.matches(row, "split point"));
  assert.ok(editor.matches(row, "202"));
  assert.ok(!editor.matches(row, "velocity"));
  assert.ok(editor.matches(row, ""));
});
