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
#include "FirmwareBridge.h"

#include <mutex>

namespace firmware {

namespace {
struct Pool {
  std::mutex lock;
  std::vector<Instance*> all;
  std::vector<bool> taken;
};

Pool& pool() {
  static Pool p;
  return p;
}
}

void add(Instance* instance) {
  auto& p = pool();
  std::lock_guard<std::mutex> guard(p.lock);
  p.all.push_back(instance);
  p.taken.push_back(false);
}

int instanceCount() {
  auto& p = pool();
  std::lock_guard<std::mutex> guard(p.lock);
  return (int) p.all.size();
}

Instance* acquire() {
  auto& p = pool();
  std::lock_guard<std::mutex> guard(p.lock);
  for (size_t i = 0; i < p.all.size(); ++i) {
    if (!p.taken[i]) {
      p.taken[i] = true;
      return p.all[i];
    }
  }
  return nullptr;
}

void release(Instance* instance) {
  auto& p = pool();
  std::lock_guard<std::mutex> guard(p.lock);
  for (size_t i = 0; i < p.all.size(); ++i) {
    if (p.all[i] == instance) p.taken[i] = false;
  }
}

const char* version() {
  auto& p = pool();
  std::lock_guard<std::mutex> guard(p.lock);
  return p.all.empty() ? "" : p.all.front()->version();
}

size_t flashSize() {
  auto& p = pool();
  std::lock_guard<std::mutex> guard(p.lock);
  return p.all.empty() ? 0 : p.all.front()->flashSize();
}

}
