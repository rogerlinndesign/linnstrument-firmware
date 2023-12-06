/************************* ls_bytebuffer: LinnStrument ByteBuffer class ***************************
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
Circular byte buffer that has independent push and pop locations. This allows a fixed memory usage
for a queue of data, without having to worry about memory allocation.

Once the size of the buffer fills up though, new data will start filling the beginning back up
again. You need to be careful to select the approriate size of the byte buffer for your use-case
in order to prevent losing data.
**************************************************************************************************/

#ifndef BYTEBUFFER_H_
#define BYTEBUFFER_H_

#include <Arduino.h>

template <unsigned int Size>
class ByteBuffer {
public:
  ByteBuffer() : write_(buffer_), read_(buffer_), tail_(buffer_+Size) {}
  ~ByteBuffer() {}

  void push(byte value) {
    *write_++ = value;
    if (write_ == tail_) write_ = buffer_;
  }

  byte peek() {
    return *read_;
  }

  byte pop() {
    byte result = *read_++;
    if (read_ == tail_) read_ = buffer_;
    return result;
  }

  boolean empty() const {
    return write_ == read_;
  }

private:
  byte buffer_[Size];
  byte* write_;
  byte* read_;
  byte* tail_;
};

#endif
