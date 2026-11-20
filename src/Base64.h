#pragma once

#include <stdint.h>

/*
    Base64 для хендшейка WebSocket (RFC 4648).
    Только статические функции: состояние не нужно, память задаёт вызывающий.
    Без String и аллокаций — собирается на ESP8266 и ESP32 из коробки.
*/

namespace DenChela {

    class Base64 {
    public:
        // Точная длина закодированных данных с набивкой '='.
        static uint16_t encodedLength(uint16_t size);

        // Кодирует size байт в dst. dst должна вмещать encodedLength(size) + 1,
        // в конце добавляется '\0'. Возвращает число записанных символов без '\0'.
        static uint16_t encode(const uint8_t* src, uint16_t size, char* dst);

        // Декодирует size символов в dst. dst должна вмещать decodedLength(size) байт.
        // Возвращает число декодированных байт или 0 при некорректном вводе.
        static uint16_t decode(const char* src, uint16_t size, uint8_t* dst);

        // Верхняя оценка декодированных данных: в два раза меньше входа, округлённо.
        static uint16_t decodedLength(uint16_t size);
    };

}
