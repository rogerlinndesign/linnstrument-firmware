/************************* ls_bytebuffer: LinnStrument ByteBuffer class ***************************
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

  bool empty() const {
    return write_ == read_;
  }

private:
  byte buffer_[Size];
  byte *write_;
  byte *read_;
  byte *tail_;
};

#endif
