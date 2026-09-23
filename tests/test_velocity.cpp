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
// Strike velocity from the first pressure samples of a touch. The fit includes a zero point at
// the start of the touch, so a ramp from zero pressure models a strike; its height sets the speed.

static byte velocityForRamp(unsigned short from, unsigned short to) {
  linn::select(5, 3);
  initVelocity();
  VelocityState state = velocityCalculating;
  for (int i = 0; i < VELOCITY_TOTAL_SAMPLES; ++i) {
    unsigned short z = from + (to - from) * i / (VELOCITY_TOTAL_SAMPLES - 1);
    state = calcVelocity(z);
    if (i < VELOCITY_TOTAL_SAMPLES - 1) {
      CHECK_EQ(state, velocityCalculating);
    }
  }
  CHECK_EQ(state, velocityNew);
  return sensorCell->velocity;
}

// once the window has passed, the velocity is settled: a much firmer sample no longer moves it
TEST(velocity_is_decided_within_the_sample_window) {
  linn::reset();
  byte v = velocityForRamp(0, 40);
  CHECK_EQ(calcVelocity(640), velocityCalculated);
  CHECK_EQ(sensorCell->velocity, v);
}

TEST(velocity_rises_with_the_strike) {
  linn::reset();
  byte previous = 0;
  const unsigned short tops[] = {5, 10, 20, 40, 80};
  for (unsigned short top : tops) {
    byte v = velocityForRamp(0, top);
    CHECK(v > previous);
    previous = v;
  }
  CHECK_EQ(velocityForRamp(0, 640), 127);
}

TEST(velocity_sensitivity_scales_the_same_strike) {
  linn::reset();
  Global.velocitySensitivity = velocityLow;
  byte low = velocityForRamp(0, 20);
  Global.velocitySensitivity = velocityMedium;
  byte medium = velocityForRamp(0, 20);
  Global.velocitySensitivity = velocityHigh;
  byte high = velocityForRamp(0, 20);
  CHECK(low < medium);
  CHECK(medium < high);
}

TEST(velocity_fixed_setting_overrides_the_strike) {
  linn::reset();
  Global.velocitySensitivity = velocityFixed;
  Global.valueForFixedVelocity = 64;
  CHECK_EQ(velocityForRamp(0, 640), 64);
}
