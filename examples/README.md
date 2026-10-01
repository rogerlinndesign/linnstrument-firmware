Examples
========

Ways to use LinnStrument's User Firmware Mode (see `user_firmware_mode.md`) without changing the
firmware.

* `user_firmware_console.html`: open it in Chrome or Edge, pick the LinnStrument, and you get a
  live view of every touch with pressure, position and slides, plus click-to-paint lights. It
  needs no server; a double click on the file is enough.
* `user_firmware_protocol.js`: the MIDI message builders and the touch stream decoder the page
  uses, written to be lifted into your own project.
  `node --test examples/user_firmware_protocol.test.js` runs its tests.
* `user_firmware_demo.sh`: the same walkthrough from a shell with SendMIDI and ReceiveMIDI: enter
  the mode, enable pressure and position for two rows, paint a diagonal, ask the instrument for
  its state, watch touches, leave the mode.

For an AI agent, RouteMIDI's MCP server gives the same access as tools; `user_firmware_mode.md`
ends with the recipe.
