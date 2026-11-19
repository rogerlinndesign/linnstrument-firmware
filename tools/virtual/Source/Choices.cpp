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
#include "Choices.h"

Choices::Choices(const juce::StringArray& args, const juce::File& file) {
  juce::PropertiesFile::Options options;
  options.storageFormat = juce::PropertiesFile::storeAsXML;
  bool scripted = args.contains("--script") || args.contains("--screenshot");
  kept = std::make_unique<juce::PropertiesFile>(scripted ? juce::File() : file, options);
  startModel = kept->getIntValue("model", 200) == 128 ? 128 : 200;
  startHeadings = kept->getBoolValue("headings", false);
  if (args.contains("--model")) startModel = args[args.indexOf("--model") + 1] == "128" ? 128 : 200;
  if (args.contains("--headings")) startHeadings = true;
}

Choices::~Choices() {
  if (modelBox != nullptr) modelBox->removeListener(this);
  if (headingsButton != nullptr) headingsButton->removeListener(this);
}

void Choices::follow(juce::ComboBox& model, juce::Button& headings) {
  modelBox = &model;
  headingsButton = &headings;
  model.addListener(this);
  headings.addListener(this);
}

void Choices::comboBoxChanged(juce::ComboBox* box) {
  kept->setValue("model", box->getSelectedId());
  kept->saveIfNeeded();
}

void Choices::buttonClicked(juce::Button* button) {
  kept->setValue("headings", button->getToggleState());
  kept->saveIfNeeded();
}
