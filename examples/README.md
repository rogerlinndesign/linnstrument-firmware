Examples
========

Pages and scripts that talk to LinnStrument over MIDI without changing the firmware. The
pages share `linnstrument.css`, the look of rogerlinndesign.com, and `connection.js`, the
instrument connection in the top right of every page: a dot and a label that open a panel with
the status and the MIDI ports, the queries that identify the instrument, and a banner when the
browser has no Web MIDI or refused it (its port choice is tested by
`node --test examples/connection.test.js`). A double click on a file in Chrome, Edge or Firefox
opens it (Safari has no Web MIDI), but a page opened from a file asks for MIDI access on every
visit, because a file has no origin the browser can remember a grant for.
`go run ./tools/linnstrument serve` serves the folder on a local port and opens the front page;
served that way the browser asks once. `index.html` is the front page for hosting them: copy
the folder to any static web host over https, which Web MIDI requires, and the pages work as
they do locally; nothing is stored anywhere but the browser. The pages ask the instrument for
its model, firmware and calibration (NRPNs 272 to 275) and paint runs of lights with CC 25,
which need firmware 2.4.0 or later; with an older one the connection panel says so, the model
is the one picked by hand, the designer lights the cells one by one, and the console's text
stays dark.

* `settings_editor.html`: every setting the instrument exposes over MIDI, as a form with the two
  splits side by side. On connecting it reads all of them back from the instrument; a change is
  sent at once and read back again, so the form always shows what the instrument kept. The form
  is generated from the NRPN table in `midi.md`: `go run ./tools/linnstrument nrpns` writes
  `nrpn_parameters.js`, and the page logic in `settings_editor.js` is tested by
  `node --test examples/settings_editor.test.js`.

Ways to use LinnStrument's User Firmware Mode (see `user_firmware_mode.md`):

* `surface_designer.html`: LinnStrument as the MIDI controller you draw. Drag across the pads to
  make zones: pads that play notes with pressure as aftertouch, keys that lay a scale out as an
  isomorphic keyboard, faders that send a CC along their cells (bipolar for pan), XY pads that
  send two CCs from the finger's position (springing back if wanted), and buttons that send a
  CC, a note or a program change, latching or momentary. The lights show the layout and its
  state. The messages go out on a port of your choice, so a loopback port (the IAC Driver on
  macOS, loopMIDI on Windows) carries them into any DAW through MIDI learn, or a synth takes
  them directly; "Send for learn" fires a zone's messages without a touch. Layouts save in the
  browser, export and import as JSON, and a layout made for a 200 keeps what lies past column 16
  while a 128 is connected; nine starting points are built in, from a mixer to a clip grid, and
  the control switches in the left column jump between layouts while playing.
  `surface_designer.js` holds the zone model and the touch translation, tested by
  `node --test examples/surface_designer.test.js`.
* `user_firmware_console.html`: open it in Chrome, Edge or Firefox, pick the LinnStrument at the
  top right, and you get a live view of every touch with pressure, position and slides,
  click-to-paint lights, and text to draw or scroll in any of the fonts. It asks the instrument
  which model it is.
* `user_firmware_protocol.js`: the MIDI message builders and the touch stream decoder the page
  uses, written to be lifted into your own project.
  `node --test examples/user_firmware_protocol.test.js` runs its tests.
* `user_firmware_demo.sh`: the same walkthrough from a shell with SendMIDI and ReceiveMIDI: enter
  the mode, enable pressure and position for two rows, paint a diagonal and a row, draw and
  scroll text, ask the instrument for its model and state, watch touches, leave the mode. With
  `LINNSTRUMENT_DEVICE="Virtual LinnStrument"` it runs against the Virtual LinnStrument app
  instead of an instrument, which is the quickest way to see the mode without one.

For an AI agent, RouteMIDI's MCP server gives the same access as tools; `user_firmware_mode.md`
has the recipe under "Driving it from scripts and AI agents".
