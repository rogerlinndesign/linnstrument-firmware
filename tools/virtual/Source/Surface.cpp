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
#include "Surface.h"

#include "Firmware/FirmwareBridge.h"
#include "LinnLookAndFeel.h"

namespace {

const char* const controlLabels[8] = {
  "GLOBAL\nSETTINGS", "SPLIT", "SWITCH 2", "SWITCH 1", "OCTAVE/\nTRANSPOSE", "VOLUME", "PRESET", "PER-SPLIT\nSETTINGS"
};

// proportions of the instrument in pad widths, measured off the product photo
const float gapUnits = 0.08f;
const float woodUnits = 0.5f;
const float marginUnits = 0.85f;
const float labelUnits = 2.5f;
const float brandUnits = 2.4f;
const float switchWidthUnits = 0.78f;
const float switchHeightUnits = 0.42f;
const juce::Colour padGrey(0xffdadad6);
const juce::Colour plateGrey(0xffb6b6b1);
const char* const colourNames[12] = { "off", "red", "yellow", "green", "cyan", "blue", "magenta", "off", "white", "orange", "lime", "pink" };

// the control switches take the gutter, the plate holds the other columns
float widthUnits(int columns) {
  return labelUnits + (float) (columns - 1) + (float) (columns - 2) * gapUnits + brandUnits + 2 * marginUnits;
}

float heightUnits() {
  return 8 + 7 * gapUnits + 2 * marginUnits + 2 * woodUnits;
}

juce::Typeface::Ptr boldTypeface() {
  if (auto* lf = dynamic_cast<LinnLookAndFeel*>(&juce::LookAndFeel::getDefaultLookAndFeel())) return lf->bold;
  return nullptr;
}

// the squared type of the printed name and its line: Eurostile Bold Extended and Bold Condensed
// on the instrument, Sarpanch Black and Sarpanch Bold narrowed to the name's length here
juce::Typeface::Ptr brandTypeface() {
  static juce::Typeface::Ptr brand = juce::Typeface::createSystemTypefaceFor(BinaryData::SarpanchBlack_ttf, BinaryData::SarpanchBlack_ttfSize);
  return brand;
}
juce::Typeface::Ptr lineTypeface() {
  static juce::Typeface::Ptr line = juce::Typeface::createSystemTypefaceFor(BinaryData::SarpanchBold_ttf, BinaryData::SarpanchBold_ttfSize);
  return line;
}

}

Surface::Surface(Engine& e) : engine(e) {
  setWantsKeyboardFocus(true);
  setTitle("Playing surface");
  setHelpText("Arrow keys move between the pads, Space or Return presses the pad or lets it go.");
  focusCell.col = 1;
  focusCell.row = 0;
  startTimerHz(Engine::refreshHz);
}

Surface::~Surface() {
  stopTimer();
}

float Surface::aspectRatio() const {
  return widthUnits(firmware::columns()) / heightUnits();
}

void Surface::resized() {
  columns = firmware::columns();
  float wUnits = widthUnits(columns), hUnits = heightUnits();
  padSize = juce::jmin((float) getWidth() / wUnits, (float) getHeight() / hUnits);
  gap = padSize * gapUnits;
  float totalW = wUnits * padSize, totalH = hUnits * padSize;
  panel = juce::Rectangle<float>(((float) getWidth() - totalW) / 2, ((float) getHeight() - totalH) / 2, totalW, totalH);
  steel = panel.reduced(0, padSize * woodUnits);
  padsArea = juce::Rectangle<float>(panel.getX() + (labelUnits + marginUnits) * padSize, steel.getY() + marginUnits * padSize,
                                    (float) (columns - 1) * padSize + (float) (columns - 2) * gap, 8 * padSize + 7 * gap);
}

// column 0 is the small switch under its label in the gutter, the other columns are the pads
juce::Rectangle<float> Surface::padBounds(int col, int row) const {
  float top = padsArea.getBottom() - (float) (row + 1) * padSize - (float) row * gap;
  if (col == 0) {
    float gutterCentre = steel.getX() + (marginUnits + labelUnits / 2) * padSize;
    return { gutterCentre - switchWidthUnits * padSize / 2, top + padSize * 0.52f, switchWidthUnits * padSize, switchHeightUnits * padSize };
  }
  return { padsArea.getX() + (float) (col - 1) * (padSize + gap), top, padSize, padSize };
}

Surface::Cell Surface::cellAt(juce::Point<float> p) const {
  Cell c;
  juce::Rectangle<float> gutter(steel.getX(), padsArea.getY(), padsArea.getX() - steel.getX(), padsArea.getHeight());
  if (!padsArea.contains(p) && !gutter.contains(p)) return c;
  float yFromTop = (p.y - padsArea.getY()) / (padSize + gap);
  c.col = gutter.contains(p) ? 0 : juce::jlimit(1, columns - 1, 1 + (int) ((p.x - padsArea.getX()) / (padSize + gap)));
  c.row = juce::jlimit(0, 7, 7 - (int) yFromTop);
  auto pad = padBounds(c.col, c.row);
  c.xFraction = juce::jlimit(0.0f, 1.0f, (p.x - pad.getX()) / pad.getWidth());
  c.yFraction = juce::jlimit(0.0f, 1.0f, 1.0f - (p.y - pad.getY()) / pad.getHeight());
  return c;
}

juce::Colour Surface::ledColour(int color) const {
  switch (color) {
    case 1: return juce::Colour(0xffff4a3d);
    case 2: return juce::Colour(0xffffd23a);
    case 3: return juce::Colour(0xff58e07a);
    case 4: return juce::Colour(0xff5fe3f0);
    case 5: return juce::Colour(0xff4f8cff);
    case 6: return juce::Colour(0xffff5cf0);
    case 8: return juce::Colour(0xfff4f7ff);
    case 9: return juce::Colour(0xffff9a2e);
    case 10: return juce::Colour(0xffb8f04a);
    case 11: return juce::Colour(0xffff7ac2);
  }
  return juce::Colours::transparentBlack;
}

void Surface::paint(juce::Graphics& g) {
  float woodH = padSize * woodUnits;
  auto top = juce::Rectangle<float>(panel.getX(), panel.getY(), panel.getWidth(), woodH);
  auto bottom = juce::Rectangle<float>(panel.getX(), steel.getBottom(), panel.getWidth(), woodH);

  // cherry edges above and below the steel panel, lit from above
  juce::ColourGradient wood(juce::Colour(0xffd08048), 0, top.getY(), juce::Colour(0xff7a3d17), 0, top.getBottom(), false);
  wood.addColour(0.55, linn::cherry);
  g.setGradientFill(wood);
  g.fillRect(top);
  juce::ColourGradient woodBottom(juce::Colour(0xff8a4a1c), 0, bottom.getY(), juce::Colour(0xff5e2e10), 0, bottom.getBottom(), false);
  woodBottom.addColour(0.3, linn::cherry);
  g.setGradientFill(woodBottom);
  g.fillRect(bottom);

  juce::ColourGradient steelFill(juce::Colour(0xff181818), 0, steel.getY(), juce::Colour(0xff0d0d0d), 0, steel.getBottom(), false);
  g.setGradientFill(steelFill);
  g.fillRect(steel);
  g.setColour(juce::Colour(0xff2a2a2a));
  g.drawRect(steel, 1.0f);
  for (float sx : { steel.getX() + padSize * 0.35f, steel.getRight() - padSize * 0.35f }) {
    for (float sy : { steel.getY() + padSize * 0.35f, steel.getBottom() - padSize * 0.35f }) {
      g.setColour(juce::Colour(0xff0a0a0a));
      g.fillEllipse(sx - padSize * 0.11f, sy - padSize * 0.11f, padSize * 0.22f, padSize * 0.22f);
      g.setColour(juce::Colour(0xff3a3a3a));
      g.drawEllipse(sx - padSize * 0.11f, sy - padSize * 0.11f, padSize * 0.22f, padSize * 0.22f, 1.0f);
    }
  }

  // the plate the pads are raised out of
  g.setColour(plateGrey);
  g.fillRoundedRectangle(padsArea.expanded(gap), padSize * 0.08f);

  // the pads: grey plastic with the LED lighting the whole pad from below
  float radius = padSize * 0.09f;
  for (int col = 0; col < columns; ++col) {
    for (int row = 0; row < 8; ++row) {
      auto pad = padBounds(col, row);
      firmware::Led led = firmware::led(col, row);
      bool lit = led.color != 0 && led.color != 7 && firmware::pulseOn(led.display);
      juce::Colour colour = lit ? ledColour(led.color) : juce::Colours::transparentBlack;

      g.setColour(padGrey);
      g.fillRoundedRectangle(pad, radius);
      if (lit) {
        // the same light as the console's: white hot centre, the colour, fading into the grey
        juce::ColourGradient glow(juce::Colours::white, pad.getCentreX(), pad.getCentreY(),
                                  padGrey, pad.getCentreX() + padSize * 0.707f, pad.getCentreY(), true);
        glow.addColour(0.22, colour);
        glow.addColour(0.6, colour.interpolatedWith(padGrey, 0.45f));
        glow.addColour(0.85, padGrey);
        g.setGradientFill(glow);
        g.fillRoundedRectangle(pad, radius);
      }
      g.setColour(juce::Colour(0x4cffffff));
      g.drawLine(pad.getX() + radius, pad.getBottom() - 0.5f, pad.getRight() - radius, pad.getBottom() - 0.5f, 1.0f);
      if (col == 0) {
        g.setColour(juce::Colour(0x2e000000));
        g.drawRoundedRectangle(pad.reduced(0.5f), radius, 1.0f);
      }

      Engine::TouchView touch;
      if (engine.touchAt(col, row, touch)) {
        g.setColour(juce::Colour(0x99000000));
        g.drawRoundedRectangle(pad.reduced(2.0f), radius * 0.8f, 2.0f);
        // the fingertip, flattening as it presses harder
        float r = padSize * (0.11f + 0.16f * touch.pressure);
        float cx = pad.getX() + touch.xFraction * pad.getWidth(), cy = pad.getBottom() - touch.yFraction * pad.getHeight();
        juce::ColourGradient tip(juce::Colour(0xbf000000), cx, cy, juce::Colour(0x00000000), cx + r, cy, true);
        tip.addColour(0.6, juce::Colour(0x8c000000));
        g.setGradientFill(tip);
        g.fillEllipse(cx - r, cy - r, 2 * r, 2 * r);
      }
    }
  }
  if (hasKeyboardFocus(false) && focusCell.valid() && focusCell.col < columns) {
    g.setColour(linn::accent);
    g.drawRoundedRectangle(padBounds(focusCell.col, focusCell.row).expanded(1.5f), radius * 1.2f, 3.0f);
  }

  // the printed label above each control switch
  juce::Font labelFont(juce::FontOptions(boldTypeface()).withHeight(juce::jlimit(7.0f, 13.0f, padSize * 0.27f)).withKerningFactor(0.04f));
  g.setFont(labelFont);
  g.setColour(juce::Colour(0xffe6e6e6));
  for (int row = 0; row < 8; ++row) {
    auto sw = padBounds(0, row);
    float rowTop = padsArea.getBottom() - (float) (row + 1) * padSize - (float) row * gap;
    juce::Rectangle<float> label(sw.getCentreX() - labelUnits * padSize / 2, rowTop, labelUnits * padSize, sw.getY() - rowTop - padSize * 0.04f);
    g.drawFittedText(controlLabels[row], label.toNearestInt(), juce::Justification::centredBottom, 2, 1.0f);
  }

  // the name and its line, printed sideways at the right end in the instrument's proportions:
  // the name runs nearly the height of the plate, the line a quarter of its size and as long
  g.saveState();
  juce::Rectangle<float> brand(padsArea.getRight() + gap, padsArea.getY(), steel.getRight() - padsArea.getRight() - gap, padsArea.getHeight());
  g.addTransform(juce::AffineTransform::rotation(-juce::MathConstants<float>::halfPi, brand.getCentreX(), brand.getCentreY()));
  juce::Rectangle<float> turned(brand.getCentreX() - brand.getHeight() / 2, brand.getCentreY() - brand.getWidth() / 2, brand.getHeight(), brand.getWidth());
  juce::String name = columns == 17 ? "LinnStrument 128" : "LinnStrument";
  juce::Font nameFont(juce::FontOptions(brandTypeface()).withHeight(100.0f));
  float nameHeight = juce::jmax(8.0f, juce::jmin(turned.getHeight() * 0.66f, 100.0f * turned.getWidth() * 0.9f / juce::GlyphArrangement::getStringWidth(nameFont, name)));
  nameFont = nameFont.withHeight(nameHeight);
  const juce::String line = "Music Performance Keyboard with 3D Note Expression";
  juce::Font lineFont(juce::FontOptions(lineTypeface()).withHeight(nameHeight * 0.27f));
  lineFont = lineFont.withHorizontalScale(juce::jlimit(0.5f, 1.0f, juce::GlyphArrangement::getStringWidth(nameFont, name) / juce::GlyphArrangement::getStringWidth(lineFont, line)));
  auto block = turned.withSizeKeepingCentre(turned.getWidth(), nameHeight * 0.95f + lineFont.getHeight());
  g.setFont(nameFont);
  g.setColour(juce::Colours::white);
  g.drawText(name, block.removeFromTop(nameHeight * 0.95f), juce::Justification::centred);
  g.setFont(lineFont);
  g.setColour(juce::Colour(0xffd0d0d0));
  g.drawText(line, block, juce::Justification::centredTop);
  g.restoreState();
}

void Surface::timerCallback() {
  repaint();
}

void Surface::mouseDown(const juce::MouseEvent& e) {
  grabKeyboardFocus();
  Cell c = cellAt(e.position);
  if (!c.valid()) return;
  focusCell = c;
  int key = c.col * 8 + c.row;
  bool latch = e.mods.isShiftDown() || e.mods.isRightButtonDown();
  if (latched.count(key)) {
    latched.erase(key);
    engine.release(c.col, c.row);
    dragCell = Cell();
    return;
  }
  if (latch) latched.insert(key);
  engine.press(c.col, c.row, c.xFraction, c.yFraction);
  dragCell = c;
}

void Surface::mouseDrag(const juce::MouseEvent& e) {
  if (!dragCell.valid()) return;
  Cell c = cellAt(e.position);
  if (!c.valid()) return;
  if (c.col != dragCell.col || c.row != dragCell.row) {
    engine.press(c.col, c.row, c.xFraction, c.yFraction);
    engine.release(dragCell.col, dragCell.row);
    latched.erase(dragCell.col * 8 + dragCell.row);
    dragCell = c;
  }
  else {
    engine.move(c.col, c.row, c.xFraction, c.yFraction);
  }
}

void Surface::mouseUp(const juce::MouseEvent&) {
  if (!dragCell.valid()) return;
  if (!latched.count(dragCell.col * 8 + dragCell.row)) {
    engine.release(dragCell.col, dragCell.row);
  }
  dragCell = Cell();
}

void Surface::moveFocus(int dCol, int dRow) {
  focusCell.col = juce::jlimit(0, columns - 1, focusCell.col + dCol);
  focusCell.row = juce::jlimit(0, 7, focusCell.row + dRow);
  focusCell.xFraction = focusCell.yFraction = 0.5f;
  if (auto* handler = getAccessibilityHandler()) handler->notifyAccessibilityEvent(juce::AccessibilityEvent::valueChanged);
  repaint();
}

// the keyboard holds a touch until the same key lets it go again, like a shift-click
void Surface::toggleFocusedCell() {
  if (!focusCell.valid()) return;
  int key = focusCell.col * 8 + focusCell.row;
  if (latched.count(key)) {
    latched.erase(key);
    engine.release(focusCell.col, focusCell.row);
  }
  else {
    latched.insert(key);
    engine.press(focusCell.col, focusCell.row, 0.5f, 0.5f);
  }
  if (auto* handler = getAccessibilityHandler()) handler->notifyAccessibilityEvent(juce::AccessibilityEvent::valueChanged);
}

bool Surface::keyPressed(const juce::KeyPress& key) {
  if (key == juce::KeyPress::leftKey) moveFocus(-1, 0);
  else if (key == juce::KeyPress::rightKey) moveFocus(1, 0);
  else if (key == juce::KeyPress::upKey) moveFocus(0, 1);
  else if (key == juce::KeyPress::downKey) moveFocus(0, -1);
  else if (key == juce::KeyPress::homeKey) moveFocus(-columns, 0);
  else if (key == juce::KeyPress::endKey) moveFocus(columns, 0);
  else if (key == juce::KeyPress::spaceKey || key == juce::KeyPress::returnKey) toggleFocusedCell();
  else return false;
  return true;
}

void Surface::focusGained(FocusChangeType) {
  repaint();
}

void Surface::focusLost(FocusChangeType) {
  repaint();
}

juce::String Surface::describeFocusedCell() const {
  if (!focusCell.valid()) return {};
  firmware::Led led = firmware::led(focusCell.col, focusCell.row);
  juce::String text = "Column " + juce::String(focusCell.col) + ", row " + juce::String(focusCell.row) + ", " + colourNames[juce::jlimit(0, 11, (int) led.color)];
  if (latched.count(focusCell.col * 8 + focusCell.row)) text += ", held";
  else if (engine.isTouched(focusCell.col, focusCell.row)) text += ", pressed";
  return text;
}

std::unique_ptr<juce::AccessibilityHandler> Surface::createAccessibilityHandler() {
  struct FocusedCellValue : public juce::AccessibilityValueInterface {
    explicit FocusedCellValue(Surface& s) : surface(s) {}
    bool isReadOnly() const override { return true; }
    double getCurrentValue() const override { return 0; }
    juce::String getCurrentValueAsString() const override { return surface.describeFocusedCell(); }
    void setValue(double) override {}
    void setValueAsString(const juce::String&) override {}
    AccessibleValueRange getRange() const override { return {}; }
    Surface& surface;
  };
  return std::make_unique<juce::AccessibilityHandler>(*this, juce::AccessibilityRole::group,
                                                      juce::AccessibilityActions().addAction(juce::AccessibilityActionType::press, [this] { toggleFocusedCell(); }),
                                                      juce::AccessibilityHandler::Interfaces { std::make_unique<FocusedCellValue>(*this) });
}
