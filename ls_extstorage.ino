/***************************** ls_extstorage: LinnStrument Settings *******************************
This work is licensed under the Creative Commons Attribution-ShareAlike 3.0 Unported License.
To view a copy of this license, visit http://creativecommons.org/licenses/by-sa/3.0/
or send a letter to Creative Commons, PO Box 1866, Mountain View, CA 94042, USA.
***************************************************************************************************
**************************************************************************************************/

/**************************************** Configuration V1 ***************************************/
struct SplitSettingsV1 {
  byte midiMode;                       // 0 = one channel, 1 = note per channel, 2 = row per channel
  byte midiChanMain;                   // main midi channel, 1 to 16
  byte midiChanPerRow;                 // per-row midi channel, 1 to 16
  boolean midiChanSet[16];             // Indicates whether each channel is used.  If midiMode!=channelPerNote, only one channel can be set.
  byte bendRange;                      // 1 - 96
  boolean sendX;                       // true to send continuous X, false if not
  boolean sendY;                       // true to send continuous Y, false if not
  boolean sendZ;                       // true to send continuous Z, false if not
  boolean pitchCorrectQuantize;        // true to quantize pitch of initial touch, false if not
  boolean pitchCorrectHold;            // true to quantize pitch when note is held, false if not
  boolean pitchResetOnRelease;         // true to enable pitch bend being set back to 0 when releasing a touch
  unsigned short ccForY;               // 0-127
  boolean relativeY;                   // true when Y should be sent relative to the initial touch, false when it's absolute
  LoudnessExpression expressionForZ;   // the expression that should be used for loudness
  unsigned short ccForZ;               // 0-127
  byte colorMain;                      // color for non-accented cells
  byte colorAccent;                    // color for accented cells
  byte colorNoteon;                    // color for played notes
  byte colorLowRow;                    // color for low row if on
  byte lowRowMode;                     // see LowRowMode values
  unsigned short preset;               // preset number 0-127
  signed char transposeOctave;         // -60, -48, -36, -24, -12, 0, +12, +24, +36, +48, +60
  signed char transposePitch;          // transpose output midi notes. Range is -12 to +12
  signed char transposeLights;         // transpose lights on display. Range is -12 to +12
  boolean ccFaders;                    // true to activated 8 CC faders for this split, false for regular music performance
  boolean arpeggiator;                 // true when the arpeggiator is on, false if notes should be played directly
  boolean strum;                       // true when this split strums the touches of the other split
};
struct ConfigurationV1 {
  GlobalSettings  global;
  SplitSettingsV1 left;
  SplitSettingsV1 right;
};
struct ConfigurationV1 configV1;
/*************************************************************************************************/

const char* countDownCode = "5, 4, 3, 2, 1 ...\n";
const byte countDownLength = 18;
const char* linnGoCode = "LinnStruments are go!\n"; 
const char* ackCode = "ACK\n";

boolean waitingForCommands = false;

enum linnCommands {
  SendSettings = 's',
  RestoreSettings = 'r'
};

byte codePos = 0;

void handleExtStorage() {
  // if no serial data is available, return
  if (Serial.available() <= 0) {
    return;
  }

  // get the next byte from the serial bus
  byte d = Serial.read();

  // check for a recognized command
  if (waitingForCommands) {
    switch (d) {
      case SendSettings:
      {
        Serial.write(ackCode);

        clearDisplay();
        delayUsec(1000);

        config.global = Global;
        config.left = Split[LEFT];
        config.right = Split[RIGHT];

        int32_t confSize = sizeof(struct Configuration);

        // send the size of the settings
        byte buff1[sizeof(int32_t)];
        memcpy(buff1, &confSize, sizeof(int32_t));
        Serial.write(buff1, sizeof(int32_t));

        // send the actual settings
        byte buff2[confSize];
        memcpy(buff2, &config, confSize);
        Serial.write(buff2, confSize);

        Serial.write(ackCode);

        break;
      }

      case RestoreSettings:
      {
        Serial.write(ackCode);

        clearDisplay();
        delayUsec(1000);

        // retrieve the size of the settings
        uint32_t lastMoment = millis();

        byte buff1[sizeof(int32_t)];
        for (byte i = 0; i < 4; ++i) {
          // retry if there's no data available
          while (Serial.available() <= 0) {
            // timeout after 2s if no data is coming in anymore
            if (calcTimeDelta(millis(), lastMoment) > 2000) {
              waitingForCommands = false;
              return;
            }
          }

          // read the next byte of the configuration size
          buff1[i] = Serial.read();
          lastMoment = millis();
        }

        int32_t confSize;
        memcpy(&confSize, buff1, sizeof(int32_t));

        // retrieve the actual settings
        lastMoment = millis();
        byte buff2[confSize];
        for (unsigned j = 0; j < confSize; ++j) {
          if (j % 32 == 0) {
            Serial.write(ackCode);
          }
          
          // retry if there's no data available
          while (Serial.available() <= 0) {
            // timeout after 2s if no data is coming in anymore
            if (calcTimeDelta(millis(), lastMoment) > 2000) {
              waitingForCommands = false;
              return;
            }
          }

          // read the next byte of the configuration data
          buff2[j] = Serial.read();
          lastMoment = millis();
        }

        boolean settingsApplied = false;
        byte settingsVersion = buff2[0];
        // if the stored version is newer than what this firmware supports, resort to default settings
        if (settingsVersion > config.global.version) {
          settingsApplied = false;
        }
        // if this is v1 of the configuration format, load it in the old structure and then convert it if the size is right
        else if (settingsVersion == 1 && confSize == sizeof(struct ConfigurationV1)) {
          memcpy(&configV1, buff2, confSize);

          byte currentVersion = config.global.version;
          config.global = configV1.global;
          config.global.version = currentVersion;
          copySplitSettingsV1ToSplitSettings(&config.left, &configV1.left);
          copySplitSettingsV1ToSplitSettings(&config.right, &configV1.right);
          settingsApplied = true;
        }
        // this is the v2 of the configuration configuration, apply it if the size is right
        else if (settingsVersion == 2 && confSize == sizeof(struct Configuration)) {
          memcpy(&config, buff2, confSize);
          settingsApplied = true;
        }

        // activate the retrieved settings
        if (settingsApplied) {
          applyConfiguration();
        }

        // send the acknowledgement of success
        Serial.write(ackCode);
        delayUsec(1000000);

        // Turn off OS upgrade mode
        switchSerialMode(false);

        // Enable normal playing mode and ensure calibration is fully turned off
        if (settingsApplied) {
          setDisplayMode(displayNormal);
          controlButton = -1;
          clearLed(0, GLOBAL_SETTINGS_ROW);

          storeSettings();
        }
        // turn on calibration instead of no new settings were applied and default settings are used
        else {
          setDisplayMode(displayCalibration);
          controlButton = 0;
          lightLed(0, GLOBAL_SETTINGS_ROW);
        }

        updateDisplay();
        waitingForCommands = false;

        break;
      }

      default:
      {
        waitingForCommands = false;
        break;
      }
    }
  }
  // handle readyness countdown state
  else {
    if (d == countDownCode[codePos]) {
      codePos++;
      if (codePos == countDownLength) {
        codePos = 0;
        waitingForCommands = true;
        Serial.write(linnGoCode);
      }
    }
    else {
      codePos = 0;
    }
  }
}

void copySplitSettingsV1ToSplitSettings(void *target, void *source) {
  SplitSettings *t = (SplitSettings *)target;
  SplitSettingsV1 *s = (SplitSettingsV1 *)source;

  t->midiMode = s->midiMode;
  t->midiChanMain = s->midiChanMain;
  t->midiChanPerRow = s->midiChanPerRow;
  memcpy(t->midiChanSet, s->midiChanSet, sizeof(boolean)*8);
  t->bendRange = s->bendRange;
  t->sendX = s->sendX;
  t->sendY = s->sendY;
  t->sendZ = s->sendZ;
  t->pitchCorrectQuantize = s->pitchCorrectQuantize;
  t->pitchCorrectHold = s->pitchCorrectHold;
  t->pitchResetOnRelease = s->pitchResetOnRelease;
  t->expressionForY = timbreCC;
  t->ccForY = s->ccForY;
  t->relativeY = s->relativeY;
  t->expressionForZ = s->expressionForZ;
  t->ccForZ = s->ccForZ;
  t->colorMain = s->colorMain;
  t->colorAccent = s->colorAccent;
  t->colorNoteon = s->colorNoteon;
  t->colorLowRow = s->colorLowRow;
  t->lowRowMode = s->lowRowMode;
  t->preset = s->preset;
  t->transposeOctave = s->transposeOctave;
  t->transposePitch = s->transposePitch;
  t->transposeLights = s->transposeLights;
  t->ccFaders = s->ccFaders;
  t->arpeggiator = s->arpeggiator;
  t->strum = s->strum;
}