#ifndef RECTANGLE_CIRCUIT_H
#define RECTANGLE_CIRCUIT_H

#include <stdint.h>

// For an isolated rectangle, use settled, time-coherent, inverted ADC codes
// from READ_Z, before sensitivity or cell compensation. RLD's sensor schematic
// shows an 8.2k pull-up: the resistance seen by the ADC is 8200*(4095/z - 1).
namespace rectangleCircuit {

enum Verdict { inconclusive, absent, present };

// ADS7883S: up to 2 LSB INL, 3 offset, 3.5 gain and half a code rounding.
// This covers converter error, not changing fingers or unsettled acquisition.
const uint16_t adcError = 9;
inline bool usable(uint16_t z) {
  return z > adcError && z < 4095 - adcError;
}

inline int32_t measure(uint16_t candidate, uint16_t sameRow,
                       uint16_t sameCol, uint16_t diagonal) {
  if (!usable(candidate) || !usable(sameRow) || !usable(sameCol) || !usable(diagonal))
    return INT32_MIN;
  // With three point contacts the only route to the empty intersection goes
  // through the other three contacts in series. Their contact resistances cancel
  // even when their pressures differ. Round the three series terms down and
  // the candidate up to obtain a lower bound on that residual in ohms.
  const int32_t pullup = 8200, numerator = pullup * 4095;
  const int32_t lowerCode = candidate - adcError;
  return numerator / (sameRow + adcError) + numerator / (sameCol + adcError) +
         numerator / (diagonal + adcError) - (numerator + lowerCode - 1) / lowerCode - 2 * pullup;
}

inline uint32_t stripUpper(uint8_t a, uint8_t b, uint8_t count, uint32_t totalOhms) {
  // With both ends of a uniform strip selected, access at fractional position
  // x costs T*x*(1-x). The unselected route costs T*abs(b-a). For a <= b,
  // their difference, divided by T, is 2*a-a*a-b*b. Maximise it over the two
  // pad intervals, allowing a full pad of placement/rail-end uncertainty.
  if (a > b) { uint8_t tmp = a; a = b; b = tmp; }
  const int32_t scale = 256;
  int32_t aHigh = ((int32_t(a) + 2) * scale + count - 1) / count;
  int32_t bLow = b ? (int32_t(b) - 1) * scale / count : 0;
  if (aHigh >= bLow) {
    int32_t closest = scale / 2;
    if (closest > aHigh) closest = aHigh;
    if (closest < bLow) closest = bLow;
    aHigh = bLow = closest;
  }
  int32_t coefficient = 2 * aHigh * scale - aHigh * aHigh - bLow * bLow;
  if (coefficient <= 0) return 0;
  return (totalOhms * uint32_t(coefficient) + 65535) >> 16;
}

inline int32_t stripBounds(uint8_t candidateCol, uint8_t otherCol,
                          uint8_t candidateRow, uint8_t otherRow, uint8_t numCols) {
  // Read-Z schematic labels the row/column access ranges 0..14k / 0..15.2k.
  // A uniform strip tied at both ends has peak access T/4. Treat those ranges
  // as engineering envelopes (not measured production tolerances). Keep the
  // larger envelope for either model. numCols includes the control column.
  if ((numCols != 17 && numCols != 26) || !candidateCol || !otherCol ||
      candidateCol >= numCols || otherCol >= numCols || candidateRow >= 8 ||
      otherRow >= 8 || candidateRow == otherRow || candidateCol == otherCol)
    return INT32_MAX;
  return int32_t(stripUpper(candidateCol, otherCol, numCols, 56000) +
                 stripUpper(candidateRow, otherRow, 8, 60800));
}

inline Verdict classify(int32_t observedLowerOhms, int32_t openUpperOhms) {
  return openUpperOhms >= 0 && observedLowerOhms > openUpperOhms ? present : inconclusive;
}

// A floating sense line can be pulled off its open-circuit voltage by input
// leakage. Estimate its error from an undriven path measured with the Z pull-up
// and the contact's Thevenin resistance (the same two-terminal path as READ_Z).
inline int32_t loadingError(uint16_t z, uint16_t openZ) {
  return ((uint32_t(openZ) + adcError) * (4095 - z + adcError) + z - adcError - 1) / (z - adcError);
}

inline Verdict compareTapVoltages(int a, int b, int c, uint16_t za, uint16_t zb, uint16_t openZ) {
  if (!usable(za) || !usable(zb) || !usable(a) || !usable(b) || !usable(c)) return inconclusive;
  int lo = a < c ? a : c, hi = a < c ? c : a;
  if (hi - lo > 2 * adcError) return inconclusive;
  int32_t tolerance = 2 * adcError + loadingError(za, openZ) + loadingError(zb, openZ);
  if (b < lo - tolerance || b > hi + tolerance) return present;
  return b >= hi - 2 * adcError && b <= lo + 2 * adcError ? absent : inconclusive;
}

} // namespace rectangleCircuit

#endif
