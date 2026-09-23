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
// Self-registering test cases and checks, no dependencies. Every test file is compiled into the
// same translation unit as the sketch, so tests call firmware functions and read globals directly.
#ifndef LINN_TESTS_FRAMEWORK_H
#define LINN_TESTS_FRAMEWORK_H

#include <cstdio>
#include <cstdlib>

struct LinnTest {
  const char* name;
  void (*run)();
  LinnTest* next;
};

extern LinnTest* linnTests;
extern int linnChecks;
extern int linnFailures;

struct LinnTestRegistration {
  LinnTest test;
  LinnTestRegistration(const char* name, void (*run)()) {
    test.name = name;
    test.run = run;
    test.next = linnTests;
    linnTests = &test;
  }
};

#define TEST(name)                                           \
  static void linn_test_##name();                            \
  static LinnTestRegistration linn_reg_##name(#name, linn_test_##name); \
  static void linn_test_##name()

#define CHECK(condition)                                                        \
  do {                                                                          \
    ++linnChecks;                                                               \
    if (!(condition)) {                                                         \
      ++linnFailures;                                                           \
      std::printf("  FAIL %s:%d  %s\n", __FILE__, __LINE__, #condition);        \
    }                                                                           \
  } while (0)

#define CHECK_EQ(actual, expected)                                                            \
  do {                                                                                        \
    ++linnChecks;                                                                             \
    long long linnA = (long long)(actual), linnB = (long long)(expected);                     \
    if (linnA != linnB) {                                                                     \
      ++linnFailures;                                                                         \
      std::printf("  FAIL %s:%d  %s == %s  (got %lld, expected %lld)\n", __FILE__, __LINE__, \
                  #actual, #expected, linnA, linnB);                                          \
    }                                                                                         \
  } while (0)

#define CHECK_NEAR(actual, expected, tolerance)                                                  \
  do {                                                                                           \
    ++linnChecks;                                                                                \
    long long linnA = (long long)(actual), linnB = (long long)(expected);                        \
    long long linnD = linnA > linnB ? linnA - linnB : linnB - linnA;                             \
    if (linnD > (long long)(tolerance)) {                                                        \
      ++linnFailures;                                                                            \
      std::printf("  FAIL %s:%d  %s within %lld of %s  (got %lld, expected %lld)\n", __FILE__,  \
                  __LINE__, #actual, (long long)(tolerance), #expected, linnA, linnB);           \
    }                                                                                            \
  } while (0)

#endif
