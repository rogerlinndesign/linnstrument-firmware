/******** ls_noteTouchMapping: LinnStrument mapping between active notes and touched cells ********
This work is licensed under the Creative Commons Attribution-ShareAlike 3.0 Unported License.
To view a copy of this license, visit http://creativecommons.org/licenses/by-sa/3.0/
or send a letter to Creative Commons, PO Box 1866, Mountain View, CA 94042, USA.
***************************************************************************************************
These mappings keep track of the MIDI notes and channels that are active and map them to the cells
that initiated the notes Additionally, the notes also keep track of the order in which identical
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

boolean validNoteNumAndChannel(signed char noteNum, signed char channel) {
  if (noteNum < 0 || noteNum > 127 || channel < 0 || channel > 15) {
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

inline void NoteEntry::setNextChannel(byte channel) {
  nextPreviousChannel = (nextPreviousChannel & B00001111) | ((channel - 1) & B00001111) << 4;
}

inline byte NoteEntry::getNextChannel() {
  return ((nextPreviousChannel & B11110000) >> 4) + 1;
}

inline void NoteEntry::setPreviousChannel(byte channel) {
  nextPreviousChannel = (nextPreviousChannel & B11110000) | ((channel - 1) & B00001111);
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
  for (byte n = 0; n < 128; ++n) {
    for (byte c = 0; c < 16; ++c) {
      mapping[n][c].colRow = 0;
      mapping[n][c].nextNote = -1;
      mapping[n][c].previousNote = -1;
      mapping[n][c].nextPreviousChannel = 0;
    }
  }
  for (byte c = 0; c < 16; ++c) {
    musicalTouchCount[c] = 0;
  }
}

boolean NoteTouchMapping::hasTouch(signed char noteNum, signed char channel) {
  if (!validNoteNumAndChannel(noteNum, channel)) {
    return false;
  }

  return mapping[noteNum][channel].colRow != 0;
}

void NoteTouchMapping::noteOn(signed char noteNum, signed char channel, byte col, byte row) {
  if (!validNoteNumAndChannel(noteNum, channel)) {
    return;
  }

  musicalTouchCount[channel] += 1;

  if (mapping[noteNum][channel].colRow == 0) {
    noteCount++;

    // no notes are in the chain yet, add this one as the first note
    if (-1 == firstNote) {
      firstNote = noteNum;
      firstChannel = channel;
      lastNote = noteNum;
      lastChannel = channel;
      mapping[noteNum][channel].nextNote = -1;
      mapping[noteNum][channel].previousNote = -1;
      mapping[noteNum][channel].nextPreviousChannel = 0;
    }
    else {
      signed char noteEntry = firstNote;
      signed char noteChannel = firstChannel;
      while (noteEntry != -1) {
        NoteEntry& entry = mapping[noteEntry][noteChannel];

        // the current note entry comes after the new note, we'll insert the new note
        // right before the current note entry
        // this solves two scenarios, first it will add the new note as a last channel in the chain
        // if this note already exists (ie. earlier presses of the same note get precedence),
        // and it will slide in a new note entry between existing ones if the note didn't exist before
        if (noteEntry > noteNum) {
          signed char prevNote = entry.previousNote;
          signed char prevChannel = entry.getPreviousChannel();

          mapping[noteNum][channel].nextNote = noteEntry;
          mapping[noteNum][channel].setNextChannel(noteChannel);
          mapping[noteNum][channel].previousNote = entry.previousNote;
          mapping[noteNum][channel].setPreviousChannel(entry.getPreviousChannel());
          entry.previousNote = noteNum;
          entry.setPreviousChannel(channel);

          // handle the case where the current note entry is the first in the chain
          if (-1 == prevNote) {
            firstNote = noteNum;
            firstChannel = channel;
          }
          else {
            mapping[prevNote][prevChannel].nextNote = noteNum;
            mapping[prevNote][prevChannel].setNextChannel(channel);
          }

          break;
        }

        // this is the last note in the chain, but it still comes before the new one,
        // insert the new note after this one
        if (entry.nextNote == -1) {
          lastNote = noteNum;
          lastChannel = channel;
          mapping[noteNum][channel].nextNote = -1;
          mapping[noteNum][channel].setNextChannel(1);
          mapping[noteNum][channel].previousNote = noteEntry;
          mapping[noteNum][channel].setPreviousChannel(noteChannel);
          entry.nextNote = noteNum;
          entry.setNextChannel(channel);
          break;
        }

        noteEntry = entry.nextNote;
        noteChannel = entry.getNextChannel();
      }
    }
  }

  mapping[noteNum][channel].setColRow(col, row);

  debugNoteChain();
}

void NoteTouchMapping::noteOff(signed char noteNum, signed char channel) {
  if (!validNoteNumAndChannel(noteNum, channel)) {
    return;
  }

  musicalTouchCount[channel] -= 1;

  if (hasTouch(noteNum, channel)) {
    noteCount--;

    // if this is the first note that is active, point the first note/channel
    // markers to the next note entry in the chain and adapt this entry's
    // previous information
    if (firstNote == noteNum && firstChannel == channel) {
      firstNote = mapping[noteNum][channel].nextNote;
      firstChannel = mapping[noteNum][channel].getNextChannel();
      if (firstNote == -1) {
        lastNote = -1;
        lastChannel = -1;
      }
      else {
        mapping[firstNote][firstChannel].previousNote = -1;
        mapping[firstNote][firstChannel].setPreviousChannel(1);
      }
    }
    // simply remove this note entry from the chain by pointing the previous and
    // next note entries to each-other
    else {
      signed char prevNote = mapping[noteNum][channel].previousNote;
      signed char prevChannel = mapping[noteNum][channel].getPreviousChannel();
      signed char nxtNote = mapping[noteNum][channel].nextNote;
      signed char nxtChannel = mapping[noteNum][channel].getNextChannel();
      mapping[prevNote][prevChannel].nextNote = nxtNote;
      mapping[prevNote][prevChannel].setNextChannel(nxtChannel);
      if (nxtNote == -1) {
        lastNote = prevNote;
        lastChannel = prevChannel;
      }
      else {
        mapping[nxtNote][nxtChannel].previousNote = prevNote;
        mapping[nxtNote][nxtChannel].setPreviousChannel(prevChannel);
      }
    }

    // reset all the data of this note entry
    mapping[noteNum][channel].colRow = 0;
    mapping[noteNum][channel].nextNote = -1;
    mapping[noteNum][channel].previousNote = -1;
    mapping[noteNum][channel].nextPreviousChannel = 0;
  }

  debugNoteChain();
}

void NoteTouchMapping::changeCell(signed char noteNum, signed char channel, byte col, byte row) {
  if (!validNoteNumAndChannel(noteNum, channel)) {
    return;
  }

  if (mapping[noteNum][channel].colRow != 0) {
    mapping[noteNum][channel].setColRow(col, row);
  }
}

void NoteTouchMapping::debugNoteChain() {
#ifdef DEBUG_ENABLED
  if (Global.serialMode && (debugLevel >= 1)) {
    DEBUGPRINT((1,"noteTouchMapping"));
    DEBUGPRINT((1,"\n"));
    DEBUGPRINT((1," noteCount="));DEBUGPRINT((1,noteCount));
    DEBUGPRINT((1," firstNote="));DEBUGPRINT((1,firstNote));
    DEBUGPRINT((1," firstChannel="));DEBUGPRINT((1,firstChannel));
    DEBUGPRINT((1,"\n"));
    signed char noteEntry = firstNote;
    signed char noteChannel = firstChannel;
    while (noteEntry != -1) {
      NoteEntry& entry = mapping[noteEntry][noteChannel];

      DEBUGPRINT((1," note="));DEBUGPRINT((1,noteEntry));
      DEBUGPRINT((1," channel="));DEBUGPRINT((1,noteChannel));
      DEBUGPRINT((1," col="));DEBUGPRINT((1,entry.getCol()));
      DEBUGPRINT((1," row="));DEBUGPRINT((1,entry.getRow()));
      DEBUGPRINT((1," previous="));DEBUGPRINT((1,entry.previousNote));DEBUGPRINT((1,","));DEBUGPRINT((1,entry.getPreviousChannel()));
      DEBUGPRINT((1," next="));DEBUGPRINT((1,entry.nextNote));DEBUGPRINT((1,","));DEBUGPRINT((1,entry.getNextChannel()));
      DEBUGPRINT((1,"\n"));
      noteEntry = entry.nextNote;
      noteChannel = entry.getNextChannel();
    }  
    DEBUGPRINT((1," lastNote="));DEBUGPRINT((1,lastNote));
    DEBUGPRINT((1," lastChannel="));DEBUGPRINT((1,lastChannel));
    DEBUGPRINT((1,"\n"));
  }
#endif
}
