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
  assert.deepEqual(connection.describe({ input: "LinnStrument MIDI", output: "LinnStrument MIDI", firmwareOld: true }), { text: "Older firmware", state: "ready" });
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
  assert.equal(connection.firmwareText({ version: "240.075" }), "Firmware 240.075");
  assert.match(connection.firmwareText({ version: null, firmwareOld: true }), /2\.4\.0 or later.*Updater/);
});

test("the status line in the panel says what to do", () => {
  assert.equal(connection.statusText({ noMidi: true }), "No Web MIDI here, use Chrome, Edge or Firefox");
  assert.equal(connection.statusText({ input: null }), "Choose the MIDI in and out");
  assert.equal(connection.statusText({ input: "Virtual LinnStrument", output: "Virtual LinnStrument" }), "Connected to Virtual LinnStrument");
  assert.equal(connection.statusText({ input: "a", output: "b", model: 200 }), "Connected to LinnStrument 200");
  assert.equal(connection.statusText({ input: "LinnStrument MIDI", output: "b", firmwareOld: true }), "Connected to LinnStrument MIDI, older firmware");
});
