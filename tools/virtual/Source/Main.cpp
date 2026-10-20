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
#include <JuceHeader.h>

#include <iostream>

#include "Commands.h"
#include "LinnLookAndFeel.h"
#include "MainComponent.h"

class VirtualLinnStrumentApplication : public juce::JUCEApplication {
public:
  const juce::String getApplicationName() override { return JUCE_APPLICATION_NAME_STRING; }
  const juce::String getApplicationVersion() override { return JUCE_APPLICATION_VERSION_STRING; }
  bool moreThanOneInstanceAllowed() override { return false; }

  // --model 128 starts as a 128; --screenshot FILE.png writes the window to a file and quits,
  // after --after SECONDS (default 2.5), for a session that cannot see the screen;
  // --script FILE (or - for stdin) reads the commands Commands.cpp lists
  void initialise(const juce::String& commandLine) override {
    juce::LookAndFeel::setDefaultLookAndFeel(&lookAndFeel);
    juce::StringArray args = juce::StringArray::fromTokens(commandLine, true);
    int model = args.contains("--model") && args[args.indexOf("--model") + 1] == "128" ? 128 : 200;
    window = std::make_unique<MainWindow>(getApplicationName(), model);
    auto* main = dynamic_cast<MainComponent*>(window->getContentComponent());
    int at = args.indexOf("--screenshot");
    if (at >= 0 && at + 1 < args.size()) {
      juce::File file(juce::File::getCurrentWorkingDirectory().getChildFile(args[at + 1].unquoted()));
      int afterAt = args.indexOf("--after");
      double seconds = afterAt >= 0 && afterAt + 1 < args.size() ? args[afterAt + 1].getDoubleValue() : 2.5;
      juce::Timer::callAfterDelay((int) (seconds * 1000), [main, file] {
        main->saveScreenshot(file);
        quit();
      });
    }
    at = args.indexOf("--script");
    if (at >= 0 && (at + 1 >= args.size() || args[at + 1].startsWith("--"))) {
      std::cout << "error: --script needs a file or - for stdin" << std::endl;
      setApplicationReturnValue(1);
      quit();
      return;
    }
    if (at >= 0) commands = std::make_unique<Commands>(*main, args[at + 1].unquoted());
  }

  void shutdown() override {
    commands.reset();
    window.reset();
    juce::LookAndFeel::setDefaultLookAndFeel(nullptr);
  }

  void systemRequestedQuit() override { quit(); }

private:
  class MainWindow : public juce::DocumentWindow {
  public:
    MainWindow(const juce::String& name, int model)
      : DocumentWindow(name, juce::Colours::black, DocumentWindow::allButtons) {
      setUsingNativeTitleBar(true);
      setContentOwned(new MainComponent(model), true);
      setResizable(true, false);
      setResizeLimits(820, 360, 3200, 1400);
      centreWithSize(getWidth(), getHeight());
      setVisible(true);
    }

    void closeButtonPressed() override {
      juce::JUCEApplication::getInstance()->systemRequestedQuit();
    }

  private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainWindow)
  };

  LinnLookAndFeel lookAndFeel;
  std::unique_ptr<MainWindow> window;
  std::unique_ptr<Commands> commands;
};

START_JUCE_APPLICATION (VirtualLinnStrumentApplication)
