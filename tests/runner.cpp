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
// Included last into the test translation unit: runs every registered test and reports.
LinnTest* linnTests = NULL;
int linnChecks = 0;
int linnFailures = 0;

// Registration prepends, so the list is reversed first to run the tests in the order they are
// written, file by file.
static LinnTest* inDeclarationOrder() {
  LinnTest* reversed = NULL;
  for (LinnTest* t = linnTests; t;) {
    LinnTest* next = t->next;
    t->next = reversed;
    reversed = t;
    t = next;
  }
  return reversed;
}

// An argument restricts the run to tests whose name contains it.
int main(int argc, char** argv) {
  const char* filter = argc > 1 ? argv[1] : "";
  int tests = 0, failedTests = 0;
  for (LinnTest* t = inDeclarationOrder(); t; t = t->next) {
    if (*filter && !strstr(t->name, filter)) continue;
    int before = linnFailures;
    std::printf("%s\n", t->name);
    t->run();
    ++tests;
    if (linnFailures > before) ++failedTests;
  }
  std::printf("\n%d tests, %d checks, %d failed checks, %d failed tests\n", tests, linnChecks, linnFailures, failedTests);
  return linnFailures ? 1 : 0;
}
