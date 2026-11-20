#pragma once

#include <stdint.h>

/*
    SHA1 для хендшейка WebSocket: двадцать байт дайджеста по RFC 3174.
    Реализация собственная, без платформенных зависимостей —
    собирается на ESP8266 и ESP32 из коробки.
    Один буфер на 64 байта, никаких аллокаций и String.
*/

namespace DenChela {

    class SHA1 {
    public:
        SHA1();

        // сброс состояния для нового расчёта
        void begin();
        // добавление порции данных (потоково)
        void update(const uint8_t* data, uint16_t size);  
        void update(const char* data, uint16_t size) {
            update(reinterpret_cast<const uint8_t*>(data), size);
        }
        // завершение: паддинг и выгрузка дайджеста
        void final(uint8_t digest[20]);

        // Одноразовый расчёт хеша строки целиком.
        static void hash(const char* data, uint16_t size, uint8_t digest[20]);

    private:

        uint32_t state[5];                          // пять слов промежуточного состояния
        uint64_t size;                              // всего байт, прошедших через update
        uint8_t buffer[64];                         // буфер текущего блока
        uint16_t bufferLength;                      // сколько байт уже лежит в буфере

        // обработка одного 64-байтового блока
        void processBlock(const uint8_t* block);
    };
}
