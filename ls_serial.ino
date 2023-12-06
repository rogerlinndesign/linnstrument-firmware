/******************************** ls_serial: LinnStrument Serial **********************************
Copyright 2023 Roger Linn Design (https://www.rogerlinndesign.com)

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
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
  ACK = 'a',
  CRCCheck = 'c',
  CRCWrong = 'w',
  CRCOk = 'o',
  LightLed = 'l',
  SendSingleProject = 'j',
  SendProjects = 'p',
  RestoreProject = 'q',
  RestoreSettings = 'r',
  SendSettings = 's'
};

byte codePos = 0;
uint32_t lastSerialMoment = 0;

static PROGMEM prog_uint32_t crc_table[16] = {
    0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac,
    0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
    0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
    0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c
};

uint32_t crc_update(uint32_t crc, uint8_t data) {
    uint8_t tbl_idx;
    tbl_idx = crc ^ (data >> (0 * 4));
    crc = pgm_read_dword_near(crc_table + (tbl_idx & 0x0f)) ^ (crc >> 4);
    tbl_idx = crc ^ (data >> (1 * 4));
    crc = pgm_read_dword_near(crc_table + (tbl_idx & 0x0f)) ^ (crc >> 4);
    return crc;
}

uint32_t crc_byte_array(uint8_t* s, uint8_t size) {
  uint32_t crc = ~0L;
  for (uint8_t i = 0; i < size; ++i) {
    crc = crc_update(crc, *s++);
  }
  crc = ~crc;
  return crc;
}

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

      case SendSingleProject:
      {
        serialSendSingleProject();
        break;
      }

      case SendProjects:
      {
        serialSendProjects();
        break;
      }

      case RestoreProject:
      {
        serialRestoreProject();
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

boolean waitForSerialAck() {
  if (!serialWaitForMaximumTwoSeconds()) return false;
  char ack = Serial.read();
  lastSerialMoment = millis();
  if (ack != ACK) return false;
  return true;
}

boolean waitForSerialCheck() {
  if (!serialWaitForMaximumTwoSeconds()) return false;
  char ack = Serial.read();
  lastSerialMoment = millis();
  if (ack != CRCCheck) return false;
  return true;
}

char waitForSerialCRC() {
  if (!serialWaitForMaximumTwoSeconds()) return 0;
  char ack = Serial.read();
  lastSerialMoment = millis();
  return ack;
}

int negotiateOutgoingCRC(byte* buffer, uint8_t size) {
  uint32_t crc = crc_byte_array(buffer, size);
  Serial.write((byte*)&crc, sizeof(uint32_t));

  char crcresponse = waitForSerialCRC();
  if (crcresponse == 0) return -1;
  if (crcresponse == CRCWrong) return 0;
  return 1;
}

int negotiateIncomingCRC(byte* buffer, uint8_t size) {
  Serial.write(CRCCheck);

  byte buff_crc[sizeof(uint32_t)];
  for (byte k = 0; k < sizeof(uint32_t); ++k) {
    if (!serialWaitForMaximumTwoSeconds()) return -1;
    buff_crc[k] = Serial.read();
    lastSerialMoment = millis();
  }
  uint32_t remote_crc;
  memcpy(&remote_crc, buff_crc, sizeof(uint32_t));

  uint32_t local_crc = crc_byte_array(buffer, size);
  if (local_crc != remote_crc) {
    Serial.write(CRCWrong);
    return 0;
  }

  Serial.write(CRCOk);
  return 1;
}

void serialSendSettings() {
  Serial.write(ackCode);

  clearDisplayImmediately();
  delayUsec(1000);

  int32_t confSize = sizeof(Configuration);

  // send the size of the settings
  Serial.write((byte*)&confSize, sizeof(int32_t));

  // send the actual settings
  const uint8_t batchsize = 96;
  byte* src = (byte*)&config;
  lastSerialMoment = millis();
  while (confSize > 0) {
    int actual = min(confSize, batchsize);
    Serial.write(src, actual);

    if (!waitForSerialCheck()) return;

    int crc = negotiateOutgoingCRC(src, actual);
    if (crc == -1)      return;
    else if (crc == 0)  continue;

    confSize -= actual;
    src += actual;
  }

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
  for (byte i = 0; i < sizeof(int32_t); ++i) {
    if (!serialWaitForMaximumTwoSeconds()) return;
    buff1[i] = Serial.read();
    lastSerialMoment = millis();
  }
  int32_t settingsSize;
  memcpy(&settingsSize, buff1, sizeof(int32_t));

  Serial.write(ackCode);

  // restore the actual settings
  uint32_t projectOffset = SETTINGS_OFFSET;
  const uint8_t batchsize = 96;
  byte buff2[batchsize];
  lastSerialMoment = millis();
  int32_t remaining = settingsSize;
  while (remaining > 0) {
    int actual = min(remaining, batchsize);
    for (byte k = 0; k < actual; ++k) {
      if (!serialWaitForMaximumTwoSeconds()) return;
      buff2[k] = Serial.read();
      lastSerialMoment = millis();
    }

    int crc = negotiateIncomingCRC(buff2, actual);
    if (crc == -1)      return;
    else if (crc == 0)  continue;

    dueFlashStorage.write(projectOffset, buff2, actual);

    remaining -= actual;
    projectOffset += actual;
  }

  boolean settingsApplied = upgradeConfigurationSettings(settingsSize, dueFlashStorage.readAddress(SETTINGS_OFFSET));

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
    clearLed(0, GLOBAL_SETTINGS_ROW);

    storeSettings();
  }
  // turn on calibration instead if no new settings were applied and default settings are used
  else {
    setDisplayMode(displayCalibration);
    controlButton = GLOBAL_SETTINGS_ROW;
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

int32_t serialSendProjectSize() {
  // send the size of a project
  int32_t projectSize = sizeof(SequencerProject);
  Serial.write((byte*)&projectSize, sizeof(int32_t));
  lastSerialMoment = millis();
  return projectSize;
}

void serialSendProjectRaw(int32_t projectSize, byte projectNumber) {
  byte marker = dueFlashStorage.read(PROJECTS_OFFSET);

  // send the actual settings
  const uint8_t batchsize = 96;

  byte prjIndex = dueFlashStorage.read(PROJECT_INDEX_OFFSET(marker, projectNumber));
  uint32_t projectOffset = PROJECTS_OFFSET + PROJECTS_MARKERS_SIZE + prjIndex * SINGLE_PROJECT_SIZE;
  int32_t remaining = projectSize;

  byte* src = (byte*)dueFlashStorage.readAddress(projectOffset);
  while (remaining > 0) {
    int actual = min(remaining, batchsize);
    Serial.write(src, actual);

    if (!waitForSerialCheck()) return;

    int crc = negotiateOutgoingCRC(src, actual);
    if (crc == -1)      return;
    else if (crc == 0)  continue;

    remaining -= actual;
    src += actual;
  }
}

void serialSendSingleProject() {
  Serial.write(ackCode);

  clearDisplayImmediately();
  delayUsec(1000);

  lastSerialMoment = millis();

  if (!serialWaitForMaximumTwoSeconds()) return;
  
  uint8_t projectNumber = Serial.read();
  Serial.write(ackCode);

  Serial.write(Device.version);

  int32_t projectSize = serialSendProjectSize();
  serialSendProjectRaw(projectSize, projectNumber);

  Serial.write(ackCode);
}

void serialSendProjects() {
  Serial.write(ackCode);

  clearDisplayImmediately();
  delayUsec(1000);

  // send the count of projects
  Serial.write((byte)MAX_PROJECTS);

  int32_t projectSize = serialSendProjectSize();

  for (byte p = 0; p < MAX_PROJECTS; ++p) {
    serialSendProjectRaw(projectSize, p);
  }

  Serial.write(ackCode);
}


void serialRestoreProject() {
  Serial.write(ackCode);

  clearDisplayImmediately();
  delayUsec(1000);

  lastSerialMoment = millis();

  if (!serialWaitForMaximumTwoSeconds()) return;
  uint8_t version = Serial.read();
  if (version < 9) return;
  Serial.write(ackCode);
  lastSerialMoment = millis();

  // retrieve the size of a project
  byte buff1[sizeof(int32_t)];
  for (byte i = 0; i < 4; ++i) {
    if (!serialWaitForMaximumTwoSeconds()) return;

    // read the next byte of the project size
    buff1[i] = Serial.read();
    lastSerialMoment = millis();
  }

  int32_t projectSize;
  memcpy(&projectSize, buff1, sizeof(int32_t));

  if (projectSize != sizeof(SequencerProject)) return;

  Serial.write(ackCode);

  if (!serialWaitForMaximumTwoSeconds()) return;

  uint8_t p = Serial.read();
  Serial.write(ackCode);

  // write the actual project
  byte marker = dueFlashStorage.read(PROJECTS_OFFSET);
  byte prjIndex = dueFlashStorage.read(PROJECT_INDEX_OFFSET(marker, p));
  uint32_t projectOffset = PROJECTS_OFFSET + PROJECTS_MARKERS_SIZE + prjIndex * SINGLE_PROJECT_SIZE;

  const uint8_t batchsize = 96;
  byte buff2[batchsize];
  lastSerialMoment = millis();
  int32_t remaining = projectSize;
  while (remaining > 0) {
    int actual = min(remaining, batchsize);
    for (byte k = 0; k < actual; ++k) {
      if (!serialWaitForMaximumTwoSeconds()) return;
      buff2[k] = Serial.read();
      lastSerialMoment = millis();
    }

    int crc = negotiateIncomingCRC(buff2, actual);
    if (crc == -1)      return;
    else if (crc == 0)  continue;

    dueFlashStorage.write(projectOffset, buff2, actual);

    remaining -= actual;
    projectOffset += actual;
  }

  // finished
  Serial.write(ackCode);
  delayUsec(500000);
}
