#include "string.h"

int64_t atoi_64(const char *str) {
    uint8_t is_neg = 0;
    if (str[0] == '-') {
        is_neg = 1;
        str++;
    }
    uint64_t ret = 0;
    while (*str != '\0') {
        ret = 10 * ret;
        ret += a2d(*str);
        str++;
    }
    return is_neg ? -(int64_t)ret : (int64_t)ret;
}

int64_t str_to_hex(const char *str) {
    uint64_t ret = 0;
    // Skip "0x" prefix
    if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
        str += 2;
    }
    while (*str != '\0') {
        ret = 16 * ret;
        ret += a2d(*str);
        str++;
    }
    return ret;
}

int strcmp_ret(const char *s1, const char *s2) {
    while (*s1 && *s2) {
        if (*s1 != *s2) {
            return 0;
        }
        s1++;
        s2++;
    }
    return (*s1 == *s2);
}
