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

#include <functional>
#include <map>
#include <set>

#include "Engine.h"
#include "TouchTracker.h"

class Surface : public juce::Component,
                private juce::Timer {
public:
  explicit Surface(Engine& engine);
  ~Surface() override;

  // width over height of the whole instrument for the current model
  float aspectRatio() const;

  // the font of the control switches' printed labels
  static juce::Font labelFontOfHeight(float height);
  // a cell a finger pressed, after the engine has it
  std::function<void(int col, int row)> onPress;

  // the firmware the surface shows, its engine's
  firmware::Instance& firmware() const { return engine.firmware(); }

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

  // the colour a firmware LED colour number lights a pad with
  static juce::Colour ledColourFor(int color);

  using Cell = TouchTracker::Cell;
  // the cell under a point, column 0 for the control switches, invalid outside them all
  virtual Cell cellAt(juce::Point<float> position) const;
  // where a pad, or in column 0 a control switch, is drawn; empty where the surface hides it
  virtual juce::Rectangle<float> padBounds(int col, int row) const;
  bool showsFocusRing() const { return hasKeyboardFocus(false) && keyboardNavigating; }

protected:
  // The whole instrument, drawn to fit: case, panel, pads and name. Another way of showing the
  // pads overrides these: where everything goes, where a control switch and its label go (the
  // label printed on the switch or above it), what's drawn behind and in front of the pads, and
  // what happens when the keyboard moves the focus.
  virtual void layOut();
  virtual void controlSlot(int row, juce::Rectangle<float>& label, juce::Rectangle<float>& sw) const;
  virtual bool labelsOnSwitches() const { return false; }
  virtual float switchLabelHeight() const;
  virtual void paintBehind(juce::Graphics& g);
  virtual void paintInFront(juce::Graphics& g);
  virtual void focusMoved(const Cell&) {}
  // the label printed for a control switch, row 0 at the bottom
  static const char* controlLabel(int row);
  // what a control switch is labelled with now, its printed label unless a subclass names what
  // the switch does on the screen shown
  virtual juce::String switchLabel(int row) const { return controlLabel(row); }

  Engine& engine;
  int columns = 26;
  float padSize = 30;
  float gap = 3;
  juce::Rectangle<float> padsArea;
  juce::Rectangle<float> panel;
  juce::Rectangle<float> steel;

private:
  void timerCallback() override;
  void toggleFocusedCell();
  void moveFocus(int dCol, int dRow);
  juce::Colour ledColour(int color) const { return ledColourFor(color); }
  void paintCase(juce::Graphics& g);
  void paintPads(juce::Graphics& g);
  void paintBrand(juce::Graphics& g);

  TouchTracker touches;
  Cell focusCell;
  // the focus ring shows only while the keyboard moves it, never after a touch or a click
  bool keyboardNavigating = false;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Surface)
};
