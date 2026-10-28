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
// Run with: node --test examples/browser/pages.test.js
// The pages keep most of their code inline, where a stray quote in a text breaks the whole page
// and no module test notices; this compiles every inline script of every page.
const test = require("node:test");
const assert = require("node:assert/strict");
const fs = require("node:fs");
const path = require("node:path");
const vm = require("node:vm");

const pages = fs.readdirSync(__dirname).filter((f) => f.endsWith(".html"));

test("every page's inline scripts compile", () => {
  assert.ok(pages.length >= 4, "found " + pages.length + " pages");
  for (const page of pages) {
    const html = fs.readFileSync(path.join(__dirname, page), "utf8");
    const scripts = [...html.matchAll(/<script(?![^>]*\bsrc=)[^>]*>([\s\S]*?)<\/script>/g)].map((m) => m[1]);
    scripts.forEach((code, i) => {
      assert.doesNotThrow(() => new vm.Script(code, { filename: page + " script " + (i + 1) }), page + " script " + (i + 1));
    });
  }
});
