#include <cassert>
#include "unicode.h"


static const uint8_t PREFIX_MASKS[8] = {
    0b11111111,
    0b01111111,
    0b00111111,
    0b00011111,
    0b00001111,
    0b00000111,
    0b00000011,
    0b00000001,
};


static const uint8_t PREFIXES[8] = {
    0b00000000,
    0b10000000,
    0b11000000, // 2
    0b11100000, // 3
    0b11110000, // 4
    0b11111000, // 5
    0b11111100, // 6
    0b11111110,
};


int u8_read_char_len(const char *s) {
    uint8_t c0 = static_cast<uint8_t>(s[0]);
    if ((c0 >> 7) == 0) {
        return 1;
    } else if ((c0 >> 5) == 0b110) {
        return 2;
    } else if ((c0 >> 4) == 0b1110) {
        return 3;
    } else if ((c0 >> 3) == 0b11110) {
        return 4;
    } else if ((c0 >> 2) == 0b111110) {
        return 5;
    } else if ((c0 >> 1) == 0b1111110) {
        return 6;
    } else {
        throw DecodeError("Bad leading char", c0);
    }
}


static unichar _u8_read_char_with_len(const char *s, int len) {
    assert(1 <= len && len <= 6);

    const uint8_t *c = reinterpret_cast<const uint8_t *>(s);
    if (len == 1) {
        return c[0];
    }

    unichar ans = c[0] & PREFIX_MASKS[len + 1];
    for (int i = 1; i < len; i++) {
        if ((c[i] >> 6) != 0b10) {
            throw DecodeError("Bad following char", (uint8_t)(c[i]));
        }
        ans = (ans << 6) | (c[i] & PREFIX_MASKS[2]);
    }
    return ans;
}


unichar u8_read_char(const char *s) {
    int len = u8_read_char_len(s);
    return _u8_read_char_with_len(s, len);
}


int u8_char_len(unichar ch) {
    assert(ch < 0x80000000);
    if (ch < 0x80) {
        return 1;
    } else {
        int ans = 2;
        for (unichar upper = 0x800; upper <= ch; upper <<= 5) {
            ans++;
        }
        return ans;
    }
}


static char *_u8_write_char_with_len(char *buf, unichar ch, int len) {
    assert(1 <= len && len <= 6);
    if (len == 1) {
        *buf++ = static_cast<char>(ch);
    } else {
        char leading = static_cast<char>(ch >> (6 * (len - 1)));
        *buf++ = leading | PREFIXES[len];

        unichar remain = ch << (38 - 6 * len);
        for (int i = 1; i < len; i++) {
            *buf++ = static_cast<char>((remain >> 26) | PREFIXES[1]);
            remain <<= 6;
        }
    }
    return buf;
}


char *u8_write_char(char *buf, unichar ch) {
    int len = u8_char_len(ch);
    return _u8_write_char_with_len(buf, ch, len);
}
