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
// The model and the headings the window was left with, taken up again at the next launch. A run
// driven by a script or taking a screenshot starts as a 200 without headings and keeps nothing, so
// that it shows the same every time and leaves the player's choice alone.
#pragma once

#include <JuceHeader.h>

class Choices : private juce::ComboBox::Listener, private juce::Button::Listener {
public:
  Choices(const juce::StringArray& args, const juce::File& file);
  ~Choices() override;

  int model() const { return startModel; }
  bool headings() const { return startHeadings; }
  bool keeps() const { return kept->getFile() != juce::File(); }

  // keeps the model box's and the headings button's choice from now on
  void follow(juce::ComboBox& model, juce::Button& headings);

private:
  void comboBoxChanged(juce::ComboBox* box) override;
  void buttonClicked(juce::Button* button) override;

  std::unique_ptr<juce::PropertiesFile> kept;
  juce::Component::SafePointer<juce::ComboBox> modelBox;
  juce::Component::SafePointer<juce::Button> headingsButton;
  int startModel = 200;
  bool startHeadings = false;
};
