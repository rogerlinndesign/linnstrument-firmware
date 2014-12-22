Arduino fixes for the LinnStrument software
===========================================

In order to get better performance through the serial port, we had to improve
the Arduino API for writing serial data.

This directory contains for files that need to be replaced in the
Arduino IDE. Note that there are different files for the 1.5.6-r2 and 1.5.7 versions:

 * UARTClass.cpp
 * UARTClass.h
 * USARTClass.cpp
 * USARTClass.h

These need to be copied to the hardware/arduino/sam/cores/arduino/ directory.

On MacOSX this is located inside the Arduino.app:

  Arduino.app/Contents/Resources/Java

On the Terminal, this command should copy them all to the right location in
one go, make sure that the 'arduino_fixes' directory is your current directory
first:

For 1.5.6-rc2:

  cp -v 1.5.6-rc2/*ART* /Applications/Arduino.app/Contents/Resources/Java/hardware/arduino/sam/cores/arduino/

For 1.5.7 and 1.5.8, using Java 6:

  cp -v 1.5.7-1.5.8/*ART* /Applications/Arduino.app/Contents/Resources/Java/hardware/arduino/sam/cores/arduino/

For 1.5.7 and 1.5.8, using Java 7:

  cp -v 1.5.7-1.5.8/*ART* /Applications/Arduino.app/Contents/Java/hardware/arduino/sam/cores/arduino/

If you don't want to use out improved serial write code, you can comment out
the 'PATCHED_ARDUINO_SERIAL_WRITE' define in the linnstrument.ino file. This
will allow you to compile the LinnStrument software with the stock Arduino
IDE, but the lights on the LinnStrument might flicker when using DIN MIDI
output.