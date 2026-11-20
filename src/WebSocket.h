#pragma once
#include "WebServer.h"
#include "SHA1.h"
#include "Base64.h"

#define MAX_LENGTH_PAYLOAD 1536
#define TIME_LIMIT_READ_FRAME 500

namespace DenChela {

    #define TIME_PERIOD_PING 10000                                          //Период отправки сообщений PING клиенту
    #define TIME_PING 10000                                                 //Время ответа клиента на сообщение PING

    const uint8_t SOCKET_PING[] PROGMEM = {0x89, 0x01, 0x00};               //Кадр отправки PING

    class WebSocket: public WebServer {
        public:

            enum CodeClose:uint16_t {
                NORMAL_CLOSURE = 1000,          //Штатное закрытие — «я закончил работу». Это то, что шлёт браузер при закрытии вкладки
                GOING_AWAY = 1001,              //Конец уходит: закрытие страницы, перезагрузка, сервер останавливается
                PROTOCOL_ERROR = 1002,          //Нарушен протокол WebSocket — битый кадр, неверная маска и т. п.
                UNSUPPORTED_DATA = 1003,        //Принят кадр с типом данных, который не умеем обрабатывать
                INVALID_PAYLOAD_DATA = 1007,    //Сообщение не соответствует своему формату — например, невалидный UTF-8 в текстовом кадре
                POLICY_VIOLATION = 1008,        //Нарушение правил — «обобщение» от 1003 и 1009, когда точная причина не подходит
                MESSAGE_TOO_BIG = 1009,         //Сообщение слишком длинное для обработки
                NEGOTIATION_FAILED = 1010,      //Клиент ожидает расширения (например, permessage-deflate), которое сервер не предоставил
                INTERNAL_ERROR = 1011           //Сервер наткнулся на неожиданное условие и не может продолжить обработку запроса
            };

            WebSocket(const uint16_t port, void (*_readMessage)(const char* data));
            ~WebSocket();
            //Главный цикл
            void handle() override;
            //Метод отправки сообщения клиенту
            void sendMessage(const char* message);
            //Соединение по WebSocket установлено
            bool isConnected();
            //Метод закрытия сокета
            void socketClose(uint16_t codeClose);
            //Метод закрытия сокета с передачей текстового сообщения
            void socketClose(uint16_t codeClose, const char* text);
            //Метод задаёт функцию, которая исполняется при закрытии сокета
            void setSocketDisconnect(void (*function)(uint16_t codeClose, const char* text));

        private:

            enum StatusSocket:uint8_t {
                CLOSE = 0,                                                      //Нет соединения с сокетом
                START = 1,                                                      //Соединение получено, ожидается хендшейк
                CONNECTED = 2,                                                  //Есть соединение с сокетом
                LOADING_FRAME = 3,                                             //Загрузка тела кадра
            };

            WiFiClient clientSocket;                                            //Клиент для сокета
            StatusSocket statusSocket = CLOSE;                                  //Статус подключения сокета
            uint8_t typeFrame;                                                  //Тип кадра
            bool isMask;                                                        //Есть ли в кадре маска
            uint16_t sizeFrame;                                                 //Размер кадра
            uint32_t timeLimitLoadingFrame;                                    //Таймер для ограничения загрузки кадра
            bool isPing = false;                                                //Флаг, указывающий, что отправлен сигнал PING
            unsigned long timePing = 0;                                         //Для отслеживания времени приёма сигнала PONG
            void (*readMessage)(const char* data);                              //Функция, которая исполняется при получении сообщения
            void (*socketDisconnect)(uint16_t codeClose, const char* text);     //Функция, которая исполняется при закрытии сокета
            char *connection;                                                   //Тип соединения
            char *upgrade;                                                      //Тип апгрейта
            char *secWebsocketKey;                                              //Ключ Sec-WebSocket-Key

            //Очистка памяти
            void clearMemory() override;
            //Опрос на входящее соединение от клиента
            char* handleHTTP(bool isContinue = false) override;
            //Возвращает строку статуса по коду requestCode
            const __FlashStringHelper* getTypeRequestCode(uint16_t requestCode) override;
            //Функция отправляющая HTTP-заголовки клиенту
            void sendHeaders(uint16_t requestCode, const char *ContentType, size_t size, bool isContinue = false) override;
            //PING-PONG с браузером
            void pingPong();
            //Функция читает заголовок кадра
            void getIsMaskAndSizeFrame();
            //Чтение и обработка входящего сообщения
            void readPayload();
    };
};