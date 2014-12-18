/***************************** ls_extstorage: LinnStrument Settings *******************************
This work is licensed under the Creative Commons Attribution-ShareAlike 3.0 Unported License.
To view a copy of this license, visit http://creativecommons.org/licenses/by-sa/3.0/
or send a letter to Creative Commons, PO Box 1866, Mountain View, CA 94042, USA.
***************************************************************************************************
**************************************************************************************************/

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

        int32_t confSize = sizeof(struct Configuration);;

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
        for (int i = 0; i < 4; ++i) {
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
        for (int j = 0; j < confSize; ++j) {
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
        memcpy(&config, buff2, confSize);

        // activate the retrieved settings
        applyConfiguration();

        // send the acknowledgement of success
        Serial.write(ackCode);
        delayUsec(1000000);

        // Turn off OS upgrade mode
        switchSerialMode(false);

        // Enable normal playing mode and ensure calibration is fully turned off
        displayMode = displayNormal;
        controlButton = -1;
        clearLed(0, GLOBAL_SETTINGS_ROW);

        storeSettings();

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
