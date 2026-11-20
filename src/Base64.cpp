#include "Base64.h"
using namespace DenChela;

static const char TABLE[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static int8_t charValue(char symbol) {              // символ -> 6 битов значения
    if (symbol >= 'A' && symbol <= 'Z') return symbol - 'A';
    if (symbol >= 'a' && symbol <= 'z') return symbol - 'a' + 26;
    if (symbol >= '0' && symbol <= '9') return symbol - '0' + 52;
    if (symbol == '+') return 62;
    if (symbol == '/') return 63;
    return -1;                                      // всё остальное — ошибка
}

uint16_t Base64::encodedLength(uint16_t size) {
    return ((size + 2) / 3) * 4;                    // 4 символа на каждые 3 байта
}

uint16_t Base64::encode(const uint8_t* src, uint16_t size, char* dst) {
    uint16_t written = 0;
    uint16_t position = 0;
    while (position + 3 <= size) {                  // полные группы по 3 байта
        uint32_t group = ((uint32_t)src[position] << 16) |
                         ((uint32_t)src[position + 1] << 8) |
                         (uint32_t)src[position + 2];
        dst[written++] = TABLE[(group >> 18) & 0x3F];
        dst[written++] = TABLE[(group >> 12) & 0x3F];
        dst[written++] = TABLE[(group >> 6) & 0x3F];
        dst[written++] = TABLE[group & 0x3F];
        position += 3;
    }
    uint16_t rest = size - position;
    if (rest == 1) {                                // один байт: два символа и '=='
        uint32_t group = (uint32_t)src[position] << 16;
        dst[written++] = TABLE[(group >> 18) & 0x3F];
        dst[written++] = TABLE[(group >> 12) & 0x3F];
        dst[written++] = '=';
        dst[written++] = '=';
    } else if (rest == 2) {                         // два байта: три символа и '='
        uint32_t group = ((uint32_t)src[position] << 16) |
                          ((uint32_t)src[position + 1] << 8);
        dst[written++] = TABLE[(group >> 18) & 0x3F];
        dst[written++] = TABLE[(group >> 12) & 0x3F];
        dst[written++] = TABLE[(group >> 6) & 0x3F];
        dst[written++] = '=';
    }
    dst[written] = '\0';
    return written;
}

uint16_t Base64::decode(const char* src, uint16_t size, uint8_t* dst) {
    uint16_t written = 0;
    uint16_t position = 0;
    while (position < size) {
        uint8_t group[4] = {0, 0, 0, 0};            // значения четырёх символов
        uint8_t symbols = 0;
        for (uint8_t i = 0; i < 4 && position < size; i++) {
            char symbol = src[position++];
            if (symbol == '=') break;               // набивка завершает группу
            int8_t value = charValue(symbol);
            if (value < 0) return 0;                // некорректный символ
            group[symbols++] = (uint8_t)value;
        }
        if (symbols < 2) return 0;                  // меньше двух символов — ошибка
        uint32_t bits = ((uint32_t)group[0] << 18) |
                        ((uint32_t)group[1] << 12) |
                        ((uint32_t)group[2] << 6) |
                        (uint32_t)group[3];
        dst[written++] = (uint8_t)(bits >> 16);
        if (symbols >= 3) dst[written++] = (uint8_t)(bits >> 8);
        if (symbols == 4) dst[written++] = (uint8_t)bits;
    }
    return written;
}

uint16_t Base64::decodedLength(uint16_t size) {
    return size / 4 * 3;                            // максимум без набивки
}