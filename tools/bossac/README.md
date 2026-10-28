bossac
======

This is the SAM3X flashing tool that the companion tool runs for `flash` and `update`, so that a
clone of this repository can flash a LinnStrument without the LinnStrument Updater or an Arduino
installation.

* `macos/bossac`: BOSSA 1.6.1-arduino built as a universal x86_64 and arm64 binary, signed with
  the Developer ID of Uwyn, LLC and the hardened runtime, and notarised by Apple. Note that the
  copy in the Arduino SAM package is Intel only.
* `windows/bossac.exe`: the same version for Windows.
* On Linux, or when a bundled copy is missing, the tool uses the copy that arduino-cli installs
  with the SAM core. On every platform, you can name another binary to use instead with
  `LINN_BOSSAC`.

Both binaries come from the LinnStrument Updater, which builds them from the BOSSA 1.6.1-arduino
sources it carries. The macOS binary was signed again in 2026, after the 2020 certificate
expired, and the Updater carries that same copy. Any change to it breaks its signature.
BOSSA is by ShumaTech under the BSD licence in `LICENSE`.
