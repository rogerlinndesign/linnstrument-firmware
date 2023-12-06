/********************** ls_channelbucket: LinnStrument ChannelBucket class ************************
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
Handles the handing out of MIDI channels from a bucket of allowed channel numbers.

The available channels are added to the bucket in the beginning.

When a note needs a new channel, it just takes it. The channel will move to the bottom of the
bucket and will be reused when all the other channels have been taken also. This makes it
possible for the same channel number to be in use by different note, depending on the number of
channels that were added to the bucket and the polyphony of the notes that are being played.

At the same time, the bucket keeps track of which channels are taken and which channels are
released and free from any touched. This is achieved by dividing the bucket in two sections, an
upper section with all the released channels and a bottom section with all the channels that are
already taken.

When a note is released, it should release its channel. If the channel is still in use by another
note since the polyphony was exceeded, the channel will move to the bottom of the taken section.
If no other notes are using the channel, the channel will be moved to the bottom of the released
section, somewhere in the middle of the whole bucket.

This whole approach ensures that channels are reused as late as possible, while attempting to
prevent handing out the same channel for multiple notes. Postponing the reuse of a channel as much
as possible is important for sounds that have long releases.
**************************************************************************************************/

#ifndef CHANNELBUCKET_H_
#define CHANNELBUCKET_H_

#include <Arduino.h>

class ChannelBucket {
public:
  ChannelBucket() {
    clear();
  }

  ~ChannelBucket() {}

  void add(byte channel) {
    // we can't add a MIDI channel that exceeds 16
    if (channel > 16) return;

    // offset the channel for a 0-based array
    channel -= 1;

    // don't add a channel several times
    if (next_[channel] != -1) return;

    // this is the first channel to be added to the bucket
    if (-1 == top_) {
      top_ = channel;
      previous_[channel] = channel;
      next_[channel] = channel;
      taken_[channel] = 0;
      bottomReleased_ = channel;
    }
    // add the channel to right after the current bottom-most released channel
    // and make the new channel the bottom-most released one
    else {
      previous_[channel] = bottomReleased_;
      previous_[next_[bottomReleased_]] = channel;

      next_[channel] = next_[bottomReleased_];
      next_[bottomReleased_] = channel;

      taken_[channel] = 0;
      bottomReleased_ = channel;
    }
  }

  byte take() {
    // return an invalid channel if none have been added
    if (-1 == top_) return 0;

    // get the channel at the top of the bucket
    byte channel = top_;

    // sink the channel to the bottom of the entire bucket, also crossing
    // the release/taken marker. Essentially the channel goes to the bottom
    // of the taken section. Since the channel was already at the top of the
    // bucket, this is very simply done by adjusting the top marker to the
    // next channel available
    top_ = next_[channel];

    // indicate that the channel was taken once more
    taken_[channel]++;

    // if this channel was the last of the released section, indicate that
    // the released marker has become invalid
    if (channel == bottomReleased_) {
      bottomReleased_ = -1;
    }

    // adjust for 1-base channel numbers
    return channel+1;
  }

  void release(byte channel) {
    // we can't release a MIDI channel that exceeds 16, nor can we work with an empty bucket
    // we expect this channel to also be already in the bucket
    if (channel > 16 || -1 == top_ || -1 == next_[channel-1]) return;

    // offset the channel for a 0-based array
    channel -= 1;

    // indicate that the channel was taken one less time
    taken_[channel]--;

    // if the channel is still taken, ensure it goes to the bottom of the taken section
    if (taken_[channel] > 0) {
      extremize(channel);
      top_ = next_[channel];
    }
    // if this release was the one using the channel, ensure that the channel goes at the bottom of the released section
    else {
      // if the released section doesn't exist anymore (all channels were taken),
      // put the channel at the extremes of the bucket and mark it as being the new
      // top channel
      if (bottomReleased_ == -1) {
        extremize(channel);
        top_ = channel;
      }
      // put the channel at the bottom of the released section
      else {
        // however, if the channel happens to be at the top of the
        // taken section (right after the released section), we don't need to do anything
        // besides readjusting the marker of the released section
        if (next_[bottomReleased_] != channel) {

          // determine the edges of both the released and the taken sections
          int releasedEdge = bottomReleased_;
          int takenEdge = next_[bottomReleased_];

          // extract the channel from the bucket so that nothing is pointing
          // towards it anymore
          extract(channel);

          // re-insert the channel between the current released and taken sections
          previous_[channel] = releasedEdge;
          next_[releasedEdge] = channel;

          previous_[takenEdge] = channel;
          next_[channel] = takenEdge;
        }
      }

      // this channel is the last released one now
      bottomReleased_ = channel;
    }
  }

  // remove all channels from the bucket
  void clear() {
    top_ = -1;
    for (int ch = 0; ch < 16; ++ch) {
      previous_[ch] = -1;
      next_[ch] = -1;
      taken_[ch] = 0;
    }
  }

private:

  void extract(byte channel) {
    if (next_[channel] != -1) {
      next_[previous_[channel]] = next_[channel];
      previous_[next_[channel]] = previous_[channel];

      previous_[channel] = -1;
      next_[channel] = -1;
    }
  }

  void extremize(byte channel) {
    int bottom = previous_[top_];
    if (bottom == channel) {
      bottom = previous_[channel];
    }
    int top = top_;
    if (top == channel) {
      top = next_[channel];
    }

    extract(channel);

    previous_[channel] = bottom;
    next_[bottom] = channel;

    previous_[top] = channel;
    next_[channel] = top;
  }

  void debugBucket() {
    Serial.print("top="); Serial.print(top_);
    Serial.print(" bottomReleased="); Serial.print(bottomReleased_);
    for (int ch = 0; ch < 16; ++ch) {
      Serial.print("\nchannel="); Serial.print(ch);
      Serial.print(" previous="); Serial.print(previous_[ch]);
      Serial.print(" next="); Serial.print(next_[ch]);
      Serial.print(" taken="); Serial.print(taken_[ch]);
    }
    Serial.print("\n");
  }

  int top_;            // the channel number of the top one in the bucket
  int previous_[16];   // the channel number of the previous channel in the bucket for each individual channel
  int next_[16];       // the channel number of the next channel in the bucket for each individual channel
  byte taken_[16];     // counts how many times each channel is still taken
  int bottomReleased_; // marks the bottom of the released section
};

#endif
