/******** ls_noteTouchMapping: LinnStrument mapping between active notes and touched cells ********
This work is licensed under the Creative Commons Attribution-ShareAlike 3.0 Unported License.
To view a copy of this license, visit http://creativecommons.org/licenses/by-sa/3.0/
or send a letter to Creative Commons, PO Box 1866, Mountain View, CA 94042, USA.
***************************************************************************************************
These mappings keep track of the MIDI notes and channels that are active and map them to the cells
that initiated the notes. Additionally, the notes also keep track of the order in which identical
notes were pressed but with a different channel.
This entire structure is intended to be used by the arpeggiator, requiring a minimal amount of
iteration to constitute the arpeggiated sequence.
**************************************************************************************************/

void resetAllTouches() {
  midiSendNoteOffForAllTouches(LEFT);
  midiSendNoteOffForAllTouches(RIGHT);
  noteTouchMapping[LEFT].initialize();
  noteTouchMapping[RIGHT].initialize();
}

boolean validNoteNumAndChannel(signed char noteNum, signed char noteChannel) {
  if (noteNum < 0 || noteNum > 127 || noteChannel < 1 || noteChannel > 16) {
    return false;
  }

  return true;
}

inline byte combineColRow(byte column, byte row) {
  return (column & B00011111) | (row & B00000111) << 5;
}

inline boolean NoteEntry::hasColRow(byte column, byte row) {
  return colRow == combineColRow(column, row);
}

inline void NoteEntry::setColRow(byte column, byte row) {
  colRow = combineColRow(column, row);
}

inline byte NoteEntry::getCol() {
  return colRow & B00011111;
}

inline byte NoteEntry::getRow() {
  return (colRow & B11100000) >> 5;
}

inline byte NoteEntry::getNextNote() {
  return nextNote;
}

inline byte NoteEntry::getPreviousNote() {
  return previousNote;
}

inline void NoteEntry::setNextChannel(byte noteChannel) {
  nextPreviousChannel = (nextPreviousChannel & B00001111) | ((noteChannel - 1) & B00001111) << 4;
}

inline byte NoteEntry::getNextChannel() {
  return ((nextPreviousChannel & B11110000) >> 4) + 1;
}

inline void NoteEntry::setPreviousChannel(byte noteChannel) {
  nextPreviousChannel = (nextPreviousChannel & B11110000) | ((noteChannel - 1) & B00001111);
}

inline byte NoteEntry::getPreviousChannel() {
  return (nextPreviousChannel & B00001111) + 1;
}

void NoteTouchMapping::initialize() {
  noteCount = 0;
  firstNote = -1;
  firstChannel = -1;
  lastNote = -1;
  lastChannel = -1;
  clearStaleNote();

  for (byte c = 0; c < 16; ++c) {
    for (byte n = 0; n < 128; ++n) {
      mapping[n][c].colRow = 0;
      mapping[n][c].nextNote = -1;
      mapping[n][c].previousNote = -1;
      mapping[n][c].nextPreviousChannel = 0;
    }
    performContinuousTasks();
  }
  for (byte c = 0; c < 16; ++c) {
    musicalTouchCount[c] = 0;
  }
  performContinuousTasks();
}

void NoteTouchMapping::releaseLatched(byte split) {
  signed char entryNote = firstNote;
  signed char entryChannel = firstChannel;
  while (entryNote != -1) {
    NoteEntry& entry = mapping[entryNote][entryChannel-1];
    signed char nextNote = entry.getNextNote();
    signed char nextChannel = entry.getNextChannel();

    TouchInfo* entry_cell = &cell(entry.getCol(), entry.getRow());
    if (entry_cell->touched != touchedCell) {
      if (isArpeggiatorEnabled(split)) {
        handleArpeggiatorNoteOff(split, entryNote, entryChannel, false);
      }
      else {
        midiSendNoteOffWithVelocity(split, entryNote, entry_cell->velocity, entryChannel);
      }
      noteOff(entryNote, entryChannel);
    }

    entryNote = nextNote;
    entryChannel = nextChannel;
  }
}

inline byte NoteTouchMapping::getMusicalTouchCount(signed char noteChannel) {
  if (noteChannel < 1 || noteChannel > 16) {
    return 0;
  }

  return musicalTouchCount[noteChannel - 1];
}

inline NoteEntry* NoteTouchMapping::getNoteEntry(signed char noteNum, signed char noteChannel) {
  if (!validNoteNumAndChannel(noteNum, noteChannel)) {
    return NULL;
  }

  return &mapping[noteNum][noteChannel - 1];
}

boolean NoteTouchMapping::hasTouch(signed char noteNum, signed char noteChannel) {
  if (!validNoteNumAndChannel(noteNum, noteChannel)) {
    return false;
  }

  return mapping[noteNum][noteChannel - 1].colRow != 0;
}

void NoteTouchMapping::noteOn(signed char noteNum, signed char noteChannel, byte col, byte row) {
  if (!validNoteNumAndChannel(noteNum, noteChannel)) {
    return;
  }

  // offset the channel for a 0-based arrays
  signed char channel = noteChannel - 1;

  musicalTouchCount[channel] += 1;

  if (mapping[noteNum][channel].colRow == 0) {
    noteCount++;

    // no notes are in the chain yet, add this one as the first note
    if (-1 == firstNote) {
      firstNote = noteNum;
      firstChannel = noteChannel;
      lastNote = noteNum;
      lastChannel = noteChannel;
      mapping[noteNum][channel].nextNote = -1;
      mapping[noteNum][channel].previousNote = -1;
      mapping[noteNum][channel].nextPreviousChannel = 0;
    }
    else {
      signed char entryNote = firstNote;
      signed char entryChannel = firstChannel;
      while (entryNote != -1) {
        NoteEntry& entry = mapping[entryNote][entryChannel - 1];

        // the current note entry comes after the new note, we'll insert the new note
        // right before the current note entry
        // this solves two scenarios, first it will add the new note as a last channel in the chain
        // if this note already exists (ie. earlier presses of the same note get precedence),
        // and it will slide in a new note entry between existing ones if the note didn't exist before
        if (entryNote > noteNum) {
          signed char prevNote = entry.previousNote;
          signed char prevChannel = entry.getPreviousChannel();

          mapping[noteNum][channel].nextNote = entryNote;
          mapping[noteNum][channel].setNextChannel(entryChannel);
          mapping[noteNum][channel].previousNote = entry.previousNote;
          mapping[noteNum][channel].setPreviousChannel(entry.getPreviousChannel());
          entry.previousNote = noteNum;
          entry.setPreviousChannel(noteChannel);

          // handle the case where the current note entry is the first in the chain
          if (-1 == prevNote) {
            firstNote = noteNum;
            firstChannel = noteChannel;
          }
          else {
            mapping[prevNote][prevChannel - 1].nextNote = noteNum;
            mapping[prevNote][prevChannel - 1].setNextChannel(noteChannel);
          }

          break;
        }

        // this is the last note in the chain, but it still comes before the new one,
        // insert the new note after this one
        if (entry.nextNote == -1) {
          lastNote = noteNum;
          lastChannel = noteChannel;
          mapping[noteNum][channel].nextNote = -1;
          mapping[noteNum][channel].setNextChannel(1);
          mapping[noteNum][channel].previousNote = entryNote;
          mapping[noteNum][channel].setPreviousChannel(entryChannel);
          entry.nextNote = noteNum;
          entry.setNextChannel(noteChannel);
          break;
        }

        entryNote = entry.nextNote;
        entryChannel = entry.getNextChannel();
      }
    }
  }

  // this could be a stale note where the note was released and then pressed again
  // before the next arp advance, clear the state if so.
  if (isNoteStale(noteNum, noteChannel)) {
    clearStaleNote();
  }

  mapping[noteNum][channel].setColRow(col, row);

  debugNoteChain();
}

void NoteTouchMapping::noteOff(signed char noteNum, signed char noteChannel) {
  if (!validNoteNumAndChannel(noteNum, noteChannel)) {
    return;
  }

  // offset the channel for a 0-based arrays
  signed char channel = noteChannel - 1;

  musicalTouchCount[channel] -= 1;

  if (hasTouch(noteNum, noteChannel)) {
    noteCount--;

    // if this is the first note that is active, point the first note/channel
    // markers to the next note entry in the chain and adapt this entry's
    // previous information
    if (firstNote == noteNum && firstChannel == noteChannel) {
      firstNote = mapping[noteNum][channel].nextNote;
      firstChannel = mapping[noteNum][channel].getNextChannel();
      if (firstNote == -1) {
        lastNote = -1;
        lastChannel = -1;
      }
      else {
        mapping[firstNote][firstChannel - 1].previousNote = -1;
        mapping[firstNote][firstChannel - 1].setPreviousChannel(1);
      }
    }
    // simply remove this note entry from the chain by pointing the previous and
    // next note entries to each-other
    else {
      signed char prevNote = mapping[noteNum][channel].previousNote;
      signed char prevChannel = mapping[noteNum][channel].getPreviousChannel();
      signed char nxtNote = mapping[noteNum][channel].nextNote;
      signed char nxtChannel = mapping[noteNum][channel].getNextChannel();
      mapping[prevNote][prevChannel - 1].nextNote = nxtNote;
      mapping[prevNote][prevChannel - 1].setNextChannel(nxtChannel);
      if (nxtNote == -1) {
        lastNote = prevNote;
        lastChannel = prevChannel;
      }
      else {
        mapping[nxtNote][nxtChannel - 1].previousNote = prevNote;
        mapping[nxtNote][nxtChannel - 1].setPreviousChannel(prevChannel);
      }
    }

    // reset all the data of this note entry
    mapping[noteNum][channel].colRow = 0;
    mapping[noteNum][channel].nextNote = -1;
    mapping[noteNum][channel].previousNote = -1;
    mapping[noteNum][channel].nextPreviousChannel = 0;

    if (isNoteStale(noteNum, noteChannel)) {
      clearStaleNote();
    }
  }

  debugNoteChain();
}

void NoteTouchMapping::changeCell(signed char noteNum, signed char noteChannel, byte col, byte row) {
  if (!validNoteNumAndChannel(noteNum, noteChannel)) {
    return;
  }

  // offset the channel for a 0-based arrays
  signed char channel = noteChannel - 1;

  if (mapping[noteNum][channel].colRow != 0) {
    mapping[noteNum][channel].setColRow(col, row);
  }
}

boolean NoteTouchMapping::isAnyNotePressed() {
  // if there's a stale note, compensate the noteCount check by ignoring that stale note
  if (hasStaleNote()) {
    return noteCount > 1;
  }

  return noteCount > 0;
}

void NoteTouchMapping::clearStaleNote() {
  staleNote = -1;
  staleChannel = 0;
}

void NoteTouchMapping::setStaleNote(signed char noteNum, signed char noteChannel) {
  staleNote = noteNum;
  staleChannel = noteChannel;
}

boolean NoteTouchMapping::isNoteStale(signed char noteNum, signed char noteChannel) {
  return validNoteNumAndChannel(noteNum, noteChannel) && (staleNote == noteNum) && (staleChannel == noteChannel);
}

boolean NoteTouchMapping::hasStaleNote() {
  // when we clear the stale note vars, we set them to an invalid note/channel. 
  // checking for there's a stale note is simply checking for validity.
  return validNoteNumAndChannel(staleNote, staleChannel);
}

void NoteTouchMapping::debugNoteChain() {
#ifdef DEBUG_ENABLED
  if (Device.serialMode && (debugLevel >= 1)) {
    DEBUGPRINT((1,"noteTouchMapping"));
    DEBUGPRINT((1,"\n"));
    DEBUGPRINT((1," noteCount="));DEBUGPRINT((1,noteCount));
    DEBUGPRINT((1," firstNote="));DEBUGPRINT((1,firstNote));
    DEBUGPRINT((1," firstChannel="));DEBUGPRINT((1,firstChannel));
    DEBUGPRINT((1,"\n"));
    signed char entryNote = firstNote;
    signed char entryChannel = firstChannel;
    while (entryNote != -1) {
      NoteEntry& entry = mapping[entryNote][entryChannel];

      DEBUGPRINT((1," note="));DEBUGPRINT((1,entryNote));
      DEBUGPRINT((1," channel="));DEBUGPRINT((1,entryChannel));
      DEBUGPRINT((1," col="));DEBUGPRINT((1,entry.getCol()));
      DEBUGPRINT((1," row="));DEBUGPRINT((1,entry.getRow()));
      DEBUGPRINT((1," previous="));DEBUGPRINT((1,entry.previousNote));DEBUGPRINT((1,","));DEBUGPRINT((1,entry.getPreviousChannel()));
      DEBUGPRINT((1," next="));DEBUGPRINT((1,entry.nextNote));DEBUGPRINT((1,","));DEBUGPRINT((1,entry.getNextChannel()));
      DEBUGPRINT((1,"\n"));
      entryNote = entry.nextNote;
      entryChannel = entry.getNextChannel();
    }  
    DEBUGPRINT((1," lastNote="));DEBUGPRINT((1,lastNote));
    DEBUGPRINT((1," lastChannel="));DEBUGPRINT((1,lastChannel));
    DEBUGPRINT((1,"\n"));
  }
#endif
}
