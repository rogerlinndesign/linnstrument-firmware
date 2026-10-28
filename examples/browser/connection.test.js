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
// Run with: node --test examples/browser/connection.test.js
const test = require("node:test");
const assert = require("node:assert/strict");
const connection = require("./connection.js");

test("the header label follows the connection state", () => {
  assert.deepEqual(connection.describe({ noMidi: true }), { text: "No Web MIDI", state: "" });
  assert.deepEqual(connection.describe({ refused: true }), { text: "MIDI access refused", state: "" });
  assert.deepEqual(connection.describe({ input: null, output: "LinnStrument MIDI" }), { text: "Choose MIDI ports", state: "" });
  assert.deepEqual(connection.describe({ input: "LinnStrument MIDI", output: "LinnStrument MIDI" }), { text: "LinnStrument MIDI", state: "ready" });
  assert.deepEqual(connection.describe({ input: "LinnStrument MIDI", output: "LinnStrument MIDI", model: 128 }), { text: "LinnStrument 128", state: "on" });
  assert.deepEqual(connection.describe({ input: "LinnStrument MIDI", output: "LinnStrument MIDI", firmwareOld: true }), { text: "LinnStrument, 2.3.4 or earlier", state: "on" });
});

test("a port list keeps the chosen port while it exists, else takes the first matching name, else none", () => {
  const ports = [{ id: "a", name: "Network Mac Session" }, { id: "b", name: "LinnStrument MIDI" }, { id: "c", name: "IAC Driver Bus 1" }];
  assert.equal(connection.pickPort(ports, "", /LinnStrument/), "b");
  assert.equal(connection.pickPort(ports, "a", /LinnStrument/), "a");
  assert.equal(connection.pickPort(ports, "gone", /LinnStrument/), "b");
  assert.equal(connection.pickPort(ports, "", /IAC|loopMIDI/), "c");
  assert.equal(connection.pickPort([{ id: "b", name: "LinnStrument MIDI" }], "", /IAC|loopMIDI/), "");
  assert.equal(connection.pickPort([], "", /LinnStrument/), "");
});

test("the firmware line reports the version, or that the firmware is too old for the pages", () => {
  assert.equal(connection.firmwareText({ version: null }), "");
  assert.equal(connection.firmwareText({ version: "2.4.0 build 75" }), "Firmware 2.4.0 build 75");
  assert.equal(connection.versionText(240), "2.4.0");
  assert.equal(connection.versionText(240, 75), "2.4.0 build 75");
  assert.equal(connection.versionText(234, 74), "2.3.4 build 74");
  assert.match(connection.firmwareText({ version: null, firmwareOld: true }), /2\.3\.4, the current release, or earlier/);
});

test("the status line in the panel says what to do", () => {
  assert.equal(connection.statusText({ noMidi: true }), "No Web MIDI here, use Chrome, Edge or Firefox");
  assert.equal(connection.statusText({ input: null }), "Choose the MIDI in and out");
  assert.equal(connection.statusText({ input: "Virtual LinnStrument", output: "Virtual LinnStrument" }), "Connected to Virtual LinnStrument");
  assert.equal(connection.statusText({ input: "a", output: "b", model: 200 }), "Connected to LinnStrument 200");
  assert.equal(connection.statusText({ input: "LinnStrument MIDI", output: "b", firmwareOld: true }), "Connected to LinnStrument MIDI");
});

test("the picked model is remembered, and a missing or refused storage remembers nothing", () => {
  const store = {};
  const storage = { getItem: (k) => (k in store ? store[k] : null), setItem: (k, v) => { store[k] = v; } };
  assert.equal(connection.storedModel(storage), null);
  connection.storeModel(128, storage);
  assert.equal(connection.storedModel(storage), 128);
  connection.storeModel(200, storage);
  assert.equal(connection.storedModel(storage), 200);
  store["linnstrument.model"] = "26";
  assert.equal(connection.storedModel(storage), null);
  const refusing = { getItem: () => { throw new Error("denied"); }, setItem: () => { throw new Error("denied"); } };
  assert.equal(connection.storedModel(refusing), null);
  assert.doesNotThrow(() => connection.storeModel(128, refusing));
});

test("an extra output never offers the instrument's port or any other LinnStrument", () => {
  const ports = [{ id: "ls", name: "LinnStrument MIDI" }, { id: "iac", name: "IAC Driver Bus 1" },
                 { id: "v", name: "Virtual LinnStrument" }, { id: "net", name: "Network Mac Session" }];
  assert.deepEqual(connection.extraPorts(ports, "v").map((p) => p.id), ["iac", "net"]);
  assert.deepEqual(connection.extraPorts(ports, "net").map((p) => p.id), ["iac"]);
});

test("a port that answers neither query says so, with where to look on the instrument", () => {
  assert.deepEqual(connection.describe({ input: "IAC Driver Bus 1", output: "IAC Driver Bus 1", silent: true }), { text: "No answer", state: "" });
  assert.match(connection.statusText({ input: "IAC Driver Bus 1", output: "x", silent: true }), /^No answer from IAC Driver Bus 1\..*MIDI I\/O.*column 15/);
  assert.equal(connection.describe({ input: "a", output: "b", model: 200, silent: true }).text, "LinnStrument 200");
});
