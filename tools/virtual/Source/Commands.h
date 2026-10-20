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
// Text commands from a script file or stdin, so that a session without a screen or a mouse can
// touch the instrument, read its lights and take screenshots. Every command answers on stdout
// with its output and then "ok", or with "error: ..." and nothing else.
#pragma once

#include <JuceHeader.h>

#include <atomic>
#include <functional>
#include <istream>
#include <memory>
#include <thread>

class MainComponent;

class Commands {
public:
  // source is a file path, or "-" for stdin
  Commands(MainComponent& main, const juce::String& source);
  ~Commands();

  static const char* help();

private:
  // shared with the reader thread, which may outlive this object when it reads stdin
  struct Reader {
    std::atomic<bool> stop { false };
    std::atomic<bool> busy { false };   // a command is being carried out
    std::atomic<bool> done { false };   // the reader returned
  };

  void readLoop(std::shared_ptr<Reader> state, std::unique_ptr<std::istream> in);
  juce::String execute(const juce::StringArray& words);
  juce::String onMessageThread(std::function<juce::String()> action);
  void pause(int milliseconds);

  MainComponent& main;
  bool fromStdin = false;
  std::shared_ptr<Reader> reader = std::make_shared<Reader>();
  std::thread thread;
};
