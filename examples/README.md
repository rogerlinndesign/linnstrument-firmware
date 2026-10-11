Examples
========

Pages and scripts that talk to LinnStrument over MIDI without changing the firmware. The two
pages share `linnstrument.css`, the look of rogerlinndesign.com, and need no server: a double
click on the file in Chrome or Edge is enough.

* `settings_editor.html`: every setting the instrument exposes over MIDI, as a form with the two
  splits side by side. On connecting it reads all of them back from the instrument; a change is
  sent at once and read back again, so the form always shows what the instrument kept. The form
  is generated from the NRPN table in `midi.md`: `go run ./tools/linnstrument nrpns` writes
  `nrpn_parameters.js`, and the page logic in `settings_editor.js` is tested by
  `node --test examples/settings_editor.test.js`.

Ways to use LinnStrument's User Firmware Mode (see `user_firmware_mode.md`):

* `user_firmware_console.html`: open it in Chrome or Edge, pick the LinnStrument, and you get a
  live view of every touch with pressure, position and slides, click-to-paint lights, and text
  to draw or scroll in any of the fonts. It asks the instrument which model it is. It needs no
  server; a double click on the file is enough.
* `user_firmware_protocol.js`: the MIDI message builders and the touch stream decoder the page
  uses, written to be lifted into your own project.
  `node --test examples/user_firmware_protocol.test.js` runs its tests.
* `user_firmware_demo.sh`: the same walkthrough from a shell with SendMIDI and ReceiveMIDI: enter
  the mode, enable pressure and position for two rows, paint a diagonal and a row, draw and
  scroll text, ask the instrument for its model and state, watch touches, leave the mode. With
  `LINNSTRUMENT_DEVICE="Virtual LinnStrument"` it runs against the Virtual LinnStrument app
  instead of an instrument, which is the quickest way to see the mode without one.

For an AI agent, RouteMIDI's MCP server gives the same access as tools; `user_firmware_mode.md`
ends with the recipe.
