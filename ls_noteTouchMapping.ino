/******** ls_noteTouchMapping: LinnStrument mapping between active notes and touched cells ********
Copyright 2023 Roger Linn Design (https://www.rogerlinndesign.com)

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
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
  noteTouchMapping[LEFT].initialize(LEFT);
  noteTouchMapping[RIGHT].initialize(RIGHT);
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

inline boolean NoteEntry::hasTouch() {
  return colRow != 0;
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

void NoteTouchMapping::initialize(byte mappedSplit) {
  split = mappedSplit;
  noteCount = 0;
  firstNote = -1;
  firstChannel = -1;
  lastNote = -1;
  lastChannel = -1;
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

void NoteTouchMapping::releaseLatched() {
  DEBUGPRINT((1,"releaseLatched"));
  DEBUGPRINT((1,"\n"));

  signed char entryNote = firstNote;
  signed char entryChannel = firstChannel;
  while (entryNote != -1) {
    NoteEntry& entry = mapping[entryNote][entryChannel-1];
    signed char nextNote = entry.getNextNote();
    signed char nextChannel = entry.getNextChannel();

    TouchInfo* entry_cell = &cell(entry.getCol(), entry.getRow());
    if (entry_cell->touched != touchedCell || entry_cell->note != entryNote || entry_cell->channel != entryChannel) {
      if (isArpeggiatorEnabled(split)) {
        handleArpeggiatorNoteOff(split, entryNote, entryChannel);
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

  if (!mapping[noteNum][channel].hasTouch()) {
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
          mapping[noteNum][channel].setNextChannel(0);
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

  mapping[noteNum][channel].setColRow(col, row);

  // this note on is the same as the currently playing note, restore the arp step to this note
  if (!isSwitchLegatoPressed(split) && !isSwitchLatchPressed(split)) {
    if (playingArpNote[split] == noteNum && playingArpChannel[split] == noteChannel) {
      stepArpNote[split] = noteNum;
      stepArpChannel[split] = noteChannel;
    }
  }

  DEBUGPRINT((1,"noteOn"));
  DEBUGPRINT((1," noteNum="));DEBUGPRINT((1,(int)noteNum));
  DEBUGPRINT((1," noteChannel="));DEBUGPRINT((1,(int)noteChannel));
  DEBUGPRINT((1,"\n"));

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
        mapping[firstNote][firstChannel - 1].setPreviousChannel(0);
      }
    }
    // simply remove this note entry from the chain by pointing the previous and
    // next note entries to each-other
    else {
      signed char prevNote = mapping[noteNum][channel].previousNote;
      signed char prevChannel = mapping[noteNum][channel].getPreviousChannel();
      signed char nxtNote = mapping[noteNum][channel].nextNote;
      signed char nxtChannel = mapping[noteNum][channel].getNextChannel();
      
      // verify if the removed note entry is the current active arp step, if that's
      // the case, update the step so that linked chain of notes is not interrupted
      if (!isSwitchLegatoPressed(split) && !isSwitchLatchPressed(split)) {
        if (stepArpNote[split] == noteNum && stepArpChannel[split] == noteChannel) {
          // when the arp is going up, update the step with the previous note entry,
          // however when it's in up/down mode, don't do this when the last note is reached
          // otherwise the arp will skip a note
          if (Global.arpDirection == ArpUp || (Global.arpDirection == ArpUpDown && arpUpDownState[split] == ArpUp && (noteNum != lastNote || noteChannel != lastChannel))) {
            stepArpNote[split] = prevNote;
            stepArpChannel[split] = prevChannel;
          }
          // when the arp is going down, update the step with the next note entry
          else if (Global.arpDirection == ArpDown || (Global.arpDirection == ArpUpDown && arpUpDownState[split] == ArpDown)) {
            stepArpNote[split] = nxtNote;
            stepArpChannel[split] = nxtChannel;
          }
        }
      }

      // update the next and previous entries
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
  }

  DEBUGPRINT((1,"noteOff"));
  DEBUGPRINT((1," noteNum="));DEBUGPRINT((1,(int)noteNum));
  DEBUGPRINT((1," noteChannel="));DEBUGPRINT((1,(int)noteChannel));
  DEBUGPRINT((1,"\n"));

  debugNoteChain();
}

void NoteTouchMapping::changeCell(signed char noteNum, signed char noteChannel, byte col, byte row) {
  if (!validNoteNumAndChannel(noteNum, noteChannel)) {
    return;
  }

  // offset the channel for a 0-based arrays
  signed char channel = noteChannel - 1;

  if (mapping[noteNum][channel].hasTouch()) {
    mapping[noteNum][channel].setColRow(col, row);
  }
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
      NoteEntry& entry = mapping[entryNote][entryChannel-1];

      DEBUGPRINT((1," note="));DEBUGPRINT((1,entryNote));
      DEBUGPRINT((1," channel="));DEBUGPRINT((1,entryChannel));
      DEBUGPRINT((1," col="));DEBUGPRINT((1,entry.getCol()));
      DEBUGPRINT((1," row="));DEBUGPRINT((1,entry.getRow()));
      DEBUGPRINT((1," previous="));DEBUGPRINT((1,(int)entry.previousNote));DEBUGPRINT((1,","));DEBUGPRINT((1,entry.getPreviousChannel()));
      DEBUGPRINT((1," next="));DEBUGPRINT((1,(int)entry.nextNote));DEBUGPRINT((1,","));DEBUGPRINT((1,entry.getNextChannel()));
      DEBUGPRINT((1,"\n"));
      if (entry.nextNote == entryNote && entry.getNextChannel() == entryChannel) {
        DEBUGPRINT((1," INFINITE LOOP\n"));
        break;
      }
      entryNote = entry.nextNote;
      entryChannel = entry.getNextChannel();
    }  
    DEBUGPRINT((1," lastNote="));DEBUGPRINT((1,lastNote));
    DEBUGPRINT((1," lastChannel="));DEBUGPRINT((1,lastChannel));
    DEBUGPRINT((1,"\n"));
  }
#endif
}
