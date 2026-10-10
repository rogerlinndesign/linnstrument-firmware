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
// The look of rogerlinndesign.com: Mulish, near-black panels, white text over grey, the blue
// accent for the one action that matters.
#pragma once

#include <JuceHeader.h>

namespace linn {
const juce::Colour black { 0xff000000 };
const juce::Colour bar { 0xff1a1a1a };
const juce::Colour panel { 0xff202020 };
const juce::Colour panel2 { 0xff2a2a2a };
const juce::Colour line { 0xff444444 };
const juce::Colour text { 0xffd9d9d9 };
const juce::Colour muted { 0xffb9b9b9 };
const juce::Colour dim { 0xff8e8e8e };
const juce::Colour accent { 0xff0082f3 };
const juce::Colour accent2 { 0xff3898ec };
const juce::Colour cherry { 0xffc2641f };
}

class LinnLookAndFeel : public juce::LookAndFeel_V4 {
public:
  LinnLookAndFeel();

  juce::Font getTextButtonFont(juce::TextButton&, int buttonHeight) override;
  juce::Font getLabelFont(juce::Label&) override;
  juce::Font getComboBoxFont(juce::ComboBox&) override;
  juce::Font getPopupMenuFont() override;
  void drawButtonBackground(juce::Graphics&, juce::Button&, const juce::Colour& backgroundColour,
                            bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
  void drawComboBox(juce::Graphics&, int width, int height, bool isButtonDown,
                    int buttonX, int buttonY, int buttonW, int buttonH, juce::ComboBox&) override;
  void drawLinearSlider(juce::Graphics&, int x, int y, int width, int height, float sliderPos,
                        float minSliderPos, float maxSliderPos, juce::Slider::SliderStyle, juce::Slider&) override;

  juce::Typeface::Ptr regular;
  juce::Typeface::Ptr bold;
};
