/*=====================================================================================================================
======================================== LinnStrument Operating System v1.2.0 =========================================
=======================================================================================================================

Operating System for the LinnStrument (c) music controller by Roger Linn Design (www.rogerlinndesign.com).

Written by Roger Linn and Geert Bevin (http://gbevin.com) with significant help by Tim Thompson (http://timthompson.com).

LinnStrument Operating System is licensed under a Creative Commons Attribution-ShareAlike 3.0 Unported License,
viewable at <http://creativecommons.org/licenses/by-sa/3.0/>.

You are free:
1) to Share — to copy, distribute and transmit the work
2) to Remix — to adapt the work

Under the following conditions:
1) Attribution — You must attribute the work in the manner specified by the author or licensor
(but not in any way that suggests that they endorse you or your use of the work).
2) Noncommercial — You may not use this work for commercial purposes.
3) Share Alike — If you alter, transform, or build upon this work, you may distribute the resulting work
only under the same or similar license to this one.

With the understanding that:
1) Waiver — Any of the above conditions can be waived if you get permission from the copyright holder.
2) Public Domain — Where the work or any of its elements is in the public domain under applicable law,
that status is in no way affected by the license.
3) Other Rights — In no way are any of the following rights affected by the license:
      a) Your fair dealing or fair use rights, or other applicable copyright exceptions and limitations;
      b) The author's moral rights;
      c) Rights other persons may have either in the work itself or in how the work is used, such as
      publicity or privacy rights.

Notice — For any reuse or distribution, you must make clear to others the license terms of this work.
The best way to do this is with a link to http://creativecommons.org/licenses/by-nc-sa/3.0/deed.en_US

For any questions about this, contact Roger Linn Design at support@rogerlinndesign.com.

=======================================================================================================================
=======================================================================================================================
=====================================================================================================================*/




/*************************************** INCLUDED LIBRARIES **************************************/
#include <SPI.h>
#include <limits.h>
#include <DueFlashStorage.h>

#include "ls_debug.h"
#include "ls_channelbucket.h"
#include "ls_midi.h"


/******************************************** CONSTANTS ******************************************/

char* OSVersion = "120.";

// SPI addresses
#define SPI_LEDS    10               // Arduino pin for LED control over SPI
#define SPI_SENSOR  4                // Arduino pin for touch sensor control over SPI
#define SPI_ADC     52               // Arduino pin for input from TI ADS7883 12-bit A/D converter

// Comment this define out to be able to compile against the standard Arduino API, but not
// benefit from our no-delay serial write improvements
#define PATCHED_ARDUINO_SERIAL_WRITE

// Uncomment this to use the code for the April 2014 board prototypes
// #define APRIL_2014_PROTOTYPE

// Uncomment to immediately start X, Y, or Z frame debugging when the LinnStrument launches
// This is useful when having to inspect the sensor data without being able to
// use the switches to change the active settings
// #define DISPLAY_XFRAME_AT_LAUNCH
// #define DISPLAY_YFRAME_AT_LAUNCH
// #define DISPLAY_ZFRAME_AT_LAUNCH
// #define DISPLAY_SURFACESCAN_AT_LAUNCH

// Touch surface constants
#define NUMCOLS  26                  // number of touch sensor columns
#define NUMROWS  8                   // number of touch sensor rows

#define NUMSPLITS  2                 // number of splits supported
#define LEFT       0
#define RIGHT      1

#define LOWEST_NOTE  30              // 30 = F#2, which is 10 semitones below guitar low E (E3/52). High E = E5/76

// Foot switch Arduino pins
#define FOOT_SW_LEFT   33
#define FOOT_SW_RIGHT  34

// Input options for setSwitches
#define READ_X  0
#define READ_Y  1
#define READ_Z  2

// Supported colors
#define COLOR_OFF      0
#define COLOR_RED      1
#define COLOR_YELLOW   2
#define COLOR_GREEN    3
#define COLOR_CYAN     4
#define COLOR_BLUE     5
#define COLOR_MAGENTA  6
#define COLOR_BLACK    7

#define LED_FLASH_DELAY  50000        // the time before a led is turned off when flashing or pulsing, in microseconds

// Differences for low power mode
#define LOWPOWER_LED_REFRESH      250   // accelerate led refresh so that they can be lit only half of the time
#define LOWPOWER_MIDI_DECIMATION  12    // use a decimation rate of 12 ms in low power mode

// Values related to the Z sensor, continuous pressure
#define DEFAULT_SENSOR_LO_Z        230                 // lowest acceptable raw Z value to start a touch
#define DEFAULT_SENSOR_FEATHER_Z   120                 // lowest acceptable raw Z value to continue a touch
#define DEFAULT_SENSOR_RANGE_Z     648                 // default range of the pressure
#define MAX_SENSOR_RANGE_Z         1016                // upper value of the pressure                          

#define MAX_TOUCHES_IN_COLUMN  3

// Pitch correction behavior
#define PITCH_CORRECT_HOLD_SAMPLES_FAST    80
#define PITCH_CORRECT_HOLD_SAMPLES_MEDIUM  4200
#define PITCH_CORRECT_HOLD_SAMPLES_SLOW    32000

// Threshold below which the average rate of change of X is considered 'stationary'
#define RATEX_THRESHOLD_FAST    3.5
#define RATEX_THRESHOLD_MEDIUM  2.2
#define RATEX_THRESHOLD_SLOW    1.5
#define RATEX_THRESHOLD_DEFAULT 2.2

#define SENSOR_PITCH_Z               173               // lowest acceptable raw Z value for which pitchbend is sent
#define ROGUE_PITCH_SWEEP_THRESHOLD  48                // the maximum threshold of instant X changes since the previous sample, anything higher will be considered a rogue pitch sweep

// The values here MUST be the same as the row numbers of the cells in per-split settings
#define MIDICHANNEL_MAIN     7
#define MIDICHANNEL_PERNOTE  6
#define MIDICHANNEL_PERROW   5

// The values for the different LED layers
#define LED_LAYER_MAIN      0
#define LED_LAYER_CUSTOM1   1
#define LED_LAYER_CUSTOM2   2
#define LED_LAYER_PLAYED    3
#define LED_LAYER_COMBINED  4
#define LED_LAYERS          4

// The values here MUST be the same as the row numbers of the cells in GlobalSettings
#define LIGHTS_MAIN    0
#define LIGHTS_ACCENT  1

// The values of SWITCH_ here MUST be the same as the row numbers of the cells used to set them.
#define SWITCH_FOOT_L    0
#define SWITCH_FOOT_R    1
#define SWITCH_SWITCH_2  2
#define SWITCH_SWITCH_1  3

#define ASSIGNED_OCTAVE_DOWN  0
#define ASSIGNED_OCTAVE_UP    1
#define ASSIGNED_SUSTAIN      2
#define ASSIGNED_CC_65        3
#define ASSIGNED_ARPEGGIATOR  4
#define ASSIGNED_ALTSPLIT     5
#define ASSIGNED_AUTO_OCTAVE  6

#define GLOBAL_SETTINGS_ROW  0
#define SPLIT_ROW            1
#define SWITCH_2_ROW         2
#define SWITCH_1_ROW         3
#define OCTAVE_ROW           4
#define VOLUME_ROW           5
#define PRESET_ROW           6
#define PER_SPLIT_ROW        7

#define SWITCH_HOLD_DELAY  500
#define SENSOR_HOLD_DELAY  300

#define EDIT_MODE_HOLD_DELAY  1000

const unsigned short ccFaderDefaults[8] = {1, 2, 3, 4, 5, 6, 7, 8};

/******************************************** VELOCITY *******************************************/

#define VELOCITY_SAMPLES       3
#define VELOCITY_ZERO_POINTS   1
#define VELOCITY_N             VELOCITY_SAMPLES + VELOCITY_ZERO_POINTS
#define VELOCITY_SUMX          10   // x1 + x2 + x3 + ... + xn
#define VELOCITY_SUMXSQ        30   // x1^2 + x2^2 + x3^2 + ... + xn^2
#define VELOCITY_SCALE_LOW     30
#define VELOCITY_SCALE_MEDIUM  34
#define VELOCITY_SCALE_HIGH    38


/****************************************** TOUCH TRACKING ***************************************/

// Current cell in the scan routine
byte sensorCol = 0;                         // currently read column in touch sensor
byte sensorRow = 0;                         // currently read row in touch sensor
byte sensorSplit = 0;                       // the split of the currently read touch sensor

// The most-recently touched cell within each channel of each split is said to have "focus",
// saved as the specific column and row for the focus cell.
// If in 1Ch/Poly mode, continuous X and Y messages are sent only from movements within the focused cell.
// If in 1Ch/Chan mode, continuous X, Y and Z messages are sent only from movements within the focused cell.
struct FocusCell {
  byte col;
  byte row;
};
FocusCell focusCell[NUMSPLITS][16];             // 2 splits and 16 MIDI channels for each split

enum TouchState {
  untouchedCell = 0,
  ignoredCell = 1,
  transferCell = 2,
  touchedCell = 3
};

struct TouchInfo {
  void shouldRefreshData();                  // indicate that the X, Y and Z data should be refreshed
  short rawX();                              // ensure that X is updated to the latest scan and return its raw value
  short calibratedX();                       // ensure that X is updated to the latest scan and return its calibrated value
  inline void refreshX();                    // ensure that X is updated to the latest scan
  short rawY();                              // ensure that Y is updated to the latest scan and return its raw value
  signed char calibratedY();                 // ensure that Y is updated to the latest scan and return its calibrated value
  inline void refreshY();                    // ensure that Y is updated to the latest scan
  short rawZ();                              // ensure that Z is updated to the latest scan and return its raw value
  inline boolean isMeaningfulTouch();        // ensure that Z is updated to the latest scan and check if it was a meaningful touch
  inline boolean isActiveTouch();            // ensure that Z is updated to the latest scan and check if it was an active touch
  inline boolean isStableYTouch();           // ensure that Z is updated to the latest scan and check if the touch is capable of providing stable Y reading
  inline void refreshZ();                    // ensure that Z is updated to the latest scan
  boolean hasNote();                         // check if a MIDI note is active for this touch
  void clearPhantoms();                      // clear the phantom coordinates
  void clearAllPhantoms();                   // clear the phantom coordinates of all the cells that are involved
  boolean hasPhantoms();                     // indicates whether there are phantom coordinates
  void setPhantoms(byte, byte, byte, byte);  // set the phantoom coordinates
  boolean isHigherPhantomPressure(short);    // checks whether this is a possible phantom candidate and has higher pressure than the argument
  void clearMusicalData();                   // clear the musical data
  void clearSensorData();                    // clears the measured sensor data
  boolean isCalculatingVelocity();           // indicates whether the initial velocity is being calculated

  // touch data
  TouchState touched;                        // touch status of all sensor cells
  unsigned long lastTouch;
  short initialX;                            // initial calibrated X value of each cell at the start of the touch
  short initialReferenceX;                   // initial calibrated reference X value of each cell at the start of the touch
  short quantizationOffsetX;                 // quantization offset to be applied to the X value
  short currentRawX;                         // last raw X value of each cell
  short currentCalibratedX;                  // last calibrated X value of each cell
  short lastMovedX;                          // the last X movement, so that we can compare movement jumps
  int32_t fxdLastMovedX;                     // the fixed precision version of the last moved X for performance improvement
  int32_t fxdRateX;                          // the averaged rate of change of the X values
  int32_t fxdRateCountX;                     // the number of times the rate of change drops below the minimal value for quantization
  boolean shouldRefreshX;                    // indicate whether it's necessary to refresh X

  signed char initialY;                      // initial Y value of each cell
  short currentRawY;                         // last raw Y value of each cell
  signed char currentCalibratedY;            // last calibrated Y value of each cell
  boolean shouldRefreshY;                    // indicate whether it's necessary to refresh Y

  short currentRawZ;                         // the raw Z value
  boolean featherTouch;                      // indicates whether this is a feather touch
  byte velocityZ;                            // the Z value with velocity sensitivity
  byte pressureZ;                            // the Z value with pressure sensitivity
  boolean shouldRefreshZ;                    // indicate whether it's necessary to refresh Z

  signed char pendingReleaseCount;           // counter before which the note release will be effective

  // phantom touch tracking
  signed char phantomCoords[4];              // stores the coordinates of a rectangle that possibly has a phantom touch, stored as column 1, column 2, row 1, row 2

  // musical data
  byte vcount;                               // the number of times the pressure was measured to obtain a velocity
  byte velocity;                             // velocity from 0 to 127
  signed char note;                          // note from 0 to 127
  signed char channel;                       // channel from 1 to 16
  signed char octaveOffset;                  // the octave offset when the note started, since this can change during playing
  int32_t fxdPrevPressure;                   // used to average out the rate of change of the pressure when transitioning between cells
  int32_t fxdPrevTimbre;                     // used to average out the rate of change of the timbre
  unsigned long velSumY;                     // these are used to calculate the intial velocity slope based on the first Z samples
  unsigned long velSumXY;
};
TouchInfo touchInfo[NUMCOLS][NUMROWS];       // store as much touch information instances as there are cells

int32_t rowsInColsTouched[NUMCOLS];          // keep track of which rows inside each column and which columns inside each row are touched, using a bitmask
int32_t colsInRowsTouched[NUMROWS];          // to makes it possible to quickly identify square formations that generate phantom presses
unsigned short cellsTouched;                 // counts the number of active touches on cells

struct VirtualTouchInfo {
  boolean hasNote();                         // check if a MIDI note is active for this touch
  void clearData();                          // clear the virtual touch data

  byte split;                                // the split this virtual touch belongs to
  byte velocity;                             // velocity from 0 to 127
  signed char note;                          // note from 0 to 127
  signed char channel;                       // channel from 1 to 16
};
VirtualTouchInfo virtualTouchInfo[NUMROWS];  // store as much touch virtual instances as there are rows, this is used for simulating strumming open strings

// Reverse mapping to find the touch information based on the MIDI note and channel,
// this is used for the arpeggiator to know which notes are active and which cells
// to look at for continuous velocity calculation
struct NoteEntry {
  byte colRow;
  signed char nextNote;
  signed char previousNote;
  byte nextPreviousChannel;

  inline boolean hasColRow(byte, byte);
  inline void setColRow(byte, byte);
  inline byte getCol();
  inline byte getRow();

  inline byte getNextNote();
  inline byte getNextChannel();
  inline byte getPreviousNote();
  inline byte getPreviousChannel();
  inline void setNextChannel(byte);
  inline void setPreviousChannel(byte);
};
struct NoteTouchMapping {
  void initialize();                                         // initialize the mapping data
  void noteOn(signed char, signed char, byte, byte);         // register the cell for which a note was turned on
  void noteOff(signed char, signed char);                    // turn off a note
  void changeCell(signed char, signed char, byte, byte);     // changes the cell of an active note
  boolean hasTouch(signed char, signed char);                // indicates whether there's a touch active for a particular note and channel
  inline NoteEntry* getNoteEntry(signed char, signed char);  // get the entry for a particular note and channel
  inline byte getMusicalTouchCount(signed char);             // the number of musical touches for a particular MIDI channel

  void debugNoteChain();

  unsigned short noteCount;
  unsigned musicalTouchCount[16];
  signed char firstNote;
  signed char firstChannel;
  signed char lastNote;
  signed char lastChannel;
  NoteEntry mapping[128][16];
};
NoteTouchMapping noteTouchMapping[NUMSPLITS];


/**************************************** DISPLAY STATE ******************************************/

enum CellDisplay {
  cellOff = 0,
  cellOn = 1,
  cellPulse = 2,
  cellSlowPulse = 3
};

enum DisplayMode {
  displayNormal,
  displayPerSplit,
  displayPreset,
  displayVolume,
  displayOctaveTranspose,
  displaySplitPoint,
  displayGlobal,
  displayGlobalWithTempo,
  displayOsVersion,
  displayCalibration,
  displayReset,
  displayBendRange,
  displayCCForY,
  displayCCForZ,
  displayCCForFader,
  displayLowRowCCXConfig,
  displayLowRowCCXYZConfig,
  displayCCForSwitch,
  displaySensorLoZ,
  displaySensorFeatherZ,
  displaySensorRangeZ,
  displayPromo,
  displayEditAudienceMessage
};
DisplayMode displayMode = displayNormal;


/***************************************** CALIBRATION *******************************************/

enum CalibrationPhase {
  calibrationInactive,
  calibrationRows,
  calibrationCols
};
byte calibrationPhase = calibrationInactive;

struct CalibrationSample {
  short minValue;
  short maxValue;
  int pass;
};
CalibrationSample calSampleRows[NUMCOLS][4]; // store four rows of calibration measurements
CalibrationSample calSampleCols[9][NUMROWS]; // store nine columns of calibration measurements

struct CalibrationX {
  int32_t fxdMeasuredX;
  int32_t fxdReferenceX;
  int32_t fxdRatio;
};

struct CalibrationY {
  int minY;
  int maxY;
  int32_t fxdRatio;
};


/***************************************** PANEL SETTINGS ****************************************/

enum LowRowMode {
  lowRowNormal,
  lowRowSustain,
  lowRowRestrike,
  lowRowStrum,
  lowRowArpeggiator,
  lowRowBend,
  lowRowCCX,
  lowRowCCXYZ
};

enum LowRowCCBehavior {
  lowRowCCHold = 0,
  lowRowCCFader = 1
};

enum MidiMode {
  oneChannel,
  channelPerNote,
  channelPerRow
};

enum BendRangeOption {
  bendRange2,
  bendRange3,
  bendRange12,
  bendRange24
};

enum PitchCorrectHoldSpeed {
  pitchCorrectHoldOff = 0,
  pitchCorrectHoldMedium = 1,
  pitchCorrectHoldFast = 2,
  pitchCorrectHoldSlow = 3
};

enum TimbreExpression {
  timbrePolyPressure,
  timbreChannelPressure,
  timbreCC1,
  timbreCC74,
};

enum LoudnessExpression {
  loudnessPolyPressure,
  loudnessChannelPressure,
  loudnessCC11
};

// per-split settings
struct SplitSettings {
  byte midiMode;                       // 0 = one channel, 1 = note per channel, 2 = row per channel
  byte midiChanMain;                   // main midi channel, 1 to 16
  byte midiChanPerRow;                 // per-row midi channel, 1 to 16
  boolean midiChanSet[16];             // Indicates whether each channel is used.  If midiMode!=channelPerNote, only one channel can be set.
  BendRangeOption bendRangeOption;     // see BendRangeOption
  byte customBendRange;                // 1 - 96
  boolean sendX;                       // true to send continuous X, false if not
  boolean sendY;                       // true to send continuous Y, false if not
  boolean sendZ;                       // true to send continuous Z, false if not
  boolean pitchCorrectQuantize;        // true to quantize pitch of initial touch, false if not
  byte pitchCorrectHold;               // See PitchCorrectHoldSpeed values
  boolean pitchResetOnRelease;         // true to enable pitch bend being set back to 0 when releasing a touch
  TimbreExpression expressionForY;     // the expression that should be used for timbre
  unsigned short customCCForY;         // 0-129 (with 128 and 129 being placeholders for PolyPressure and ChannelPressure)
  boolean relativeY;                   // true when Y should be sent relative to the initial touch, false when it's absolute
  LoudnessExpression expressionForZ;   // the expression that should be used for loudness
  unsigned short customCCForZ;         // 0-127
  unsigned short ccForFader[8];        // each fader can control a CC number ranging from 0-127
  byte colorMain;                      // color for non-accented cells
  byte colorAccent;                    // color for accented cells
  byte colorNoteon;                    // color for played notes
  byte colorLowRow;                    // color for low row if on
  byte lowRowMode;                     // see LowRowMode values
  byte lowRowCCXBehavior;              // see LowRowCCBehavior values
  unsigned short ccForLowRow;          // 0-127
  byte lowRowCCXYZBehavior;            // see LowRowCCBehavior values
  unsigned short ccForLowRowX;         // 0-99
  unsigned short ccForLowRowY;         // 0-99
  unsigned short ccForLowRowZ;         // 0-99
  signed char transposeOctave;         // -60, -48, -36, -24, -12, 0, +12, +24, +36, +48, +60
  signed char transposePitch;          // transpose output midi notes. Range is -12 to +12
  signed char transposeLights;         // transpose lights on display. Range is -12 to +12
  boolean ccFaders;                    // true to activated 8 CC faders for this split, false for regular music performance
  boolean arpeggiator;                 // true when the arpeggiator is on, false if notes should be played directly
  boolean strum;                       // true when this split strums the touches of the other split
  boolean mpe;                         // true when MPE is active for this split
};
SplitSettings Split[NUMSPLITS];

struct DeviceSettings {
  byte version;                              // the version of the configuration format
  boolean serialMode;                        // 0 = normal MIDI I/O, 1 = Arduino serial mode for OS update and serial monitor
  CalibrationX calRows[NUMCOLS+1][4];        // store four rows of calibration data
  CalibrationY calCols[9][NUMROWS];          // store nine columns of calibration data
  boolean calibrated;                        // indicates whether the calibration data actually resulted from a calibration operation
  unsigned short sensorLoZ;                  // the lowest acceptable raw Z value to start a touch
  unsigned short sensorFeatherZ;             // the lowest acceptable raw Z value to continue a touch
  unsigned short sensorRangeZ;               // the maximum raw value of Z
  boolean promoAnimationAtStartup;           // store whether the promo animation should run at startup
  char audienceMessages[16][31];             // the 16 audience messages that will scroll across the surface
  boolean operatingLowPower;                 // whether low power mode is active or not
};
DeviceSettings Device;

// The values here MUST match the row #'s for the leds that get lit up in GlobalSettings
enum VelocitySensitivity {
  velocityLow,
  velocityMedium,
  velocityHigh,
  velocityFixed
};

// The values here MUST match the row #'s for the leds that get lit up in GlobalSettings
enum PressureSensitivity {
  pressureLow,
  pressureMedium,
  pressureHigh
};

enum ArpeggiatorStepTempo {
  ArpFourth = 0,
  ArpEighth = 1,
  ArpEighthTriplet = 2,
  ArpSixteenth = 3,
  ArpSixteenthSwing = 4,
  ArpSixteenthTriplet = 5,
  ArpThirtysecond = 6,
  ArpThirtysecondTriplet = 7,
  ArpSixtyfourthTriplet = 8,
};

enum ArpeggiatorDirection {
  ArpUp,
  ArpDown,
  ArpUpDown,
  ArpRandom,
  ArpReplayAll
};

struct GlobalSettings {
  void setSwitchAssignment(byte, byte);

  byte splitPoint;                           // leftmost column number of right split (0 = leftmost column of playable area)
  byte currentPerSplit;                      // controls which split's settings are being displayed
  boolean mainNotes[12];                     // determines which notes receive "main" lights
  boolean accentNotes[12];                   // determines which notes receive accent lights (octaves, white keys, black keys, etc.)
  byte rowOffset;                            // interval between rows. 0 = no overlap, 1-12 = interval, 13 = guitar
  VelocitySensitivity velocitySensitivity;   // See VelocitySensitivity values
  PressureSensitivity pressureSensitivity;   // See PressureSensitivity values
  boolean pressureAftertouch;                // Indicates whether pressure should behave like traditional piano keyboard aftertouch or be continuous from the start
  byte switchAssignment[4];                  // The element values are ASSIGNED_*.  The index values are SWITCH_*.
  boolean switchBothSplits[4];               // Indicate whether the switches should operate on both splits or only on the focused one
  unsigned short ccForSwitch;                // 0-127
  byte midiIO;                               // 0 = MIDI jacks, 1 = USB
  ArpeggiatorDirection arpDirection;         // the arpeggiator direction that has to be used for the note sequence
  ArpeggiatorStepTempo arpTempo;             // the multiplier that needs to be applied to the current tempo to achieve the arpeggiator's step duration
  signed char arpOctave;                     // the number of octaves that the arpeggiator has to operate over: 0, +1, or +2
};
GlobalSettings Global;

struct PresetSettings {
  GlobalSettings global;
  SplitSettings split[NUMSPLITS];
};

#define NUMPRESETS 4 
struct Configuration {
  DeviceSettings device;
  PresetSettings settings;
  PresetSettings preset[NUMPRESETS];
};
struct Configuration config;


/**************************************** SECRET SWITCHES ****************************************/

#define SECRET_SWITCHES 5
#define SWITCH_DEBUGMIDI secretSwitch[0]
#define SWITCH_XFRAME secretSwitch[1]
#define SWITCH_YFRAME secretSwitch[2]
#define SWITCH_ZFRAME secretSwitch[3]
#define SWITCH_SURFACESCAN secretSwitch[4]

boolean secretSwitch[SECRET_SWITCHES];  // The secretSwitch* values are controlled by cells in column 18


/***************************************** OPERATING MODE ****************************************/

enum OperatingMode {
  modePerformance,
  modeManufacturingTest,
  modeFirmware
};
OperatingMode operatingMode = modePerformance;


/**************************************** FIXED POINT MATH ***************************************/

#define FXD_FBITS        8
#define FXD_FROM_INT(a)  (int32_t)((a) << FXD_FBITS)
#define FXD_MAKE(a)      (int32_t)((a*(1 << FXD_FBITS)))

inline int FXD_TO_INT(int32_t a) {
  a = a + ((a & (int32_t)1 << (FXD_FBITS-1)) << 1);   // rounding instead of truncation
  return ((a) >> FXD_FBITS);
}

inline int32_t FXD_MUL(int32_t a, int32_t b) {
  int32_t t = a * b;
  t = t + ((t & (int32_t)1 << (FXD_FBITS-1)) << 1);   // rounding instead of truncation
  return t >> FXD_FBITS;
}

inline int32_t FXD_DIV(int32_t a, int32_t b) {
  return ((int32_t)a << FXD_FBITS) / (int32_t)b;
}

// these macros have lower precision, but can be used for larger numbers when doing mult and div operations

#define FXD4_FBITS        4
#define FXD4_FROM_INT(a)  (int32_t)((a) << FXD4_FBITS)
#define FXD4_MAKE(a)      (int32_t)((a*(1 << FXD4_FBITS)))

inline int FXD4_TO_INT(int32_t a) {
  a = a + ((a & (int32_t)1 << (FXD4_FBITS-1)) << 1);   // rounding instead of truncation
  return ((a) >> FXD4_FBITS);
}

inline int32_t FXD4_MUL(int32_t a, int32_t b) {
  int32_t t = a * b;
  t = t + ((t & (int32_t)1 << (FXD4_FBITS-1)) << 1);   // rounding instead of truncation
  return t >> FXD4_FBITS;
}

inline int32_t FXD4_DIV(int32_t a, int32_t b) {
  return ((int32_t)a << FXD4_FBITS) / (int32_t)b;
}

const int32_t FXD_CONST_1 = FXD_FROM_INT(1);
const int32_t FXD_CONST_2 = FXD_FROM_INT(2);
const int32_t FXD_CONST_3 = FXD_FROM_INT(3);
const int32_t FXD_CONST_100 = FXD_FROM_INT(100);
const int32_t FXD_CONST_127 = FXD_FROM_INT(127);


/*************************************** CONVENIENCE MACROS **************************************/

// convenience macros to easily access the cells with touch information
#define sensorCell()               touchInfo[sensorCol][sensorRow]
#define cell(col, row)             touchInfo[col][row]
#define virtualCell()              virtualTouchInfo[sensorRow]

// calculate the difference between now and a previous timestamp, taking a possible single overflow into account
#define calcTimeDelta(now, last)   (now < last ? now + ~last : now - last)

// obtain the focused cell for a channel in a asplit
#define focus(split, channel)      focusCell[split][channel - 1]


/*************************************** OTHER RUNTIME STATE *************************************/

DueFlashStorage dueFlashStorage;                    // access to the persistent flash storage

boolean setupDone = false;                          // indicates whether the setup routine is finished

signed char debugLevel = -1;                        // level of debug messages that should be printed
boolean firstTimeBoot = false;                      // this will be true when the LinnStrument booted up the first time after a firmware upgrade
unsigned long lastReset;                            // the last time a reset was started

byte globalColor = COLOR_BLUE;                      // color for global, split point and transpose settings

boolean changedSplitPoint = false;                  // reflects whether the split point was changed
boolean splitButtonDown = false;                    // reflects state of Split button

signed char controlButton = -1;                     // records the row of the current controlButton being held down
unsigned long lastControlPress[NUMROWS];

unsigned long ledRefreshInterval = 500;             // LED timing
unsigned long prevLedTimerCount;                    // timer for refreshing leds
unsigned long prevGlobalSettingsDisplayTimerCount;  // timer for refreshing the global settings display

ChannelBucket splitChannels[NUMSPLITS];             // the MIDI channels that are being handed out
unsigned short midiPreset[NUMSPLITS];               // preset number 0-127
byte ccFaderValues[NUMSPLITS][128];                 // the current values of the CC faders
byte currentEditedCCFader[NUMSPLITS];               // the current CC fader number that is being edited
signed char arpTempoDelta[NUMSPLITS];               // ranges from -24 to 24 to apply a speed difference to the selected arpeggiator speed

unsigned long lastSwitchPress[4];
boolean switchState[4][NUMSPLITS];
byte switchTargetEnabled[7][NUMSPLITS];             // 7 targets, we keep track of them individually for each split and how many times they're active
boolean footSwitchState[2];                         // holds the last read footswitch state, so that we only react on state changes of the input signal
boolean footSwitchOffState[2];                      // holds the OFF state of foot switch, read at startup, thereby permit normally-closed or normally-open switches
unsigned long prevFootSwitchTimerCount;             // time interval (in microseconds) between foot switch reads

byte focusedSplit = LEFT;                           // the split that currently has focus, either by taking up the whole instrument or by being played last
boolean splitActive = false;                        // false = split off, true = split on
boolean doublePerSplit = false;                     // false when only one per split is active, true if they both are

byte switchSelect = SWITCH_FOOT_L;                  // determines which switch setting is being displayed/changed
byte midiChannelSelect = MIDICHANNEL_MAIN;          // determines which midi channel setting is being displayed/changed
byte lightSettings = LIGHTS_MAIN;                   // determines which Lights array is being displayed/changed

boolean userFirmwareActive = false;                 // indicates whether user firmware mode is active or not
boolean userFirmwareSlideMode[NUMROWS];             // indicates whether slide mode is on for a particular row
boolean userFirmwareXActive[NUMROWS];               // indicates whether X data is on for a particular row
boolean userFirmwareYActive[NUMROWS];               // indicates whether Y data is on for a particular row
boolean userFirmwareZActive[NUMROWS];               // indicates whether Z data is on for a particular row

boolean animationActive = false;                    // indicates whether animation is active, preventing any other display
boolean stopAnimation = false;                      // indicates whether animation should be stopped

int32_t fxd4CurrentTempo = FXD4_FROM_INT(120);      // the current tempo
byte midiDecimateRate = 0;                          // by default no decimation
byte lastValueMidiNotesOn[NUMSPLITS][128][16];      // for each split, keep track of MIDI note on to filter out note off messages that are not needed
unsigned short pitchHoldDuration[NUMSPLITS];        // for each split the actual pitch hold duration in samples
int32_t fxdPitchHoldDuration[NUMSPLITS];
int32_t fxdRateXThreshold[NUMSPLITS];               // the threshold below which the average rate of change of X is considered 'stationary' and pitch hold quantization will start to occur
int latestNoteNumberForAutoOctave = -1;             // keep track of the latest note number that was generated to use for auto octave switching

byte audienceMessageToEdit = 0;                     // the audience message to edit with that mode is active
short audienceMessageOffset = 0;                    // the offset in columns for printing the edited audience message
short audienceMessageLength = 0;                    // the length in pixels of the audience message to edit

byte lowRowCCXConfigState = 1;                      // the last state of the advanced low row CCX configuration, this counts down to go to further pages
byte lowRowCCXYZConfigState = 3;                    // the last state of the advanced low row CCXYZ configuration, this counts down to go to further pages

unsigned long presetBlinkStart[NUMPRESETS];         // the moments at which the preset LEDs started blinking


/************************* FUNCTION DECLARATIONS TO WORK AROUND COMPILER *************************/

void setLed(byte col, byte row, byte color, CellDisplay disp);
void setLed(byte col, byte row, byte color, CellDisplay disp, byte layer);

boolean ensureCellBeforeHoldWait(byte resetColor, CellDisplay resetDisplay);

void setDisplayMode(DisplayMode mode);
void exitDisplayMode(DisplayMode mode);

void applyPresetSettings(PresetSettings& preset);
void applyBendRange(SplitSettings& target, byte bendRange);

void cellTouched(TouchState state);
void cellTouched(byte col, byte row, TouchState state);


/********************************************** SETUP ********************************************/

void reset() {
  lastReset = millis();

  Global.currentPerSplit = LEFT;
  focusedSplit = Global.currentPerSplit;
  splitActive = false;

  controlButton = -1;
  for (byte i = 0; i < NUMROWS; ++i) {
    lastControlPress[i] = 0;
  }

  initializeLeds();

  initializeTouchInfo();

  initializeLowRowState();

  initializeDeviceSettings();

  initializePresetSettings();

  initializeArpeggiator();

  initializeLastMidiTracking();

  initializeSwitches();
}

boolean switchPressAtStartup(byte switchRow) {
  sensorCol = 0;
  sensorRow = switchRow;
  // initially we need read Z a few times for the readings to stabilize
  readZ(); readZ(); unsigned short switchZ = readZ();
  if (switchZ > Device.sensorLoZ + 128) {
    return true;
  }
  return false;
}

void applyLowPowerMode() {
  // change the behavior for low power mode
  if (Device.operatingLowPower) {
    ledRefreshInterval = LOWPOWER_LED_REFRESH;
    midiDecimateRate = LOWPOWER_MIDI_DECIMATION;
  }
}

void applyMpeMode() {
  for (byte s = 0; s < NUMSPLITS; ++s) {
    if (Split[s].mpe) {
      midiSendMpeState(Split[s].midiChanMain, countMpePolyphonny(s));
      midiSendMpePitchBendRange(s);
    }
  }
}

void setup() {
  //*************************************************************************************************************************************************
  //**************** IMPORTANT, DONT CHANGE ANYTHING REGARDING THIS CODE BLOCK AT THE RISK OF BRICKING THE LINNSTRUMENT !!!!! ***********************
  //*************************************************************************************************************************************************
  /*!!*/
  /*!!*/  initializeDeviceSensorSettings();
  /*!!*/
  /*!!*/  // Initialize output pin 35 (midi/SERIAL) and set it HIGH for serial operation
  /*!!*/  // IMPORTANT: IF YOU UPLOAD DEBUG CODE THAT DISABLES THE UI'S ABILITY TO SET THIS BACK TO SERIAL MODE, YOU WILL BRICK THE LINNSTRUMENT!!!!!
  /*!!*/  pinMode(35, OUTPUT);
  /*!!*/  digitalWrite(35, HIGH);
  /*!!*/
  /*!!*/  // Enabled DIN MIDI output first since otherwise the DIN output will have a continuous stream of junk data if USB would be active from the
  /*!!*/  // startup of the LinnStrument
  /*!!*/  pinMode(36, OUTPUT);
  /*!!*/  digitalWrite(36, LOW);                             // set to use DIN jack
  /*!!*/
  /*!!*/  // Initialize output pin 36 (din/USB) and set it HIGH for USB operation
  /*!!*/  // IMPORTANT: IF YOU UPLOAD DEBUG CODE THAT DISABLES THE UI'S ABILITY TO SET THIS BACK TO USB MODE, YOU WILL BRICK THE LINNSTRUMENT!!!!!
  /*!!*/  pinMode(36, OUTPUT);
  /*!!*/  digitalWrite(36, HIGH);                             // set to use USB jack
  /*!!*/
  /*!!*/  // initialize the SPI port for setting one column of LEDs
  /*!!*/  SPI.begin(SPI_LEDS);
  /*!!*/  SPI.setDataMode(SPI_LEDS, SPI_MODE0);
  /*!!*/  SPI.setClockDivider(SPI_LEDS, 4);                   // max clock is about 20 mHz. 4 = 21 mHz. Transferring all 4 bytes takes 1.9 uS.
  /*!!*/
  /*!!*/  // initialize the SPI port for setting analog switches in touch sensor
  /*!!*/  SPI.begin(SPI_SENSOR);
  /*!!*/  SPI.setDataMode(SPI_SENSOR, SPI_MODE0);
  /*!!*/  SPI.setClockDivider(SPI_SENSOR, 4);                 // set clock speed to 84/4 = 21 mHz. Max clock is 25mHz @ 4.5v
  /*!!*/  selectSensorCell(0, 0, READ_Z);                     // set it analog switches to read column 0, row 0 and to read pressure
  /*!!*/
  /*!!*/  // initialize the SPI input port for reading the TI ADS7883 ADC
  /*!!*/  SPI.begin(SPI_ADC);
  /*!!*/  SPI.setDataMode(SPI_ADC, SPI_MODE0);
  /*!!*/  SPI.setClockDivider(SPI_ADC, 4);                    // set speed to 84/4 = 21 mHz. Max clock for ADC is 32 mHz @ 2.7-4.5v, 48mHz @ 4.5-5.5v
  /*!!*/
  /*!!*/  // Initialize the output enable line for the 2 LED display chips
  /*!!*/  pinMode(37, OUTPUT);
  /*!!*/  digitalWrite(37, HIGH);
  /*!!*/
  /*!!*/  if (switchPressAtStartup(0)) {
  /*!!*/    // if the global settings and switch 2 buttons are pressed at startup, the LinnStrument will do a global reset
  /*!!*/    if (switchPressAtStartup(2)) {
  /*!!*/      dueFlashStorage.write(0, 1);
  /*!!*/    }
  /*!!*/    // if only the global settings button is pressed at startup, activatate firmware upgrade mode
  /*!!*/    else {
  /*!!*/      operatingMode = modeFirmware;
  /*!!*/  
  /*!!*/      // use serial and not MIDI
  /*!!*/      pinMode(35, OUTPUT);
  /*!!*/      digitalWrite(35, HIGH);
  /*!!*/  
  /*!!*/      // use USB connection and not DIN
  /*!!*/      pinMode(36, OUTPUT);
  /*!!*/      digitalWrite(36, HIGH);
  /*!!*/  
  /*!!*/      clearDisplay();
  /*!!*/  
  /*!!*/      bigfont_draw_string(0, 0, "FWUP", COLOR_RED);
  /*!!*/      return;
  /*!!*/    }
  /*!!*/  }
  /*!!*/
  //*************************************************************************************************************************************************

  // initialize input pins for 2 foot switches
  pinMode(FOOT_SW_LEFT, INPUT_PULLUP);
  pinMode(FOOT_SW_RIGHT, INPUT_PULLUP);
  
  // initialize the calibration data for it to be a no-op, unless it's loaded from a previous calibration sample result
  initializeCalibrationData();

  reset();

  // setup system timers for interval between LED column refreshes and foot switch reads
  prevLedTimerCount = prevFootSwitchTimerCount = prevGlobalSettingsDisplayTimerCount = micros();

  // detect if test mode is active by holding down the per-split button at startup
  if (switchPressAtStartup(7)) {
    operatingMode = modeManufacturingTest;

    // Disable serial mode
    digitalWrite(35, LOW);

    // Set the MIDI I/O to DIN
    Global.midiIO = 0;
    digitalWrite(36, LOW);
    Serial.begin(31250);
    Serial.flush();
    return;
  }

  // default to performance mode
  sensorCol = 0;
  sensorRow = 0;
  {
    operatingMode = modePerformance;

    // set display to normal performance mode & refresh it
    clearDisplay();
    setDisplayMode(displayNormal);
    setLed(0, SPLIT_ROW, globalColor, splitActive ? cellOn : cellOff);

    // perform some initialization
    initializeCalibrationSamples();

    initializeStorage();

    applyConfiguration();

    for (byte ss=0; ss<SECRET_SWITCHES; ++ss) {
      secretSwitch[ss] = false;
    }

    // detect if low power mode is toggled by holding down the octave/transpose button at startup
    if (switchPressAtStartup(4)) {
      Device.operatingLowPower = true;
      sensorCell().touched = touchedCell;
      storeSettings();
    }

    applyLowPowerMode();

    applyMpeMode();

    // update the display for the last state
    updateDisplay();
  }

  // if the promo animation was running last time the LinnStrument was on, start it up automatically
  if (Device.promoAnimationAtStartup) {
    playPromoAnimation();
  }

#ifdef DISPLAY_XFRAME_AT_LAUNCH
  #define DEBUG_ENABLED
  Device.serialMode = true;
  SWITCH_XFRAME = true;
#endif

#ifdef DISPLAY_YFRAME_AT_LAUNCH
  #define DEBUG_ENABLED
  Device.serialMode = true;
  SWITCH_YFRAME = true;
#endif

#ifdef DISPLAY_ZFRAME_AT_LAUNCH
  #define DEBUG_ENABLED
  Device.serialMode = true;
  SWITCH_ZFRAME = true;
#endif

#ifdef DISPLAY_SURFACESCAN_AT_LAUNCH
  #define DEBUG_ENABLED
  Device.serialMode = true;
  SWITCH_SURFACESCAN = true;
#endif

  setupDone = true;
}


/******************************* MAIN LOOP *****************************/

// loop:
// Main execution loop for LinnStrument OS
// Read Z (pressure) for each of the 200 row/column intersections. If a cell is touched, send Note On messages and also read
// continuous Z, X and Y (if each is enabled), and send resulting continuous MIDI messages. After each cell rad, check the timer
// and refresh the next LED column and read foot switches as necessary.

void loop() {
  // the default musical performance mode
  if (operatingMode == modePerformance) {
    modeLoopPerformance();
  }
  // manufactoring test mode where leds are shows for specific signals
  else if (operatingMode == modeManufacturingTest) {
    modeLoopManufacturingTest();
  }
  // firmware update mode that does nothing else but updating the leds and
  // waiting for a firmware update
  else if (operatingMode == modeFirmware) {
    checkRefreshLedColumn(micros());
  }
}

inline void modeLoopPerformance() {
  if (displayMode == displayReset) {                             // if reset is active, don't process any input data
    if (calcTimeDelta(millis(), lastReset) > 3000) {             // restore normal operations three seconds after the reset started
      applyConfiguration();
      storeSettings();
      setDisplayMode(displayNormal);                             // this should make the reset operation feel more predictable
      updateDisplay();
    }
  }
  else {
    TouchState previousTouch = sensorCell().touched;                              // get previous touch status of this cell

    boolean canShortCircuit = false;

    if (previousTouch != touchedCell && previousTouch != ignoredCell &&
        sensorCell().isMeaningfulTouch()) {                                       // if touched now but not before, it's a new touch
      handleNewTouch();
      canShortCircuit = true;
    }
    else if (previousTouch == touchedCell && sensorCell().isActiveTouch()) {      // if touched now and touched before
      handleXYZupdate();                                                          // handle any X, Y or Z movements
      canShortCircuit = true;
    }
    else if (previousTouch != untouchedCell && !sensorCell().isActiveTouch() &&   // if not touched now but touched before, it's been released
             calcTimeDelta(millis(), sensorCell().lastTouch) > 70 ) {             // only release if it's later than 70ms after the touch to debounce some note starts
      handleTouchRelease();
    }

    if (canShortCircuit &&
        sensorCell().touched == touchedCell &&
        sensorCell().isCalculatingVelocity()) {                                   // if the initial velocity is being calculated, ensure that only Z data is being refresh and
      sensorCell().shouldRefreshData();                                           // immediately process this cell again without going through a full surface scan
      return;
    }

    handleQuantizeHoldForOtherCells();
  }

  performContinuousTasks(micros());

#ifdef DEBUG_ENABLED
  if (SWITCH_XFRAME) displayXFrame();                            // Turn on secret switch to display the X value of all cells in grid at the end of each total surface scan
  if (SWITCH_YFRAME) displayYFrame();                            // Turn on secret switch to display the Y value of all cells in grid at the end of each total surface scan
  if (SWITCH_ZFRAME) displayZFrame();                            // Turn on secret switch to display the pressure value of all cells in grid at the end of each total surface scan
  if (SWITCH_SURFACESCAN) displaySurfaceScanTime();              // Turn on secret switch to display the total time for a total surface scan
#endif

  nextSensorCell();                                              // done-- move on to the next sensor cell
}
