Examples
========

These are ways to talk to LinnStrument over MIDI without changing the firmware. They're in two
folders. `browser/` holds pages that run in Chrome, Edge or Firefox, and it's what a web host
serves. `shell/` holds a walkthrough with command line tools.

Browser pages
-------------

Here's how to get started:

1. Open https://uwyn.com/linnstrument/ in Chrome, Edge or Firefox. Safari has no Web MIDI, and
   note that on an iPhone or iPad no browser has Web MIDI, Chrome included.
2. Allow MIDI when the browser asks. It remembers that for the site.
3. Open one of the three pages. The instrument is found by its name, and the dot and label at
   the top right open the connection panel, with the status and the MIDI ports.

The pages work with firmware 2.3.4, the current release. The next firmware, 2.4.0, isn't
released yet, and it adds things the pages use when it's there. On 2.3.4 the header reads
"LinnStrument, 2.3.4 or earlier" with a green dot, and:

* The pages can't ask the instrument which model it is, so the connection panel opens with a
  model picker. A LinnStrument 128 owner picks 128 there once, and the page remembers it.
* The designer lights the cells one by one, which is slower to repaint the whole surface.
* The console's text controls wait for 2.4.0, with a line saying why.
* The settings editor marks the four settings that only 2.4.0 has (model, firmware version,
  build, calibrated) as needing it.

These are the pages:

* `settings_editor.html`: every setting the instrument exposes over MIDI, as a form with the two
  splits side by side. When you connect, it reads all of them back from the instrument. A change
  is sent at once and read back again, so the form always shows what the instrument kept. Keep
  in mind that a change is kept on the instrument and has no undo, so note a value before
  changing it. Setting Device MIDI I/O to MIDI Jacks, which asks for a second click, moves MIDI
  off USB and the page loses the instrument. You then switch it back on the instrument itself by
  tapping the bottom pad of column 15 in Global Settings.
* `surface_designer.html`: LinnStrument as the MIDI controller you draw, in User Firmware Mode
  (see `user_firmware_mode.md`). You drag across the pads to make zones of five kinds. Pad
  zones play notes with pressure as aftertouch, keys lay a scale out as an isomorphic keyboard,
  faders send a CC along their cells (bipolar for pan), XY pads send two CCs from the finger's
  position (springing back if wanted), and buttons send a CC, a note or a program change,
  latching or momentary. The lights show the layout and its state. Layouts are saved in the
  browser and export and import as JSON, and a layout made for a 200 keeps what lies past column
  16 while a 128 is connected.
  Nine starting points are built in, from a mixer to a clip grid, and the control switches in
  the left column jump between layouts while you play. The layout runs in the page, not on the
  instrument: the computer and the page stay open while you play, and the instrument keeps
  nothing of it once Run is off.
* `user_firmware_console.html`: a live view of every touch with pressure, position and slides,
  click-to-paint lights, and, with firmware 2.4.0, text to draw or scroll in any of the fonts.

### Using the designer with your DAW

The designer sends its messages on the "Controller out" port of your choice. A synth takes them
directly, but a DAW needs a loopback port. Here's how to set that up:

1. Make the port. On macOS, open Audio MIDI Setup, choose Window > Show MIDI Studio,
   double-click IAC Driver and tick "Device is online". On Windows, install loopMIDI and add a
   port.
2. In the designer, pick that port as Controller out.
3. In the DAW, enable that port as a MIDI input, and switch off the LinnStrument's own input (or
   its track and remote use) while the designer runs. In User Firmware Mode the instrument sends
   raw touches on its own port, which would otherwise play notes or be grabbed by MIDI learn
   instead of the zone's message.
4. Switch Run on in the header. The instrument enters User Firmware Mode and the layout lights
   up.
5. Map each zone with the DAW's MIDI learn. "Send for learn" fires a zone's messages without a
   touch.
6. Switch Run off to hand the instrument back.

If the page closes unexpectedly while Run is on (a crash, the computer sleeping, the cable
pulled), the instrument stays in User Firmware Mode. It's then silent, with its own lights off.
To leave the mode, hold OS Update in Global Settings (column 16, third row from the bottom) for
a full second, or switch the instrument off and on. Note that a short tap on OS Update switches
the instrument to firmware update mode instead, so just tap it again to come back.

### For developers

The pages share `linnstrument.css`, which has the look of rogerlinndesign.com, and
`connection.js`, the connection widget. That widget takes care of the queries that identify the
instrument (NRPNs 272 to 275 for its model, firmware and calibration, which need 2.4.0), the
port choice, and a banner when the browser has no Web MIDI or refused it. On 2.4.0 the designer
paints runs of lights with CC 25. On an older firmware the header reads
"LinnStrument, 2.3.4 or earlier" and the pages fall back as described above. The settings
editor's form is generated from the NRPN table in `midi.md`. `user_firmware_protocol.js` holds
the MIDI message builders and the touch stream decoder the pages use, and it's written so that
you can lift it into your own project.

The pages are published at https://uwyn.com/linnstrument/ with `tools/deploy-pages.sh`, which
sends them to the rsync destination in `LINN_PAGES_TARGET`, and `tools/deploy-pages.sh -n` shows
what it would change. The variable keeps the server out of this public repository. It sends only
what the pages load and links each script and stylesheet by its content hash, because Cloudflare
keeps those for four hours while the HTML is never cached. `index.html` is the front page, and
any other static web host over https, which Web MIDI requires, works as well: copy `browser/`
there.

You can also run the pages from a copy of this repository. Get the files with "Code > Download
ZIP" on the repository's GitHub page, unzip them and open `examples/browser/index.html` with
Chrome, Edge or Firefox. On a Mac a double-click opens Safari, so right-click the file and choose
Open With. A page opened from a file asks for MIDI on every visit, because a file has no origin
the browser can remember the grant for. Nothing is stored
anywhere but in the browser. `connection.js`, `settings_editor.js`, `surface_designer.js` and
`user_firmware_protocol.js` are modules with tests beside them, which you run with
`node --test examples/browser/*.test.js`. `go run ./tools/linnstrument nrpns` regenerates
`nrpn_parameters.js` from `midi.md`. Finally, `go run ./tools/linnstrument serve` serves the
folder on a local port and opens the front page, and when the pages are served that way the
browser asks for MIDI once.

Shell
-----

* `shell/user_firmware_demo.sh`: the console's walkthrough from a shell with SendMIDI and
  ReceiveMIDI. It enters the mode, enables pressure and position for two rows, paints a diagonal
  and a row, draws and scrolls text, asks the instrument for its model and state, watches
  touches, and leaves the mode. With `LINNSTRUMENT_DEVICE="Virtual LinnStrument"` it runs
  against the Virtual LinnStrument app instead of an instrument, which is the quickest way to
  see the mode without one.

For an AI agent, RouteMIDI's MCP server gives the same access as tools. `user_firmware_mode.md`
has the recipe under "Driving it from scripts and AI agents".
