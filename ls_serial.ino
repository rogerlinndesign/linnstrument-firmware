/******************************** ls_serial: LinnStrument Serial **********************************
This work is licensed under the Creative Commons Attribution-ShareAlike 3.0 Unported License.
To view a copy of this license, visit http://creativecommons.org/licenses/by-sa/3.0/
or send a letter to Creative Commons, PO Box 1866, Mountain View, CA 94042, USA.
***************************************************************************************************

**************************************************************************************************/

// Handshake codes for settings transfer
const char* countDownCode = "5, 4, 3, 2, 1 ...\n";
const byte countDownLength = 18;
const char* linnGoCode = "LinnStruments are go!\n"; 
const char* ackCode = "ACK\n";
const char* linnStrumentControlCode = "LC\n";
const byte linnStrumentControlLength = 3;

boolean waitingForCommands = false;

enum linnCommands {
  SendSettings = 's',
  RestoreSettings = 'r',
  LightLed = 'l'
};

byte codePos = 0;
uint32_t lastSerialMoment = 0;

void handleSerialIO() {
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
        serialSendSettings();
        break;
      }

      case RestoreSettings:
      {
        serialRestoreSettings();
        break;
      }

      case LightLed:
      {
        serialLightLed();
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
    else if (d == linnStrumentControlCode[codePos]) {
      codePos++;
      if (codePos == linnStrumentControlLength) {
        codePos = 0;
        waitingForCommands = true;
        controlModeActive = true;
        clearDisplay();
        updateDisplay();
        Serial.write(ackCode);
      }
    }
    else {
      codePos = 0;
    }
  }
}

void serialSendSettings() {
  Serial.write(ackCode);

  clearDisplayImmediately();
  delayUsec(1000);

  int32_t confSize = sizeof(Configuration);

  // send the size of the settings
  byte buff1[sizeof(int32_t)];
  memcpy(buff1, &confSize, sizeof(int32_t));
  Serial.write(buff1, sizeof(int32_t));

  // send the actual settings
  byte buff2[confSize];
  memcpy(buff2, &config, confSize);
  Serial.write(buff2, confSize);

  Serial.write(ackCode);
}

boolean serialWaitForMaximumTwoSeconds() {
  // retry if there's no data available
  while (Serial.available() <= 0) {
    // timeout after 2s if no data is coming in anymore
    if (calcTimeDelta(millis(), lastSerialMoment) > 2000) {
      waitingForCommands = false;
      return false;
    }
  }

  return true;
}

void serialRestoreSettings() {
  Serial.write(ackCode);

  clearDisplayImmediately();
  delayUsec(1000);

  // retrieve the size of the settings
  lastSerialMoment = millis();

  byte buff1[sizeof(int32_t)];
  for (byte i = 0; i < 4; ++i) {
    if (!serialWaitForMaximumTwoSeconds()) return;

    // read the next byte of the configuration size
    buff1[i] = Serial.read();
    lastSerialMoment = millis();
  }

  int32_t confSize;
  memcpy(&confSize, buff1, sizeof(int32_t));

  // retrieve the actual settings
  lastSerialMoment = millis();
  byte buff2[confSize];
  for (unsigned j = 0; j < confSize; ++j) {
    if (j % 32 == 0) {
      Serial.write(ackCode);
    }
    
    if (!serialWaitForMaximumTwoSeconds()) return;

    // read the next byte of the configuration data
    buff2[j] = Serial.read();
    lastSerialMoment = millis();
  }

  boolean settingsApplied = upgradeConfigurationSettings(confSize, buff2);

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
  if (settingsApplied && Device.calibrated) {
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
}

void serialLightLed() {
  lastSerialMoment = millis();

  byte buff[3];
  for (byte i = 0; i < 3; ++i) {
    if (!serialWaitForMaximumTwoSeconds()) return;

    // read the next byte of the configuration size
    buff[i] = Serial.read();
    lastSerialMoment = millis();
  }

  setLed(buff[0], buff[1], buff[2], cellOn);

  updateDisplay();
}
