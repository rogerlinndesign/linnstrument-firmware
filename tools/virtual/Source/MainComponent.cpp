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
#include "MainComponent.h"

#include "LinnLookAndFeel.h"

MainComponent::MainComponent(int model) {
  addAndMakeVisible(surface);

  brandLabel.setFont(juce::Font(juce::FontOptions(21.0f, juce::Font::bold)));
  brandLabel.setColour(juce::Label::textColourId, juce::Colours::white);
  addAndMakeVisible(brandLabel);
  subLabel.setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::bold).withKerningFactor(0.14f)));
  subLabel.setColour(juce::Label::textColourId, linn::muted);
  addAndMakeVisible(subLabel);

  modelBox.addItem("LinnStrument 200", 200);
  modelBox.addItem("LinnStrument 128", 128);
  modelBox.setSelectedId(model, juce::dontSendNotification);
  modelBox.setTitle("Model");
  modelBox.setTooltip("Which instrument the firmware runs as");
  modelBox.setHelpText(modelBox.getTooltip());
  modelBox.onChange = [this] { startModel(); fitWindowToInstrument(); };
  addAndMakeVisible(modelBox);

  statusLabel.setJustificationType(juce::Justification::centredLeft);
  statusLabel.setColour(juce::Label::textColourId, linn::muted);
  statusLabel.setFont(juce::Font(juce::FontOptions(12.0f)));
  statusLabel.setTitle("Instrument");
  statusLabel.setMinimumHorizontalScale(1.0f);
  addAndMakeVisible(statusLabel);

  for (auto* label : { &pressureLabel, &velocityLabel }) {
    label->setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::bold).withKerningFactor(0.12f)));
    label->setColour(juce::Label::textColourId, linn::muted);
    label->setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(label);
  }
  pressureLabel.setText("PRESSURE", juce::dontSendNotification);
  velocityLabel.setText("VELOCITY", juce::dontSendNotification);

  pressureSlider.setRange(100, 3000, 1);
  pressureSlider.setValue(600, juce::dontSendNotification);
  pressureSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
  pressureSlider.setTitle("Pressure");
  pressureSlider.setTooltip("How hard the mouse presses: the pressure every touch reports");
  pressureSlider.setHelpText(pressureSlider.getTooltip());
  pressureLabel.attachToComponent(&pressureSlider, true);
  pressureSlider.onValueChange = [this] { engine.setPressure((int) pressureSlider.getValue()); };
  addAndMakeVisible(pressureSlider);

  velocitySlider.setRange(1, 12, 1);
  velocitySlider.setValue(10, juce::dontSendNotification);
  velocitySlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
  velocitySlider.setTitle("Velocity");
  velocitySlider.setTooltip("How fast the mouse strikes: further right is a harder hit");
  velocitySlider.setHelpText(velocitySlider.getTooltip());
  velocityLabel.attachToComponent(&velocitySlider, true);
  velocitySlider.onValueChange = [this] { engine.setVelocityScans(13 - (int) velocitySlider.getValue()); };
  addAndMakeVisible(velocitySlider);

  for (auto* button : { &footLeft, &footRight }) {
    button->setClickingTogglesState(true);
    addAndMakeVisible(button);
  }
  footLeft.setTitle("Left foot switch");
  footLeft.setTooltip("The left foot switch, the arpeggiator by default");
  footLeft.setHelpText(footLeft.getTooltip());
  footRight.setTitle("Right foot switch");
  footRight.setTooltip("The right foot switch, sustain by default");
  footRight.setHelpText(footRight.getTooltip());
  footLeft.onClick = [this] { footSwitchChanged(0); };
  footRight.onClick = [this] { footSwitchChanged(1); };

  hintLabel.setText("Click or press Space on a pad to play, drag to slide or bend, shift-click holds a touch. Arrow keys move between pads.", juce::dontSendNotification);
  hintLabel.setColour(juce::Label::textColourId, linn::dim);
  hintLabel.setFont(juce::Font(juce::FontOptions(12.0f)));
  hintLabel.setJustificationType(juce::Justification::centredRight);
  hintLabel.setMinimumHorizontalScale(1.0f);
  addAndMakeVisible(hintLabel);

  engine.setVelocityScans(3);
  startModel();
  fitWindowToInstrument();
  startTimerHz(4);
}

MainComponent::~MainComponent() {
  stopTimer();
  engine.stop();
}

void MainComponent::startModel() {
  engine.start(modelBox.getSelectedId());
  surface.resized();
  showStatus();
}

void MainComponent::showStatus() {
  auto text = engine.status();
  if (text == statusLabel.getText()) return;
  statusLabel.setText(text, juce::dontSendNotification);
  layoutFooter();
}

// the status takes the width its text needs and the hint what is left, ending in an ellipsis
// when that is too little; a label would otherwise squeeze its text narrower to make it fit
void MainComponent::layoutFooter() {
  auto bottom = getLocalBounds().removeFromBottom(hintHeight).reduced(24, 6);
  auto border = statusLabel.getBorderSize().getLeftAndRight();
  int needed = (int) std::ceil(juce::GlyphArrangement::getStringWidth(statusLabel.getFont(), statusLabel.getText())) + border + 2;
  statusLabel.setBounds(bottom.removeFromLeft(juce::jmin(needed, bottom.getWidth())));
  bottom.removeFromLeft(24);
  hintLabel.setBounds(bottom);
}

// the window takes the instrument's own proportions, so no black bands are left around it
void MainComponent::fitWindowToInstrument() {
  int width = juce::jmax(getWidth(), 1180);
  int surfaceWidth = width - 2 * surfaceInset;
  int surfaceHeight = juce::roundToInt((float) surfaceWidth / surface.aspectRatio());
  setSize(width, barHeight + surfaceInset + surfaceHeight + surfaceInset + hintHeight);
}

void MainComponent::footSwitchChanged(int which) {
  engine.setFootSwitch(which, which == 0 ? footLeft.getToggleState() : footRight.getToggleState());
}

void MainComponent::setFootSwitch(int which, bool down) {
  (which == 0 ? footLeft : footRight).setToggleState(down, juce::dontSendNotification);
  footSwitchChanged(which);
}

void MainComponent::setModel(int model) {
  if (modelBox.getSelectedId() == model) return;
  modelBox.setSelectedId(model, juce::sendNotificationSync);
}

bool MainComponent::saveScreenshot(const juce::File& file, float scale) {
  juce::Image image = createComponentSnapshot(getLocalBounds(), false, scale);
  juce::FileOutputStream out(file);
  if (!out.openedOk()) return false;
  out.setPosition(0);
  out.truncate();
  return juce::PNGImageFormat().writeImageToStream(image, out);
}

void MainComponent::timerCallback() {
  showStatus();
}

void MainComponent::paint(juce::Graphics& g) {
  g.fillAll(linn::black);
  // the warm light the product photos put behind the instrument
  auto stage = surface.getBounds().toFloat();
  juce::ColourGradient glow(linn::cherry.withAlpha(0.26f), stage.getCentreX(), stage.getCentreY() - stage.getHeight() * 0.1f,
                            linn::cherry.withAlpha(0.0f), stage.getCentreX(), stage.getCentreY() + stage.getWidth() * 0.55f, true);
  g.setGradientFill(glow);
  g.fillRect(getLocalBounds());
  g.setColour(linn::bar);
  g.fillRect(0, 0, getWidth(), barHeight);
}

void MainComponent::resized() {
  auto area = getLocalBounds();
  auto top = area.removeFromTop(barHeight).reduced(24, 15);
  // what gives way, in order, when the window is narrower than the bar needs: the VIRTUAL tag,
  // then the sliders down to 60, then the model menu down to 136
  int sub = 82, slider = 130, model = 176;
  int deficit = 138 + sub + model + 20 + 2 * slider + 70 + 16 + 66 + 28 + 74 + 8 + 74 - top.getWidth();
  if (deficit > 0) { sub = 0; deficit -= 82; }
  if (deficit > 0) { int cut = juce::jmin(deficit, 2 * (130 - 60)); slider -= (cut + 1) / 2; deficit -= cut; }
  if (deficit > 0) { int cut = juce::jmin(deficit, 176 - 136); model -= cut; }
  subLabel.setVisible(sub > 0);
  brandLabel.setBounds(top.removeFromLeft(138));
  if (sub > 0) {
    subLabel.setBounds(top.removeFromLeft(70));
    top.removeFromLeft(12);
  }
  modelBox.setBounds(top.removeFromLeft(model));
  top.removeFromLeft(20);
  footRight.setBounds(top.removeFromRight(74));
  top.removeFromRight(8);
  footLeft.setBounds(top.removeFromRight(74));
  top.removeFromRight(28);
  velocitySlider.setBounds(top.removeFromRight(slider));
  velocityLabel.setBounds(top.removeFromRight(66));
  top.removeFromRight(16);
  pressureSlider.setBounds(top.removeFromRight(slider));
  pressureLabel.setBounds(top.removeFromRight(70));

  area.removeFromBottom(hintHeight);
  layoutFooter();
  surface.setBounds(area.reduced(surfaceInset, surfaceInset));
}
