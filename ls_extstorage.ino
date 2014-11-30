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
  ReadSettings = 'r',
  Done = 'd'
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
        break;
      }
      case ReadSettings:
        break;
      case Done:
      default:
        waitingForCommands = false;
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
