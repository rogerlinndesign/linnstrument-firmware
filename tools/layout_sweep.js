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
// Checks the browser pages' layout at every width from a phone to a wide screen, for both models.
// Serve the pages with `go run ./tools/linnstrument serve`, open any of them in Chrome, and paste
// this file into the DevTools console. Each page loads in an iframe of the given width with Web
// MIDI blocked, so no instrument is touched, and the header label set to the longest one a real
// instrument shows; the result lists what is wrong per page and width:
// a page that scrolls sideways, content under the footer, an instrument area squeezed short, an
// element past the window's edge (the instrument's own sideways scroller is allowed).
(async () => {
  const PAGES = ["index.html", "surface_designer.html", "settings_editor.html", "user_firmware_console.html"];
  const WIDTHS = [320, 360, 480, 600, 760, 800, 960, 961, 1000, 1100, 1200, 1600];
  const probe = (w) => {
    const d = w.document, vw = w.innerWidth, out = [];
    if (d.documentElement.scrollWidth > vw + 1) out.push("page sideways " + (d.documentElement.scrollWidth - vw));
    if (d.querySelector("header").getBoundingClientRect().height < 50) out.push("header squeezed");
    const footer = d.querySelector("footer").getBoundingClientRect();
    let bottom = 0;
    d.querySelectorAll("main > *").forEach((e) => { bottom = Math.max(bottom, e.getBoundingClientRect().bottom); });
    if (vw < 960 && bottom > footer.top + 1) out.push("under the footer by " + Math.round(bottom - footer.top));
    const stage = d.querySelector(".stage");
    if (stage && stage.scrollWidth > stage.clientWidth + 1) out.push("instrument area sideways");
    if (stage && vw < 960 && stage.getBoundingClientRect().height < 300) out.push("instrument area short");
    const content = d.querySelector(".content");
    if (content && content.scrollWidth > content.clientWidth + 1) out.push("settings sideways " + (content.scrollWidth - content.clientWidth));
    d.querySelectorAll("main *, header *").forEach((e) => {
      const b = e.getBoundingClientRect();
      if (b.width && b.right > vw + 1 && e.offsetParent && !e.closest(".instrument-scroll")) {
        out.push("past the edge: " + e.tagName.toLowerCase() + (e.id ? "#" + e.id : e.className ? "." + String(e.className).split(" ")[0] : ""));
      }
    });
    return [...new Set(out)].slice(0, 6);
  };
  const kept = (() => { try { return localStorage.getItem("linnstrument.model"); } catch (e) { return null; } })();
  const report = [];
  for (const model of ["200", "128"]) {
    localStorage.setItem("linnstrument.model", model);
    for (const page of PAGES) {
      const problems = [];
      for (const width of WIDTHS) {
        const frame = document.createElement("iframe");
        frame.setAttribute("allow", "midi 'none'");
        frame.style.cssText = "position:absolute;left:0;top:0;border:0;height:800px;width:" + width + "px";
        document.body.appendChild(frame);
        await new Promise((done) => { frame.onload = done; frame.src = page; });
        await new Promise((done) => setTimeout(done, 200));
        const label = frame.contentWindow.document.querySelector(".connection .label");
        if (label) label.textContent = "LinnStrument, 2.3.4 or earlier";
        await new Promise((done) => setTimeout(done, 30));
        const found = probe(frame.contentWindow);
        if (found.length) problems.push(width + ": " + found.join("; "));
        frame.remove();
      }
      report.push(model + " " + page + ": " + (problems.length ? problems.join(" | ") : "ok"));
    }
  }
  if (kept === null) localStorage.removeItem("linnstrument.model"); else localStorage.setItem("linnstrument.model", kept);
  console.log(report.join("\n"));
  return report.join("\n");
})();
