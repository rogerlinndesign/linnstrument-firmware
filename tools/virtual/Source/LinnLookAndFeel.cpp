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
#include "LinnLookAndFeel.h"

LinnLookAndFeel::LinnLookAndFeel() {
  regular = juce::Typeface::createSystemTypefaceFor(BinaryData::MulishRegular_ttf, BinaryData::MulishRegular_ttfSize);
  bold = juce::Typeface::createSystemTypefaceFor(BinaryData::MulishBold_ttf, BinaryData::MulishBold_ttfSize);
  setDefaultSansSerifTypeface(regular);
  boldFace() = bold;

  auto scheme = getDarkColourScheme();
  scheme.setUIColour(ColourScheme::windowBackground, linn::black);
  scheme.setUIColour(ColourScheme::widgetBackground, linn::panel2);
  scheme.setUIColour(ColourScheme::menuBackground, linn::panel2);
  scheme.setUIColour(ColourScheme::outline, linn::line);
  scheme.setUIColour(ColourScheme::defaultText, linn::text);
  scheme.setUIColour(ColourScheme::defaultFill, linn::accent);
  scheme.setUIColour(ColourScheme::highlightedText, juce::Colours::white);
  scheme.setUIColour(ColourScheme::highlightedFill, linn::accent);
  scheme.setUIColour(ColourScheme::menuText, linn::text);
  setColourScheme(scheme);

  setColour(juce::Label::textColourId, linn::text);
  setColour(juce::TextButton::textColourOffId, juce::Colours::white);
  setColour(juce::TextButton::textColourOnId, juce::Colours::white);
  setColour(juce::ComboBox::textColourId, juce::Colours::white);
  setColour(juce::ComboBox::arrowColourId, linn::muted);
  setColour(juce::PopupMenu::backgroundColourId, linn::panel2);
  setColour(juce::PopupMenu::textColourId, linn::text);
  setColour(juce::PopupMenu::highlightedBackgroundColourId, linn::accent);
  setColour(juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
  setColour(juce::TooltipWindow::backgroundColourId, linn::panel2);
  setColour(juce::TooltipWindow::textColourId, linn::text);
  setColour(juce::TooltipWindow::outlineColourId, linn::line);
}

juce::Font LinnLookAndFeel::getTextButtonFont(juce::TextButton&, int buttonHeight) {
  return juce::Font(juce::FontOptions(bold).withHeight(juce::jmin(14.0f, (float) buttonHeight * 0.55f)));
}

juce::Font LinnLookAndFeel::getLabelFont(juce::Label& label) {
  juce::Font font = label.getFont();
  return juce::Font(juce::FontOptions(font.isBold() ? bold : regular).withHeight(font.getHeight()).withKerningFactor(font.getExtraKerningFactor()));
}

juce::Font LinnLookAndFeel::getComboBoxFont(juce::ComboBox&) {
  return juce::Font(juce::FontOptions(regular).withHeight(14.0f));
}

juce::Font LinnLookAndFeel::getPopupMenuFont() {
  return juce::Font(juce::FontOptions(regular).withHeight(14.0f));
}

void LinnLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button, const juce::Colour&,
                                           bool highlighted, bool down) {
  auto area = button.getLocalBounds().toFloat().reduced(0.5f);
  // a button joined to another keeps its corners square on that side, so that the two read as one
  bool left = button.isConnectedOnLeft(), right = button.isConnectedOnRight();
  bool top = button.isConnectedOnTop(), bottom = button.isConnectedOnBottom();
  juce::Path shape;
  shape.addRoundedRectangle(area.getX(), area.getY(), area.getWidth(), area.getHeight(), 4.0f, 4.0f,
                            !(left || top), !(right || top), !(left || bottom), !(right || bottom));
  bool on = button.getToggleState();
  if (on) {
    g.setColour(down ? linn::accent.darker(0.2f) : highlighted ? linn::accent2 : linn::accent);
    g.fillPath(shape);
    return;
  }
  if (down) {
    g.setColour(juce::Colour(0x22ffffff));
    g.fillPath(shape);
  }
  g.setColour(highlighted ? linn::muted : linn::line);
  g.strokePath(shape, juce::PathStrokeType(1.0f));
}

void LinnLookAndFeel::drawComboBox(juce::Graphics& g, int width, int height, bool, int, int, int, int, juce::ComboBox& box) {
  auto area = juce::Rectangle<float>(0, 0, (float) width, (float) height).reduced(0.5f);
  g.setColour(linn::panel2);
  g.fillRoundedRectangle(area, 4.0f);
  g.setColour(box.isMouseOver(true) ? linn::dim : linn::line);
  g.drawRoundedRectangle(area, 4.0f, 1.0f);
  juce::Path chevron;
  float cx = (float) width - 15.0f, cy = (float) height * 0.5f;
  chevron.startNewSubPath(cx - 4.0f, cy - 2.0f);
  chevron.lineTo(cx, cy + 2.0f);
  chevron.lineTo(cx + 4.0f, cy - 2.0f);
  g.setColour(linn::muted);
  g.strokePath(chevron, juce::PathStrokeType(1.5f));
}

void LinnLookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                                       float, float, juce::Slider::SliderStyle, juce::Slider& slider) {
  float cy = (float) y + (float) height * 0.5f;
  auto track = juce::Rectangle<float>((float) x, cy - 2.0f, (float) width, 4.0f);
  g.setColour(linn::line);
  g.fillRoundedRectangle(track, 2.0f);
  g.setColour(linn::accent);
  g.fillRoundedRectangle(track.withRight(sliderPos), 2.0f);
  float r = slider.isMouseOverOrDragging() ? 8.0f : 7.0f;
  g.setColour(juce::Colour(0x66000000));
  g.fillEllipse(sliderPos - r, cy - r + 1.0f, 2 * r, 2 * r);
  g.setColour(juce::Colours::white);
  g.fillEllipse(sliderPos - r, cy - r, 2 * r, 2 * r);
}
