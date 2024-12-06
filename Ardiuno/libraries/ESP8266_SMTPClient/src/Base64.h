#ifndef BASE64_H
#define BASE64_H

#include <Arduino.h>

class Base64 {
public:
    static String encode(const uint8_t* data, size_t length);
};

#endif