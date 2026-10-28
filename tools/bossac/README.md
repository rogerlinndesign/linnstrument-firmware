bossac
======

This is the SAM3X flashing tool that the companion tool runs for `flash` and `update`, so that a
clone of this repository can flash a LinnStrument without the LinnStrument Updater or an Arduino
installation.

* `macos/bossac`: BOSSA 1.6.1-arduino built as a universal x86_64 and arm64 binary, signed with
  the Developer ID of Uwyn and the hardened runtime. Note that the copy in the Arduino SAM
  package is Intel only.
* `windows/bossac.exe`: the same version for Windows.
* On Linux, or when a bundled copy is missing, the tool uses the copy that arduino-cli installs
  with the SAM core. On every platform, you can name another binary to use instead with
  `LINN_BOSSAC`.

Both binaries come from the LinnStrument Updater, which builds them from the BOSSA 1.6.1-arduino
sources it carries. Keep them byte-identical to the Updater's, since a modified macOS binary
loses its signature. BOSSA is by ShumaTech under the BSD licence in `LICENSE`.
