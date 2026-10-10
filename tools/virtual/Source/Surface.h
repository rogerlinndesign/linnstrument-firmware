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
// The instrument itself: the black panel with its cherry edges, the grey pads with the LEDs
// glowing through them, the printed labels of the control column, and the mouse as a finger.
#pragma once

#include <JuceHeader.h>

#include <set>

#include "Engine.h"

class Surface : public juce::Component,
                private juce::Timer {
public:
  explicit Surface(Engine& engine);
  ~Surface() override;

  // width over height of the whole instrument for the current model
  float aspectRatio() const;

  void paint(juce::Graphics& g) override;
  void resized() override;
  void mouseDown(const juce::MouseEvent& e) override;
  void mouseDrag(const juce::MouseEvent& e) override;
  void mouseUp(const juce::MouseEvent& e) override;
  bool keyPressed(const juce::KeyPress& key) override;
  void focusGained(FocusChangeType) override;
  void focusLost(FocusChangeType) override;
  std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override;

  // what a screen reader hears for the pad that has the keyboard focus
  juce::String describeFocusedCell() const;

private:
  struct Cell {
    int col = -1, row = -1;
    float xFraction = 0, yFraction = 0;
    bool valid() const { return col >= 0; }
  };

  void timerCallback() override;
  void toggleFocusedCell();
  void moveFocus(int dCol, int dRow);
  Cell cellAt(juce::Point<float> position) const;
  juce::Rectangle<float> padBounds(int col, int row) const;
  juce::Colour ledColour(int color) const;

  Engine& engine;
  int columns = 26;
  float padSize = 30;
  float gap = 3;
  juce::Rectangle<float> padsArea;
  juce::Rectangle<float> panel;
  juce::Rectangle<float> steel;
  Cell dragCell;
  Cell focusCell;
  std::set<int> latched;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Surface)
};
