#include "Base64.h"

static const char base64_chars[] = 
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

String Base64::encode(const uint8_t* data, size_t length) {
    String encoded = "";
    int i = 0;
    uint8_t array3[3];
    uint8_t array4[4];

    while (length--) {
        array3[i++] = *(data++);
        if (i == 3) {
            array4[0] = (array3[0] & 0xfc) >> 2;
            array4[1] = ((array3[0] & 0x03) << 4) + ((array3[1] & 0xf0) >> 4);
            array4[2] = ((array3[1] & 0x0f) << 2) + ((array3[2] & 0xc0) >> 6);
            array4[3] = array3[2] & 0x3f;

            for (i = 0; i < 4; i++) {
                encoded += base64_chars[array4[i]];
            }
            i = 0;
        }
    }

    if (i) {
        for (int j = i; j < 3; j++) {
            array3[j] = '\0';
        }

        array4[0] = (array3[0] & 0xfc) >> 2;
        array4[1] = ((array3[0] & 0x03) << 4) + ((array3[1] & 0xf0) >> 4);
        array4[2] = ((array3[1] & 0x0f) << 2) + ((array3[2] & 0xc0) >> 6);
        array4[3] = array3[2] & 0x3f;

        for (int j = 0; j < i + 1; j++) {
            encoded += base64_chars[array4[j]];
        }

        while (i++ < 3) {
            encoded += '=';
        }
    }

    return encoded;
}