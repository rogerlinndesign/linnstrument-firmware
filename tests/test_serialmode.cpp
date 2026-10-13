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
// Switching between MIDI and serial mode from the outside: NRPN 271 and the serial m command.

TEST(nrpn_271_enters_serial_mode) {
  linn::reset();
  CHECK(!Device.serialMode);
  receivedNrpn(271, 1, 0);
  CHECK(Device.serialMode);
  CHECK_EQ(host::pinState(35), HIGH);
}

TEST(nrpn_271_ignores_other_values) {
  linn::reset();
  receivedNrpn(271, 0, 0);
  CHECK(!Device.serialMode);
  receivedNrpn(271, 2, 0);
  CHECK(!Device.serialMode);
}

TEST(serial_handshake_answers_go_code) {
  linn::reset();
  switchSerialMode(true);
  host::feedSerial("5, 4, 3, 2, 1 ...\n");
  linn::pumpSerial();
  CHECK(host::serialOutputText() == "LinnStruments are go!\n");
}

TEST(serial_m_command_returns_to_midi_mode) {
  linn::reset();
  switchSerialMode(true);
  host::feedSerial("5, 4, 3, 2, 1 ...\n");
  linn::pumpSerial();
  host::clearSerialOutput();
  host::feedSerial("m");
  linn::pumpSerial();
  CHECK(host::serialOutputText() == "ACK\n");
  CHECK(!Device.serialMode);
  CHECK_EQ(host::pinState(35), LOW);
}

TEST(serial_command_without_handshake_is_ignored) {
  linn::reset();
  switchSerialMode(true);
  host::feedSerial("m");
  linn::pumpSerial();
  CHECK(host::serialOutputText().empty());
  CHECK(Device.serialMode);
}

// the batch checksum is the standard CRC-32, checked against its reference vector
TEST(serial_crc_is_ieee_crc32) {
  uint8_t vector[] = "123456789";
  CHECK_EQ(crc_byte_array(vector, 9), 0xCBF43926u);
}

TEST(serial_s_command_streams_the_settings_in_crc_batches) {
  linn::reset();
  switchSerialMode(true);
  const int size = sizeof(Configuration);
  const int batches = (size + 95) / 96;
  host::feedSerial("5, 4, 3, 2, 1 ...\n");
  host::feedSerial("s");
  for (int i = 0; i < batches; ++i) {
    host::feedSerial("co");
  }
  linn::pumpSerial();
  std::vector<uint8_t> out = host::serialOutput();
  std::string head = "LinnStruments are go!\nACK\n";
  CHECK(out.size() > head.size() + 4);
  CHECK(std::string(out.begin(), out.begin() + head.size()) == head);
  int32_t announced;
  memcpy(&announced, &out[head.size()], 4);
  CHECK_EQ(announced, size);
  CHECK_EQ(out.size(), head.size() + 4 + size + 4 * batches + 4);
  CHECK(std::string(out.end() - 4, out.end()) == "ACK\n");
  size_t at = head.size() + 4;
  int crcMismatches = 0;
  for (int i = 0; i < batches && at + 4 <= out.size(); ++i) {
    int n = min(96, size - i * 96);
    uint32_t expected = crc_byte_array(&out[at], (uint8_t)n);
    uint32_t sent;
    memcpy(&sent, &out[at + n], 4);
    if (sent != expected) ++crcMismatches;
    at += n + 4;
  }
  CHECK_EQ(crcMismatches, 0);
}

// The r command, the way the Updater restores settings after a flash: a 32-bit size, then the
// settings in batches of 96 bytes, each answered with c and followed by its CRC-32, which the
// instrument accepts with o or refuses with w and waits for again.
static void addBatches(std::vector<uint8_t>& stream, const std::vector<uint8_t>& blob, int corruptBatch) {
  for (size_t at = 0, batch = 0; at < blob.size(); at += 96, ++batch) {
    size_t n = min((size_t)96, blob.size() - at);
    uint32_t crc = crc_byte_array((uint8_t*)&blob[at], (uint8_t)n);
    if ((int)batch == corruptBatch) {
      stream.insert(stream.end(), blob.begin() + at, blob.begin() + at + n);
      uint32_t wrong = crc ^ 1;
      stream.insert(stream.end(), (uint8_t*)&wrong, (uint8_t*)&wrong + 4);
    }
    stream.insert(stream.end(), blob.begin() + at, blob.begin() + at + n);
    stream.insert(stream.end(), (uint8_t*)&crc, (uint8_t*)&crc + 4);
  }
}

static std::vector<uint8_t> restoreStream(const std::vector<uint8_t>& blob, int32_t announcedSize, int corruptBatch = -1) {
  std::vector<uint8_t> stream;
  stream.push_back('r');
  stream.insert(stream.end(), (uint8_t*)&announcedSize, (uint8_t*)&announcedSize + 4);
  addBatches(stream, blob, corruptBatch);
  return stream;
}

static std::vector<uint8_t> blobWithRowOffset(byte rowOffset, boolean calibrated) {
  Configuration blob = config;
  blob.settings.global.rowOffset = rowOffset;
  blob.device.calibrated = calibrated;
  return std::vector<uint8_t>((uint8_t*)&blob, (uint8_t*)&blob + sizeof(Configuration));
}

// counts a CRC answer after the handshake reply, whose "go" has an o of its own
static int countByte(const std::vector<uint8_t>& out, uint8_t value) {
  int n = 0;
  for (size_t i = strlen("LinnStruments are go!\n"); i < out.size(); ++i) if (out[i] == value) ++n;
  return n;
}

TEST(serial_r_command_restores_the_settings_and_leaves_serial_mode) {
  linn::reset();
  switchSerialMode(true);
  std::vector<uint8_t> blob = blobWithRowOffset(7, true);
  host::feedSerial("5, 4, 3, 2, 1 ...\n");
  std::vector<uint8_t> stream = restoreStream(blob, (int32_t)blob.size());
  host::feedSerial(stream.data(), stream.size());
  linn::pumpSerial();
  std::vector<uint8_t> out = host::serialOutput();
  CHECK_EQ(Global.rowOffset, 7);
  CHECK(Device.calibrated);
  CHECK(!Device.serialMode);
  CHECK_EQ(host::pinState(35), LOW);
  CHECK_EQ(displayMode, displayNormal);
  CHECK_EQ(countByte(out, 'o'), (int)((blob.size() + 95) / 96));
  CHECK_EQ(countByte(out, 'w'), 0);
  CHECK(std::string(out.end() - 4, out.end()) == "ACK\n");
}

TEST(serial_restore_asks_again_for_a_batch_whose_crc_is_wrong) {
  linn::reset();
  switchSerialMode(true);
  std::vector<uint8_t> blob = blobWithRowOffset(9, true);
  host::feedSerial("5, 4, 3, 2, 1 ...\n");
  std::vector<uint8_t> stream = restoreStream(blob, (int32_t)blob.size(), 2);
  host::feedSerial(stream.data(), stream.size());
  linn::pumpSerial();
  std::vector<uint8_t> out = host::serialOutput();
  CHECK_EQ(countByte(out, 'w'), 1);
  CHECK_EQ(countByte(out, 'o'), (int)((blob.size() + 95) / 96));
  CHECK_EQ(Global.rowOffset, 9);
}

// a blob whose size matches no layout is refused: the defaults stay and the instrument asks
// for a calibration, as it does after a bare flash
TEST(serial_restore_refuses_a_blob_of_the_wrong_size_and_asks_for_calibration) {
  linn::reset();
  switchSerialMode(true);
  std::vector<uint8_t> blob = blobWithRowOffset(7, true);
  blob.pop_back();
  host::feedSerial("5, 4, 3, 2, 1 ...\n");
  std::vector<uint8_t> stream = restoreStream(blob, (int32_t)blob.size());
  host::feedSerial(stream.data(), stream.size());
  linn::pumpSerial();
  CHECK_EQ(Global.rowOffset, 5);
  CHECK(!Device.serialMode);
  CHECK_EQ(displayMode, displayCalibration);
}

TEST(serial_restore_gives_up_after_two_seconds_without_data) {
  linn::reset();
  switchSerialMode(true);
  host::feedSerial("5, 4, 3, 2, 1 ...\n");
  host::feedSerial("r\x10\x00");
  linn::pumpSerial();
  CHECK(!waitingForCommands);
  CHECK(Device.serialMode);
}

