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

#include <JuceHeader.h>

#include "Commands.h"
#include "Engine.h"
#include "Surface.h"

class MainComponent : public juce::Component,
                      public CommandTarget,
                      private juce::Timer {
public:
  explicit MainComponent(int model);
  ~MainComponent() override;

  void paint(juce::Graphics& g) override;
  void resized() override;

  // for the command interface in Commands.cpp
  Engine& getEngine() override { return engine; }
  bool saveScreenshot(const juce::File& file, float scale = 2.0f) override;
  void setModel(int model) override;
  void setFootSwitch(int which, bool down) override;

private:
  void timerCallback() override;
  void startModel();
  void fitWindowToInstrument();
  void footSwitchChanged(int which);
  void showStatus();
  void layoutFooter();

  Engine engine;
  Surface surface { engine };
  juce::Label brandLabel { {}, "LinnStrument" };
  juce::Label subLabel { {}, "VIRTUAL" };
  juce::ComboBox modelBox;
  juce::Label statusLabel;
  juce::Label pressureLabel { {}, "Pressure" };
  juce::Slider pressureSlider;
  juce::Label velocityLabel { {}, "Velocity" };
  juce::Slider velocitySlider;
  juce::TextButton footLeft { "Foot L" };
  juce::TextButton footRight { "Foot R" };
  juce::Label hintLabel;
  juce::TooltipWindow tooltips { this, 600 };

  static constexpr int barHeight = 60;
  static constexpr int hintHeight = 34;
  static constexpr int surfaceInset = 20;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
