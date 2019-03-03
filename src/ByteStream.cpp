#include <ByteStream.h>

ByteStream::ByteStream(size_t _size) {
    buf = new uint8_t[_size];
    maxsize = _size;
    len = 0;
}
