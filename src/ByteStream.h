#pragma once

#include <Arduino.h>

class ByteStream : public Stream
{
    uint8_t *buf;
    size_t maxsize, len;

  public:
    ByteStream(size_t _size);
    ~ByteStream()
    {
        delete[] buf;
    }

    uint8_t *getBytes()
    {
        return buf;
    }

    void setLen(size_t l)
    {
        len = l;
    }

    size_t write(const uint8_t *data, size_t size) override
    {
        if (size <= (maxsize - len) && data)
        {
            memcpy(buf + len, data, size);
            len += size;
            return size;
        }
        return 0;
    }
    size_t write(uint8_t data) override
    {
        return write(&data, 1);
    }

    int available() override
    {
        return len;
    }
    int read() override
    {
        return -1;
    }
    int peek() override
    {
        return -1;
    }
    void flush() override
    {
    }
};
