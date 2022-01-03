/***************************** ls_arpeggiator: LinnStrument Arpeggiator ****************************
This work is licensed under the Creative Commons Attribution-ShareAlike 3.0 Unported License.
To view a copy of this license, visit http://creativecommons.org/licenses/by-sa/3.0/
or send a letter to Creative Commons, PO Box 1866, Mountain View, CA 94042, USA.
***************************************************************************************************
These routines implement the internal LinnStrument arpeggiator. They closely work together with the
system clock when that is active and with the internal tracking of how notes map for each split to
the actual cell that was pressed, allowing velocity to be continuously varied during the arpeggiator
sequence.
***************************************************************************************************/
     
signed char lastArpNote[NUMSPLITS];                   // the last note played by the arpeggiator or -1 if it should be starting from scratch
signed char lastArpChannel[NUMSPLITS];                // the last channel played by the arpeggiator or -1 if it should be starting from scratch
boolean lastArpStepOdd[NUMSPLITS];                    // indicates whether the last arpeggiator step was odd (1-based : 1, 3, 5)
ArpeggiatorDirection arpUpDownState[NUMSPLITS];       // the state in the alternating up/down pattern
signed char arpOctaveState[NUMSPLITS];                // the octave that is used while playing the arpeggiator sequence

unsigned long lastTapTempo = 0;

#define MICROS_PER_MINUTE 60000000UL

void initializeArpeggiator() {
  randomSeed(analogRead(0));

  for (byte split = 0; split < NUMSPLITS; ++split) {
    noteTouchMapping[split].initialize();
    resetArpeggiatorState(split);
  }
}

void resetArpeggiatorState(byte split) {
  // Send note off event to stale note
  if(noteTouchMapping[split].hasStaleNote()) {
    midiSendNoteOff(split, noteTouchMapping[split].staleNote, noteTouchMapping[split].staleChannel);
    noteTouchMapping[split].noteOff(noteTouchMapping[split].staleNote, noteTouchMapping[split].staleChannel);
  }

  lastArpNote[split] = -1;
  lastArpChannel[split] = -1;
  lastArpStepOdd[split] = false;
  arpUpDownState[split] = ArpUp;
  arpOctaveState[split] = 0;
}

byte getArpeggiatorNote(byte split, byte notenum) {
  return getOctaveNote(arpOctaveState[split], notenum);
}

byte getOctaveNote(byte octave, byte notenum) {
  return notenum + (octave * 12);
}

void temporarilyEnableArpeggiator() {
  arpTempoDelta[sensorSplit] = 0;
  midiSendNoteOffForAllTouches(sensorSplit);
  resetArpeggiatorState(sensorSplit);
}

void disableTemporaryArpeggiator() {
  turnArpeggiatorOff(sensorSplit);
}

void handleArpeggiatorNoteOff(byte split, byte notenum, byte channel) {
  // Represents whether a note should be removed from the cell mapping
  boolean shouldRemoveMapping = true;

  // handle replay all differently since it plays multiple notes simultaneously
  if (Global.arpDirection == ArpReplayAll) {
    if (lastArpNote[split] != -1) {
      for (byte octave = 0; octave <= Global.arpOctave; ++octave) {
        midiSendNoteOff(split, getOctaveNote(octave, notenum), channel);
      }
    }
  }
  // if this is a strummed note, always turn all octave notes off
  else if (isStrummedSplit(split)) {
    for (byte octave = 0; octave <= Global.arpOctave; ++octave) {
      midiSendNoteOff(split, getOctaveNote(octave, notenum), channel);
    }
  }
  // Handle single note sequences. If we just released the note that the arp is playing
  // then set that note as stale and defer both the midi off and the cell map removal
  // until later in the arp update when the note has been turned off.
  else if (lastArpNote[split] == notenum && lastArpChannel[split] == channel) {
    if(!noteTouchMapping[split].isNoteStale(notenum, channel)) {
      noteTouchMapping[split].setStaleNote(notenum, channel);
    }
    shouldRemoveMapping = false;
  }

  if(shouldRemoveMapping) {
    noteTouchMapping[split].noteOff(notenum, channel);
  }

  // reset state when no notes are played at all anymore
  if (!noteTouchMapping[split].isAnyNotePressed()) {
    resetArpeggiatorState(split);
  }
}

void turnArpeggiatorOff(byte split) {
  sendArpeggiatorStepMidiOff(split);
  resetArpeggiatorState( split);
}

void sendArpeggiatorStepMidiOff(byte split) {
  if (lastArpNote[split] != -1) {
    if (Global.arpDirection == ArpReplayAll) {
      if (noteTouchMapping[split].noteCount > 0) {
        signed char arpNote = noteTouchMapping[split].firstNote;
        signed char arpChannel = noteTouchMapping[split].firstChannel;

        while (arpNote != -1) {
          for (byte octave = 0; octave <= Global.arpOctave; ++octave) {
            midiSendNoteOff(split, getOctaveNote(octave, arpNote), arpChannel);
          }

          NoteEntry* entry = noteTouchMapping[split].getNoteEntry(arpNote, arpChannel);
          if (entry == NULL) {
            arpNote = -1;
          }
          else {
            arpNote = entry->getNextNote();
            arpChannel = entry->getNextChannel();
          }
        }
      }
    }
    else {
      midiSendNoteOff(split, getArpeggiatorNote(split, lastArpNote[split]), lastArpChannel[split]);
    }
  }
}

inline void checkAdvanceArpeggiator() {
  checkAdvanceArpeggiatorForSplit(LEFT);
  checkAdvanceArpeggiatorForSplit(RIGHT);
}

byte arpTempoChoices[9] {
   24, // ArpFourth
   12, // ArpEighth
   8,  // ArpEighthTriplet
   6,  // ArpSixteenth
   TEMPO_ARP_SIXTEENTH_SWING, // ArpSixteenthSwing
   4,  // ArpSixteenthTriplet
   3,  // ArpThirtysecond
   2,  // ArpThirtysecondTriplet
   1   // ArpSixtyfourthTriplet
 };

inline void checkAdvanceArpeggiatorForSplit(byte split) {
  if (isArpeggiatorEnabled(split)) {

    byte combinedTempoIndex = constrain(Global.arpTempo + arpTempoDelta[split], 0, 8);
    byte combinedTempo = arpTempoChoices[combinedTempoIndex];

    if ((combinedTempo == TEMPO_ARP_SIXTEENTH_SWING && ((clock24PPQ % 12 == 0) || (clock24PPQ % 12 == 7))) ||  // we need to handle swing differently since it's irregular
        (combinedTempo != TEMPO_ARP_SIXTEENTH_SWING && (clock24PPQ % combinedTempo == 0 ))) {
      advanceArpeggiatorForSplit(split);
    }
  }
}

void advanceArpeggiatorForSplit(byte split) {
  signed char arpNote = -1;
  signed char arpChannel = -1;

  sendArpeggiatorStepMidiOff(split);

  // handle replayAll differently since it plays multiple notes simultaneously
  if (Global.arpDirection == ArpReplayAll) {
    if (noteTouchMapping[split].noteCount > 0) {

      // send all the note ons
      arpNote = noteTouchMapping[split].firstNote;
      arpChannel = noteTouchMapping[split].firstChannel;

      while (arpNote != -1) {

        NoteEntry* entry = noteTouchMapping[split].getNoteEntry(arpNote, arpChannel);
        if (entry == NULL) {
          arpNote = -1;
        }
        else {
          for (byte octave = 0; octave <= Global.arpOctave; ++octave) {
            // after the initial velocity, new velocity values are continuously being calculated simply based
            // on the Z data so that velocity can change during the arpeggiation
            TouchInfo* entry_cell = &cell(entry->getCol(), entry->getRow());
            if (entry_cell->touched == touchedCell) {
              entry_cell->velocity = calcPreferredVelocity(entry_cell->velocityZ);      
            }
            midiSendNoteOn(split, getOctaveNote(octave, arpNote), entry_cell->velocity, arpChannel);
          }

          arpNote = entry->getNextNote();
          arpChannel = entry->getNextChannel();
        }
      }

      lastArpNote[split] = noteTouchMapping[split].firstNote;
      lastArpChannel[split] = noteTouchMapping[split].firstChannel;
      lastArpStepOdd[split] = !lastArpStepOdd[split];
    }
    else {
      lastArpNote[split] = -1;
      lastArpChannel[split] = -1;
    }
  }
  // handle single note sequences
  else {
    if (noteTouchMapping[split].isAnyNotePressed()) {

      switch (Global.arpDirection) {

        // sequence steps upwards
        case ArpUp:
        {
          if (lastArpNote[split] == -1) {
            arpNote = noteTouchMapping[split].firstNote;
            arpChannel = noteTouchMapping[split].firstChannel;
          }
          else {
            NoteEntry* lastEntry = noteTouchMapping[split].getNoteEntry(lastArpNote[split], lastArpChannel[split]);
            if (lastEntry == NULL) {
              arpNote = -1;
            }
            else {
              arpNote = lastEntry->getNextNote();
              arpChannel = lastEntry->getNextChannel();
            }

            // start again from the beginning
            if (arpNote == -1) {
              arpNote = noteTouchMapping[split].firstNote;
              arpChannel = noteTouchMapping[split].firstChannel;
              if (++arpOctaveState[split] > Global.arpOctave) {
                arpOctaveState[split] = 0;
              }
            }
          }
          break;
        }

        // sequence steps downwards
        case ArpDown:
        {
          if (lastArpNote[split] == -1) {
            arpNote = noteTouchMapping[split].lastNote;
            arpChannel = noteTouchMapping[split].lastChannel;
          }
          else {
            NoteEntry* lastEntry = noteTouchMapping[split].getNoteEntry(lastArpNote[split], lastArpChannel[split]);
            if (lastEntry == NULL) {
              arpNote = -1;
            }
            else {
              arpNote = lastEntry->getPreviousNote();
              arpChannel = lastEntry->getPreviousChannel();
            }

            // start again from the end
            if (arpNote == -1) {
              arpNote = noteTouchMapping[split].lastNote;
              arpChannel = noteTouchMapping[split].lastChannel;
              if (++arpOctaveState[split] > Global.arpOctave) {
                arpOctaveState[split] = 0;
              }
            }
          }
          break;
        }

        // sequence steps alternativing upwards and downwards
        case ArpUpDown:
        {
          if (lastArpNote[split] == -1) {
            arpUpDownState[split] = ArpUp;
            arpNote = noteTouchMapping[split].firstNote;
            arpChannel = noteTouchMapping[split].firstChannel;
          }
          else {
            NoteEntry* lastEntry = noteTouchMapping[split].getNoteEntry(lastArpNote[split], lastArpChannel[split]);
            if (lastEntry == NULL) {
              arpNote = -1;
            }
            else {
              if (arpUpDownState[split] == ArpDown) {
                arpNote = lastEntry->getPreviousNote();
                arpChannel = lastEntry->getPreviousChannel();
              }
              else {
                arpNote = lastEntry->getNextNote();
                arpChannel = lastEntry->getNextChannel();
              }
            }

            // change directions
            if (arpNote == -1) {

              // handle the state where there's only one note active
              if (noteTouchMapping[split].firstNote == noteTouchMapping[split].lastNote &&
                  noteTouchMapping[split].firstChannel == noteTouchMapping[split].lastChannel) {

                arpNote = noteTouchMapping[split].firstNote;
                arpChannel = noteTouchMapping[split].firstChannel;

                if (Global.arpOctave != 0) {
                  if (arpUpDownState[split] == ArpUp) {
                    if (arpOctaveState[split] != Global.arpOctave) {
                      arpOctaveState[split]++;
                    }
                    else {
                      arpOctaveState[split]--;
                      arpUpDownState[split] = ArpDown;
                    }
                  }
                  else if (arpUpDownState[split] == ArpDown) {
                    if (arpOctaveState[split] != 0) {
                      arpOctaveState[split]--;
                    }
                    else {
                      arpOctaveState[split]++;
                      arpUpDownState[split] = ArpUp;
                    }
                  }
                }
              }
              // when there's no octave switching, wrap around without repeated notes
              // otherwise continue on the next octave if needed this follows the active direction and wrap around when reaching a boundary
              else {

                if (arpUpDownState[split] == ArpUp) {
                  if (arpOctaveState[split] != Global.arpOctave) {
                    arpNote = noteTouchMapping[split].firstNote;
                    arpChannel = noteTouchMapping[split].firstChannel;
                    arpOctaveState[split]++;
                  }
                  else {
                    arpUpDownState[split] = ArpDown;
                    
                    NoteEntry* entry = noteTouchMapping[split].getNoteEntry(noteTouchMapping[split].lastNote, noteTouchMapping[split].lastChannel);
                    if (entry == NULL) {
                      arpNote = -1;
                    }
                    else {
                      arpNote = entry->getPreviousNote();
                      arpChannel = entry->getPreviousChannel();
                    }
                  }
                }
                else if (arpUpDownState[split] == ArpDown) {
                  if (arpOctaveState[split] != 0) {
                    arpNote = noteTouchMapping[split].lastNote;
                    arpChannel = noteTouchMapping[split].lastChannel;
                    arpOctaveState[split]--;
                  }
                  else {
                    arpUpDownState[split] = ArpUp;

                    NoteEntry* entry = noteTouchMapping[split].getNoteEntry(noteTouchMapping[split].firstNote, noteTouchMapping[split].firstChannel);
                    if (entry == NULL) {
                      arpNote = -1;
                    }
                    else {
                      arpNote = entry->getNextNote();
                      arpChannel = entry->getNextChannel();
                    }
                  }
                }
              }
            }
          }
          break;
        }

        // sequence steps randomly
        case ArpRandom:
        {
          long pos = random(noteTouchMapping[split].noteCount);

          arpNote = noteTouchMapping[split].firstNote;
          arpChannel = noteTouchMapping[split].firstChannel;

          while (arpNote != -1 && pos-- != 0) {
            NoteEntry* entry = noteTouchMapping[split].getNoteEntry(arpNote, arpChannel);
            if (entry == NULL) {
              arpNote = -1;
            }
            else {
              arpNote = entry->getNextNote();
              arpChannel = entry->getNextChannel();
            }
          }

          if (Global.arpOctave) {
            arpOctaveState[split] = random(Global.arpOctave + 1);
          }
          break;
        }

        case ArpReplayAll:
          // handled above, at the beginning of the function
          break;
      }

      if (arpNote != -1) {
        // if this is the first step in a new sequence, this will be an odd step (starting at one)
        if (lastArpNote[split] == -1) {
          lastArpStepOdd[split] = true;
        }
        else {
          lastArpStepOdd[split] = !lastArpStepOdd[split];
        }

        // send the MIDI note
        NoteEntry* entry = noteTouchMapping[split].getNoteEntry(arpNote, arpChannel);
        if (entry == NULL) {
          arpNote = -1;
        }
        else {
          // after the initial velocity, new velocity values are continuously being calculated simply based
          // on the Z data so that velocity can change during the arpeggiation
          TouchInfo* entry_cell = &cell(entry->getCol(), entry->getRow());
          if (entry_cell->touched == touchedCell) {
            entry_cell->velocity = calcPreferredVelocity(entry_cell->velocityZ);      
          }
          midiSendNoteOn(split, getArpeggiatorNote(split, arpNote), entry_cell->velocity, arpChannel);
        }
      }

      // if the note that just ended was stale, remove it from the cell mapping, it's important to do this after 
      // we've processed the next note for the arp, otherwise next/previous note info will be wiped out.
      if(noteTouchMapping[split].isNoteStale(lastArpNote[split], lastArpChannel[split])) {
        noteTouchMapping[split].noteOff(lastArpNote[split], lastArpChannel[split]);
      }

      lastArpNote[split] = arpNote;
      lastArpChannel[split] = arpChannel;
    }
  }
}

inline boolean isArpeggiatorEnabled(byte split) {
  return Split[split].arpeggiator || isLowRowArpeggiatorPressed(split);
}
