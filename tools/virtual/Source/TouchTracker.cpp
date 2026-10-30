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
#include "TouchTracker.h"

#include <cstdlib>

void TouchTracker::down(int source, const Cell& cell, bool latch) {
  int key = cell.col * 8 + cell.row;
  if (latched.count(key)) {
    latched.erase(key);
    if (!heldByAnother(source, cell)) release(cell.col, cell.row);
    drags.erase(source);
    return;
  }
  if (latch) latched.insert(key);
  press(cell);
  drags[source] = cell;
}

void TouchTracker::drag(int source, const Cell& cell) {
  auto it = drags.find(source);
  if (it == drags.end()) return;
  Cell& from = it->second;
  bool neighbour = cell.row == from.row && std::abs(cell.col - from.col) == 1;
  if (neighbour && slide && !heldByAnother(source, from) && !isLatched(from.col, from.row) && slide(from, cell)) {
    from = cell;
  }
  else if (cell.col != from.col || cell.row != from.row) {
    press(cell);
    Cell left = from;
    from = cell;
    latched.erase(left.col * 8 + left.row);
    lift(source, left);
  }
  else {
    move(cell);
  }
}

void TouchTracker::up(int source) {
  auto it = drags.find(source);
  if (it == drags.end()) return;
  Cell cell = it->second;
  drags.erase(it);
  if (!isLatched(cell.col, cell.row)) lift(-1, cell);
}

void TouchTracker::toggle(const Cell& cell) {
  int key = cell.col * 8 + cell.row;
  if (latched.count(key)) {
    latched.erase(key);
    // the keyboard is no finger, so any finger on the cell keeps it down
    if (!heldByAnother(-1, cell)) release(cell.col, cell.row);
  }
  else {
    latched.insert(key);
    press(cell);
  }
}

bool TouchTracker::heldByAnother(int source, const Cell& cell) const {
  for (auto& [other, held] : drags) {
    if (other != source && held.col == cell.col && held.row == cell.row) return true;
  }
  return false;
}

// a cell stays down while another finger is still on it
void TouchTracker::lift(int source, const Cell& cell) {
  if (!heldByAnother(source, cell)) release(cell.col, cell.row);
}
