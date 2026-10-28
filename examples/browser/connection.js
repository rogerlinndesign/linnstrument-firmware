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
// The instrument connection every LinnStrument page shares: Web MIDI access, the port lists,
// the queries that identify the instrument, and the widget in the header's top right (a dot and
// a label) that opens a panel with the status lines and the port selects. A page creates one
// with create({ onEvent, onPorts, onFirmware, extraOutputs: [{ id, label, aria, hint, pick }],
// extras, offlineNote }), gets every decoded message through onEvent, the chosen ports through
// onPorts and the news that the firmware is an old one through onFirmware, and uses what it
// returns: send, sendTo(id), ports(), info(), note(text, state), open(on, focus), addControl(el).
// Browser only; connection.test.js covers the pure parts.
(function (root, factory) {
  if (typeof module === "object" && module.exports) {
    module.exports = factory(require("./user_firmware_protocol.js"));
  } else {
    root.LinnConnection = factory(root.LinnUserFirmware);
  }
})(typeof self !== "undefined" ? self : this, function (ufm) {
  "use strict";

  // the header label and dot for a connection state
  function describe(info) {
    if (info.noMidi) return { text: "No Web MIDI", state: "" };
    if (info.refused) return { text: "MIDI access refused", state: "" };
    if (!info.input || !info.output) return { text: "Choose MIDI ports", state: "" };
    if (info.model) return { text: "LinnStrument " + info.model, state: "on" };
    if (info.silent) return { text: "No answer", state: "" };
    // 2.3.4 is the current release, so a firmware before 2.4.0 is connected and working, not a fault
    if (info.firmwareOld) return { text: "LinnStrument, 2.3.4 or earlier", state: "on" };
    return { text: info.input, state: "ready" };
  }

  // the ports an extra output may use: never the instrument's own, nor any other LinnStrument,
  // which would take the controller's messages as its own input
  function extraPorts(outputs, instrumentId) {
    return outputs.filter(function (p) { return p.id !== instrumentId && !/LinnStrument/.test(p.name); });
  }

  // The port to select from a list: the one chosen before if it is still there, else the first
  // whose name matches the pattern, else none ("").
  function pickPort(ports, kept, pattern) {
    if (kept && ports.some(function (p) { return p.id === kept; })) return kept;
    const match = pattern ? ports.find(function (p) { return pattern.test(p.name); }) : null;
    return match ? match.id : "";
  }

  // The firmware answers queries in the order they came and only 2.4.0 or later knows the model's
  // NRPN, so a mode answer without a model answer by the end of the wait means an older one.
  const OLD_FIRMWARE = "Firmware 2.3.4, the current release, or earlier: everything but text works. 2.4.0, in testing, adds model detection, faster lighting and text";
  function firmwareText(info) {
    if (info.firmwareOld) return OLD_FIRMWARE;
    return info.version ? "Firmware " + info.version : "";
  }
  // NRPN 273 answers 240 for firmware 2.4.0 and NRPN 274 its build; the instrument's own screen
  // shows them as 240 and .075
  function versionText(version, build) {
    const text = String(version).split("").join(".");
    return build === undefined || build === null ? text : text + " build " + build;
  }

  // the model a person picked, kept in the browser for every page of the same origin, since a
  // firmware before 2.4.0 cannot report it; storage can be missing or refused, as in a private window
  const MODEL_KEY = "linnstrument.model";
  function storedModel(storage) {
    try {
      const model = parseInt((storage || localStorage).getItem(MODEL_KEY), 10);
      return model === 200 || model === 128 ? model : null;
    } catch (e) {
      return null;
    }
  }
  function storeModel(model, storage) {
    try { (storage || localStorage).setItem(MODEL_KEY, String(model)); } catch (e) { }
  }

  function statusText(info) {
    if (info.noMidi) return "No Web MIDI here, use Chrome, Edge or Firefox";
    if (info.refused) return "MIDI access refused, allow it for this page";
    if (!info.input || !info.output) return "Choose the MIDI in and out";
    if (info.model) return "Connected to LinnStrument " + info.model;
    if (info.silent) return "No answer from " + info.input + ". If it's a LinnStrument, check that its MIDI I/O is set to USB: in Global Settings, the bottom pad of column 15";
    return "Connected to " + info.input;
  }

  function create(options) {
    const opts = options || {};
    const extraOutputs = opts.extraOutputs || [];
    let decoder = ufm.createDecoder();
    let access = null, input = null, output = null;
    let autoOpened = false;
    const chosenByPerson = {};   // select key -> the id the person picked, "" for None
    const extra = {};
    const info = { noMidi: false, refused: false, input: null, output: null, model: null, version: null, calibrated: null, firmwareOld: false, silent: false };
    let calibrationTimer = null;
    let modeAnswered = false;

    // --- the header widget and its panel

    const header = document.querySelector("header");
    const button = document.createElement("button");
    button.className = "connection";
    button.type = "button";
    button.setAttribute("aria-haspopup", "dialog");
    button.setAttribute("aria-expanded", "false");
    button.setAttribute("aria-controls", "connectionPanel");
    button.innerHTML = '<span class="dot" aria-hidden="true"></span><span class="label"></span><span class="flag" aria-hidden="true" hidden></span>';
    // what the panel says, announced even while the panel is closed
    const live = document.createElement("span");
    live.className = "visually-hidden";
    live.setAttribute("aria-live", "polite");
    const panel = document.createElement("div");
    panel.className = "connection-panel";
    panel.id = "connectionPanel";
    panel.setAttribute("role", "dialog");
    panel.setAttribute("aria-label", "Instrument connection");
    panel.hidden = true;
    let html = '<h2>Instrument</h2>' +
      '<div class="status" role="status" aria-live="polite"><span class="dot" aria-hidden="true"></span><span class="text"></span></div>' +
      '<div class="info firmware"></div><div class="info calibration" aria-live="polite"></div><div class="info note" aria-live="polite"></div>' +
      '<div class="ports">' +
      '<label><span class="caps">MIDI in</span><select class="port-in" aria-label="MIDI input from the instrument"></select></label>' +
      '<label><span class="caps">MIDI out</span><select class="port-out" aria-label="MIDI output to the instrument"></select></label>';
    extraOutputs.forEach(function (x) {
      html += '<label><span class="caps">' + x.label + '</span><select class="port-extra" data-id="' + x.id + '" aria-label="' + (x.aria || x.label) + '"></select></label>';
      if (x.hint) html += '<div class="hint">' + x.hint + '</div>';
    });
    html += '</div>';
    panel.innerHTML = html;
    (opts.extras || []).forEach(function (el) { panel.appendChild(el); });
    if (!header.querySelector(".spacer")) {
      const spacer = document.createElement("span");
      spacer.className = "spacer";
      header.appendChild(spacer);
    }
    header.appendChild(button);
    header.appendChild(panel);
    header.appendChild(live);

    const label = button.querySelector(".label");
    const statusBox = panel.querySelector(".status");
    const selIn = panel.querySelector(".port-in");
    const selOut = panel.querySelector(".port-out");
    const selExtra = {};
    panel.querySelectorAll(".port-extra").forEach(function (s) { selExtra[s.dataset.id] = s; });

    // focus moves into the panel only when the person opened it
    function open(on, focus) {
      panel.hidden = !on;
      button.setAttribute("aria-expanded", on ? "true" : "false");
      if (on && focus) {
        const first = panel.querySelector("select, button, input");
        if (first) first.focus();
      }
    }
    button.addEventListener("click", function () { open(panel.hidden, true); });
    document.addEventListener("keydown", function (e) {
      if (e.key === "Escape" && !panel.hidden) { open(false); button.focus(); }
    });
    document.addEventListener("pointerdown", function (e) {
      if (!panel.hidden && !panel.contains(e.target) && !button.contains(e.target)) open(false);
    });
    panel.addEventListener("focusout", function (e) {
      if (!panel.hidden && e.relatedTarget && !panel.contains(e.relatedTarget) && e.relatedTarget !== button) open(false);
    });

    let announced = "";
    function show() {
      const d = describe(info);
      label.textContent = d.text;
      button.setAttribute("aria-label", "Instrument connection: " + statusText(info));
      button.className = "connection" + (d.state ? " " + d.state : "");
      statusBox.className = "status" + (d.state ? " " + d.state : "");
      statusBox.querySelector(".text").textContent = statusText(info);
      const fw = panel.querySelector(".firmware");
      fw.textContent = firmwareText(info);
      fw.className = "info firmware";
      const cal = panel.querySelector(".calibration");
      if (info.calibrated === null || info.firmwareOld) { cal.textContent = ""; cal.className = "info calibration"; }
      else if (info.calibrated === "unknown") { cal.textContent = "Calibration not reported"; cal.className = "info calibration"; }
      else { cal.textContent = info.calibrated ? "Calibrated" : "Not calibrated, positions are approximate"; cal.className = "info calibration " + (info.calibrated ? "ok" : "warn"); }
      attention();
    }

    // the panel's warnings are not in view while it is closed, so the button flags them and
    // the live region reads them out when they change
    function attention() {
      const noteEl = panel.querySelector(".note");
      // an older firmware is in the label itself, so it needs no flag next to the dot
      const warn = info.calibrated === false || noteEl.className.indexOf("warn") >= 0;
      button.querySelector(".flag").hidden = !warn;
      const text = [statusText(info), info.firmwareOld ? OLD_FIRMWARE : "", panel.querySelector(".calibration").textContent, noteEl.textContent].filter(Boolean).join(". ");
      if (text !== announced) { announced = text; if (panel.hidden) live.textContent = text; }
    }

    // a line of the page's own in the panel, for example a warning about the instrument's mode
    function note(text, state) {
      const el = panel.querySelector(".note");
      el.textContent = text || "";
      el.className = "info note" + (state ? " " + state : "");
      attention();
    }

    // --- MIDI

    // a port that was unplugged throws on send; the lists catch up on the next statechange
    function sendOn(port, messages) {
      if (!port || !messages || !messages.length) return false;
      try {
        messages.forEach(function (m) { port.send(m); });
        return true;
      } catch (e) {
        return false;
      }
    }
    function send(messages) { return sendOn(output, messages); }
    function sendTo(id, messages) { return sendOn(extra[id], messages); }

    function onMessage(e) {
      const event = decoder.decode(Array.from(e.data));
      if (!event) return;
      switch (event.type) {
        case "model": info.model = event.model; info.firmwareOld = false; info.silent = false; show(); break;
        case "mode": modeAnswered = true; if (info.silent) { info.silent = false; show(); } break;
        case "version": info.versionNumber = event.version; info.version = versionText(event.version); show(); break;
        case "build": if (info.version) info.version = versionText(info.versionNumber, event.build); show(); break;
        case "calibrated": info.calibrated = event.calibrated; show(); break;
      }
      if (opts.onEvent) opts.onEvent(event);
    }

    // the first send to a port opens it, which fires statechange; only a port appearing,
    // disappearing or being unplugged (Chrome keeps it listed as disconnected) rebuilds the
    // lists, and only a change of the chosen ports acts on them
    function signature() {
      const ids = [];
      access.inputs.forEach(function (p) { ids.push("i" + p.id + p.state); });
      access.outputs.forEach(function (p) { ids.push("o" + p.id + p.state); });
      return ids.join("|");
    }
    function present(list) {
      const ports = [];
      list.forEach(function (p) { if (p.state !== "disconnected") ports.push(p); });
      return ports;
    }
    let known = null;
    function portsChanged() {
      const now = signature();
      if (now === known) return;
      known = now;
      fillPorts();
    }
    // Every list gets a None entry, so a missing instrument shows as missing instead of the
    // browser picking the first port. A port the person chose is kept while it exists, and a
    // None they chose stays None; otherwise the name rule picks.
    function fillPorts() {
      const before = instrumentChosen(), beforeExtras = extrasChosen();
      const inputs = present(access.inputs), outputs = present(access.outputs);
      const fill = function (select, ports, want) {
        select.innerHTML = "";
        select.appendChild(new Option("None", "", false, want === ""));
        ports.forEach(function (p) { select.appendChild(new Option(p.name, p.id, false, p.id === want)); });
      };
      const pick = function (key, ports, pattern) {
        const mine = chosenByPerson[key];
        if (mine === "") return "";
        return pickPort(ports, mine || "", pattern);
      };
      fill(selIn, inputs, pick("input", inputs, /LinnStrument/));
      fill(selOut, outputs, pick("output", outputs, /LinnStrument/));
      const others = extraPorts(outputs, selOut.value);
      extraOutputs.forEach(function (x) { fill(selExtra[x.id], others, pick(x.id, others, x.pick)); });
      if (instrumentChosen() !== before) choosePorts();
      else {
        if (extrasChosen() !== beforeExtras) chooseExtras();
        show();
      }
      if ((!selIn.value || !selOut.value) && !autoOpened) { autoOpened = true; open(true, false); }
    }
    function instrumentChosen() { return (selIn.value || "") + "|" + (selOut.value || ""); }
    function extrasChosen() { return Object.keys(selExtra).map(function (id) { return selExtra[id].value || ""; }).join("|"); }
    function chooseExtras() {
      Object.keys(selExtra).forEach(function (id) { extra[id] = access.outputs.get(selExtra[id].value) || null; });
      if (opts.onPorts) opts.onPorts(ports());
    }
    function choosePorts() {
      if (input) input.onmidimessage = null;
      input = access.inputs.get(selIn.value) || null;
      output = access.outputs.get(selOut.value) || null;
      Object.keys(selExtra).forEach(function (id) { extra[id] = access.outputs.get(selExtra[id].value) || null; });
      decoder = ufm.createDecoder();
      if (input) input.onmidimessage = onMessage;
      info.input = input ? input.name : null;
      info.output = output ? output.name : null;
      info.model = null; info.version = null; info.calibrated = null; info.firmwareOld = false; info.silent = false;
      modeAnswered = false;
      clearTimeout(calibrationTimer);
      note("");
      show();
      if (opts.onPorts) opts.onPorts(ports());
      if (input && output) {
        autoOpened = false;
        send(ufm.queryModel().concat(ufm.queryVersion(), ufm.queryCalibrated(), ufm.queryUserFirmwareMode()));
        calibrationTimer = setTimeout(function () {
          if (info.calibrated === null) info.calibrated = "unknown";
          if (info.model === null && modeAnswered) {
            info.firmwareOld = true;
            if (opts.onFirmware) opts.onFirmware(info);
          }
          // neither query answered: a port that is not a LinnStrument, or one whose MIDI goes
          // out on its jacks; a loopback port echoes the queries but never answers them
          if (info.model === null && !modeAnswered) info.silent = true;
          show();
        }, 1500);
      }
    }
    selIn.addEventListener("change", function () { chosenByPerson.input = selIn.value; choosePorts(); });
    // a new instrument port leaves the extra lists, so they are filled again around it
    selOut.addEventListener("change", function () { chosenByPerson.output = selOut.value; fillPorts(); choosePorts(); });
    Object.keys(selExtra).forEach(function (id) {
      selExtra[id].addEventListener("change", function () { chosenByPerson[id] = selExtra[id].value; chooseExtras(); });
    });

    function ports() {
      const result = { input: input, output: output };
      Object.keys(extra).forEach(function (id) { result[id] = extra[id]; });
      return result;
    }

    // without Web MIDI the page still works for what needs no instrument, and says so in a
    // banner under the header instead of a hidden status line
    function banner(title, text) {
      const el = document.createElement("div");
      el.className = "connection-banner";
      el.setAttribute("role", "alert");
      el.innerHTML = "<b></b> <span></span>";
      el.querySelector("b").textContent = title;
      el.querySelector("span").textContent = text;
      header.insertAdjacentElement("afterend", el);
    }

    show();
    if (!navigator.requestMIDIAccess) {
      info.noMidi = true;
      show();
      banner("No Web MIDI in this browser.", "This page talks to LinnStrument over Web MIDI, which Chrome, Edge and Firefox offer and Safari does not. " +
             "Open it in one of those to connect an instrument; without one you can still look around" + (opts.offlineNote ? " and " + opts.offlineNote : "") + ".");
    } else {
      label.textContent = "Allow MIDI access";
      navigator.requestMIDIAccess().then(function (a) {
        access = a;
        access.onstatechange = portsChanged;
        portsChanged();
      }, function () {
        info.refused = true;
        show();
        banner("MIDI access was refused.", "Allow it for this site in the browser's address bar or site settings, then reload the page.");
      });
    }

    // a page's own control in the header, left of the connection: a mode or run switch
    function addControl(el) {
      header.insertBefore(el, button);
    }

    return { send: send, sendTo: sendTo, ports: ports, info: function () { return info; }, note: note, open: open, addControl: addControl };
  }

  return { create: create, describe: describe, statusText: statusText, firmwareText: firmwareText, versionText: versionText, pickPort: pickPort, extraPorts: extraPorts,
           storedModel: storedModel, storeModel: storeModel };
});
