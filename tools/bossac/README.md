bossac
======

The SAM3X flashing tool the companion tool runs for `flash` and `update`, so that a clone of this
repository can flash a LinnStrument without the LinnStrument Updater or an Arduino installation.

* `macos/bossac`: BOSSA 1.6.1-arduino built as a universal x86_64 and arm64 binary, signed with
  the Developer ID of Uwyn and the hardened runtime. The copy in the Arduino SAM package is
  Intel only.
* `windows/bossac.exe`: the same version for Windows.
* On Linux the tool uses the copy arduino-cli installs with the SAM core.

Both binaries come from the LinnStrument Updater, which builds them from the BOSSA 1.6.1-arduino
sources it carries; keep them byte-identical to the Updater's, since a modified macOS binary
loses its signature. BOSSA is by ShumaTech under the BSD licence in `LICENSE`.
