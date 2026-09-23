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
