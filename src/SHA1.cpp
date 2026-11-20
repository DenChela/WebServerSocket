#include "SHA1.h"
using namespace DenChela;

SHA1::SHA1() {
    begin();
}

void SHA1::begin() {
    state[0] = 0x67452301;
    state[1] = 0xEFCDAB89;
    state[2] = 0x98BADCFE;
    state[3] = 0x10325476;
    state[4] = 0xC3D2E1F0;
    size = 0;
    bufferLength = 0;
}

void SHA1::update(const uint8_t* data, uint16_t size) {
    while (size != 0) {
        uint16_t step = 64 - bufferLength;          // сколько влезает в буфер
        if (step > size) step = size;
        for (uint16_t i = 0; i < step; i++) buffer[bufferLength + i] = data[i];
        bufferLength += step;
        data += step;
        this->size += step;
        size -= step;
        if (bufferLength == 64) {                   // буфер полон — обрабатываем блок
            processBlock(buffer);
            bufferLength = 0;
        }
    }
}

void SHA1::final(uint8_t digest[20]) {
    uint64_t bits = size * 8;                       // длина в битах — до паддинга
    uint8_t last = 0x80;
    update(&last, 1);
    last = 0x00;
    while (bufferLength != 56) update(&last, 1);    // нули до 56 байтов в буфере
    uint8_t lengthBytes[8];                         // длина сообщения big-endian
    for (uint8_t i = 0; i < 8; i++)
        lengthBytes[i] = (uint8_t)(bits >> (56 - 8 * i));
    update(lengthBytes, 8);
    for (uint8_t i = 0; i < 5; i++)                 // выгрузка состояния в дайджест
        for (uint8_t j = 0; j < 4; j++)
            digest[i * 4 + j] = (uint8_t)(state[i] >> (24 - 8 * j));
    begin();                                        // готовность к новому расчёту
}

void SHA1::hash(const char* data, uint16_t size, uint8_t digest[20]) {
    SHA1 sha1;
    sha1.update(data, size);
    sha1.final(digest);
}

void SHA1::processBlock(const uint8_t* block) {
    uint32_t w[80];
    for (uint8_t i = 0; i < 16; i++)                // слово собирается big-endian:
        w[i] = ((uint32_t)block[4 * i] << 24) |     // на любом порядке байт
               ((uint32_t)block[4 * i + 1] << 16) | // результат одинаковый
               ((uint32_t)block[4 * i + 2] << 8) |
               (uint32_t)block[4 * i + 3];
    for (uint8_t i = 16; i < 80; i++)               // расширение расписания сообщений
        w[i] = (((w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16]) << 1) |
                ((w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16]) >> 31));

    const uint32_t rotlA = 5;                       // ротации по шагам алгоритма
    const uint32_t rotlB = 30;
    uint32_t a = state[0];
    uint32_t b = state[1];
    uint32_t c = state[2];
    uint32_t d = state[3];
    uint32_t e = state[4];
    for (uint8_t t = 0; t < 80; t++) {
        uint32_t f;
        uint32_t k;
        if (t < 20) {                               // раунд 0..19
            f = (b & c) | ((~b) & d);
            k = 0x5A827999;
        } else if (t < 40) {                        // раунд 20..39
            f = b ^ c ^ d;
            k = 0x6ED9EBA1;
        } else if (t < 60) {                        // раунд 40..59
            f = (b & c) | (b & d) | (c & d);
            k = 0x8F1BBCDC;
        } else {                                    // раунд 60..79
            f = b ^ c ^ d;
            k = 0xCA62C1D6;
        }
        uint32_t temp = (((a << rotlA) | (a >> 27)) + f + e + k + w[t]);
        e = d;
        d = c;
        c = (b << rotlB) | (b >> 2);
        b = a;
        a = temp;
    }
    state[0] += a;                                  // накопление состояния блока
    state[1] += b;
    state[2] += c;
    state[3] += d;
    state[4] += e;
}