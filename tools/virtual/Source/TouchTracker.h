/*
Copyright 2026 Roger Linn Design (https://www.rogerlinndesign.com)

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/
#pragma once

#include <functional>
#include <map>
#include <set>

// Which cell each finger, the mouse or the keyboard holds, so that several fingers play at once.
// A finger sliding onto another cell presses that one before it lets go of the last, a cell stays
// down while any finger is still on it, and a latched cell stays down after its touch lifts until
// it's touched again. What that does to the instrument goes out through press, move and release.
class TouchTracker {
public:
  struct Cell {
    int col = -1, row = -1;
    float xFraction = 0, yFraction = 0;
    bool valid() const { return col >= 0; }
  };

  std::function<void(const Cell&)> press;
  std::function<void(const Cell&)> move;
  std::function<void(int col, int row)> release;
  // a finger moving into the next cell of its row, which on the instrument is a slide rather than a
  // new touch; returns false when it can't be done that way, and the move is a press and release
  std::function<bool(const Cell& from, const Cell& to)> slide;

  // source tells the fingers apart: JUCE's index of the mouse or touch
  void down(int source, const Cell& cell, bool latch);
  void drag(int source, const Cell& cell);
  void up(int source);
  // the keyboard's way of pressing a cell and letting it go again
  void toggle(const Cell& cell);
  bool isLatched(int col, int row) const { return latched.count(col * 8 + row) > 0; }

private:
  bool heldByAnother(int source, const Cell& cell) const;
  void lift(int source, const Cell& cell);

  std::map<int, Cell> drags;
  std::set<int> latched;
};
