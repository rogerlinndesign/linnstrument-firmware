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
// The settings editor's logic: which parameters to read, the messages that read and write them,
// how the instrument's replies map back, and how the parameters group into a form. No DOM here,
// so settings_editor.test.js can run it in Node; settings_editor.html is the page around it.
(function (root, factory) {
  if (typeof module === "object" && module.exports) {
    module.exports = factory(require("./nrpn_parameters.js"), require("./user_firmware_protocol.js"));
  } else {
    root.LinnSettingsEditor = factory(root.LinnNrpns, root.LinnUserFirmware);
  }
})(typeof self !== "undefined" ? self : this, function (nrpns, ufm) {
  "use strict";

  const NRPN_QUERY = 299;
  const RESET_MSB = 101, RESET_LSB = 100;

  function cc(number, value, channel) {
    return [0xB0 | ((channel || 1) - 1), number, value];
  }

  // a query is the four control changes of NRPN 299 with the parameter number as its value
  function queryMessages(numbers, channel) {
    const out = [];
    numbers.forEach(function (n) {
      out.push(cc(99, NRPN_QUERY >> 7, channel), cc(98, NRPN_QUERY & 0x7F, channel), cc(6, n >> 7, channel), cc(38, n & 0x7F, channel));
    });
    return out;
  }

  // a write is the six control changes midi.md asks for: number, value, and the null RPN that
  // closes it
  function setMessages(number, value, channel) {
    if (!Number.isInteger(number) || number < 0 || number > 0x3FFF) throw new Error("NRPN number must be 0 to 16383");
    if (!Number.isInteger(value) || value < 0 || value > 0x3FFF) throw new Error("NRPN value must be 0 to 16383");
    return [cc(99, number >> 7, channel), cc(98, number & 0x7F, channel), cc(6, value >> 7, channel), cc(38, value & 0x7F, channel),
            cc(RESET_MSB, 127, channel), cc(RESET_LSB, 127, channel)];
  }

  // the parameters the instrument can answer a query for: everything but the pure actions
  function queryable(parameters) {
    return (parameters || nrpns.parameters).filter(function (p) { return p.kind !== "action"; });
  }

  // The first answer that proves an instrument is listening: the model on firmware 2.4.0 or
  // later, otherwise the mode, which every firmware answers and which is queried last.
  function startsRead(event) {
    return !!event && (event.type === "model" || event.type === "mode");
  }

  // the decoder names a few replies; every reply is a parameter number and its value here
  function replyValue(event) {
    if (!event) return null;
    switch (event.type) {
      case "nrpn": return { number: event.number, value: event.value };
      case "mode": return { number: 245, value: event.active ? 1 : 0 };
      case "model": return { number: 272, value: event.model };
      case "version": return { number: 273, value: event.version };
      case "build": return { number: 274, value: event.build };
      case "calibrated": return { number: 275, value: event.calibrated ? 1 : 0 };
    }
    return null;
  }

  function clampValue(parameter, value) {
    if (!Number.isFinite(value)) return parameter.min;
    return Math.max(parameter.min, Math.min(parameter.max, Math.round(value)));
  }

  function formatValue(parameter, value) {
    if (value === null || value === undefined) return "";
    if (parameter.options) {
      const option = parameter.options.find(function (o) { return o.value === value; });
      if (option) return option.label;
    }
    return String(value);
  }

  // consecutive toggles that only differ by a trailing number or note name become one row of
  // small switches: the 16 per-note channels, the 12 note lights
  const trailingToken = /^(.*\S) (\d+|[A-G]#?)$/;
  function groupToggles(parameters) {
    const rows = [];
    parameters.forEach(function (p) {
      const m = p.kind === "toggle" ? trailingToken.exec(p.name) : null;
      const last = rows[rows.length - 1];
      if (m && last && last.kind === "toggles" && last.name === m[1]) {
        last.items.push({ parameter: p, label: m[2] });
      } else if (m) {
        rows.push({ kind: "toggles", name: m[1], items: [{ parameter: p, label: m[2] }] });
      } else {
        rows.push({ kind: p.kind, name: p.name, parameter: p });
      }
    });
    return rows.map(function (r) {
      if (r.kind === "toggles" && r.items.length === 1) return { kind: "toggle", name: r.items[0].parameter.name, parameter: r.items[0].parameter };
      return r;
    });
  }

  // the form: global and device settings, the two splits side by side, the actions apart
  function sections(parameters) {
    const all = parameters || nrpns.parameters;
    const settings = function (group) {
      return all.filter(function (p) { return p.group === group && p.kind !== "action" && p.kind !== "choice-action"; });
    };
    const left = groupToggles(settings("Split Left"));
    const right = groupToggles(settings("Split Right"));
    const splits = left.map(function (row, i) {
      const twin = right.find(function (r) { return r.name === row.name && r.kind === row.kind; }) || right[i];
      return { kind: row.kind, name: row.name, left: row, right: twin };
    });
    const actions = all.filter(function (p) { return p.kind === "action" || p.kind === "choice-action"; });
    return [
      { id: "global", title: "Global", rows: groupToggles(settings("Global")) },
      { id: "splits", title: "Splits", rows: splits },
      { id: "device", title: "Device", rows: groupToggles(settings("Device")) },
      { id: "actions", title: "Actions", rows: actions.map(function (p) { return { kind: p.kind, name: p.group + " " + p.name, parameter: p }; }) },
    ];
  }

  function matches(row, text) {
    const needle = (text || "").trim().toLowerCase();
    if (!needle) return true;
    const names = [row.name];
    if (row.items) row.items.forEach(function (i) { names.push(i.parameter.name); });
    if (row.parameter) names.push(row.parameter.description, String(row.parameter.number));
    return names.some(function (n) { return n.toLowerCase().indexOf(needle) >= 0; });
  }

  return {
    parameters: nrpns.parameters,
    tables: nrpns.tables,
    queryable: queryable,
    startsRead: startsRead,
    queryMessages: queryMessages,
    setMessages: setMessages,
    replyValue: replyValue,
    clampValue: clampValue,
    formatValue: formatValue,
    sections: sections,
    matches: matches,
    createDecoder: ufm.createDecoder,
    queryModel: ufm.queryModel,
    queryVersion: ufm.queryVersion,
    queryCalibrated: ufm.queryCalibrated,
  };
});
