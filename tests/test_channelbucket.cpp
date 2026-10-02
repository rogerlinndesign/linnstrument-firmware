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
// The channel bucket hands out MIDI channels for channel-per-note playing: least recently freed
// first, and when every channel is in use, the one that has been playing longest.

TEST(channelbucket_empty_bucket_hands_out_nothing) {
  ChannelBucket bucket;
  bucket.clear();
  CHECK_EQ(bucket.take(), 0);
}

TEST(channelbucket_hands_out_channels_in_the_order_added_then_reuses) {
  ChannelBucket bucket;
  bucket.clear();
  bucket.add(2);
  bucket.add(3);
  bucket.add(4);
  CHECK_EQ(bucket.take(), 2);
  CHECK_EQ(bucket.take(), 3);
  CHECK_EQ(bucket.take(), 4);
  CHECK_EQ(bucket.take(), 2);
}

TEST(channelbucket_released_channel_is_reused_last_among_the_free_ones) {
  ChannelBucket bucket;
  bucket.clear();
  bucket.add(2);
  bucket.add(3);
  bucket.add(4);
  CHECK_EQ(bucket.take(), 2);
  CHECK_EQ(bucket.take(), 3);
  bucket.release(2);
  CHECK_EQ(bucket.take(), 4);
  CHECK_EQ(bucket.take(), 2);
}

TEST(channelbucket_channel_stays_taken_while_two_notes_share_it) {
  ChannelBucket bucket;
  bucket.clear();
  bucket.add(2);
  bucket.add(3);
  CHECK_EQ(bucket.take(), 2);
  CHECK_EQ(bucket.take(), 3);
  CHECK_EQ(bucket.take(), 2);
  bucket.release(2);
  CHECK_EQ(bucket.take(), 3);
  bucket.release(2);
  bucket.release(3);
  bucket.release(3);
  CHECK_EQ(bucket.take(), 2);
}

TEST(channelbucket_clear_then_a_new_set_starts_fresh) {
  ChannelBucket bucket;
  bucket.clear();
  bucket.add(2);
  bucket.add(3);
  CHECK_EQ(bucket.take(), 2);
  bucket.clear();
  CHECK_EQ(bucket.take(), 0);
  bucket.add(5);
  bucket.add(6);
  CHECK_EQ(bucket.take(), 5);
  CHECK_EQ(bucket.take(), 6);
  CHECK_EQ(bucket.take(), 5);
}

TEST(channelbucket_ignores_duplicates_and_channels_above_16) {
  ChannelBucket bucket;
  bucket.clear();
  bucket.add(2);
  bucket.add(2);
  bucket.add(17);
  bucket.add(16);
  CHECK_EQ(bucket.take(), 2);
  CHECK_EQ(bucket.take(), 16);
  CHECK_EQ(bucket.take(), 2);
}

TEST(channelbucket_all_sixteen_channels) {
  ChannelBucket bucket;
  bucket.clear();
  for (byte ch = 1; ch <= 16; ++ch) bucket.add(ch);
  for (byte ch = 1; ch <= 16; ++ch) CHECK_EQ(bucket.take(), ch);
  bucket.release(9);
  for (byte ch = 1; ch <= 16; ++ch) {
    if (ch != 9) bucket.release(ch);
  }
  CHECK_EQ(bucket.take(), 9);
  CHECK_EQ(bucket.take(), 1);
}
