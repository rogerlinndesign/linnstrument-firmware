Arduino fixes for the LinnStrument software
===========================================

In order to get better performance through the serial port, we had to improve
the Arduino API for writing serial data.

We currently support Arduino IDE 1.6.5 with version 1.6.4 of the Arduino SAM boards.

Make sure to first install the required packages for the Arduino SAM Boards through
the Arduino IDE board manager from the Tools menu.

This readme's directory contains files that need to overwrite files in the Arduino IDE.

 * UARTClass.cpp
 * UARTClass.h

On MacOSX these need to be copied to this directory:
$HOME/Library/Arduino15/packages/arduino/hardware/sam/1.6.4/cores/arduino/

Using the Terminal, the following command copies them all to the right location in one go.

Make sure that the 'arduino_fixes' directory is your current directory first:
cp -v 1.6.4/*ART* $HOME/Library/Arduino15/packages/arduino/hardware/sam/1.6.4/cores/arduino/

If you don't want to use out improved serial write code, you can comment out
the 'PATCHED_ARDUINO_SERIAL_WRITE' define in the linnstrument.ino file. This
will allow you to compile the LinnStrument software with the stock Arduino
IDE, but the lights on the LinnStrument might flicker when using DIN MIDI
output.