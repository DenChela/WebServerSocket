#pragma once
#include <FS.h>
#if defined(ESP8266)
    #include <ESP8266WiFi.h>
#elif defined(ESP32)
    #include <WiFi.h>
#endif

#define TIME_WAITING 10                 //Время ожидания догрузки запроса: после этих миллисекунд объём данных должен выровняться
#define TIME_RESPONSE 500               //Время ожидания ответа от клиента
#define MAX_LENGTH_DATA 1536            //Максимальная длина принимаемых данных

namespace DenChela {
    class WebSocket;

    class WebServer : public WiFiServer {

        public:
            WebServer(const uint16_t _port);
            ~WebServer();
            //Опрос на входящее соединение от клиента и исполнение действий
            virtual void handle();
            //Создание списка обработчиков для HTTP-запросов
            void on(const char** _httpRequests, void(**_functions)(WebServer &server), uint8_t _countHttpRequest);
            //Функция задаёт обработчик, если HTTP-запрос не найден
            void setNotWriteRequest(void (*function)(WebServer& server));
            //Отправка ответа клиенту
            void send(uint16_t requestCode, const char *contentType, const char *data);
            //Отправка текста из файла, не забываем файл закрыть после отправки
            void send(uint16_t requestCode, const char *contentType, File &file);
            //Получение URI
            const char* getURL();
            //Получить количество аргументов
            const uint16_t arguments();
            //Получить ссылку на имя аргумента по индексу
            const char* getNameArg(uint16_t index);
            //Получить ссылку на аргумент по индексу
            const char* getArg(uint16_t index);

            friend WebSocket;
        private:
        
            enum StatusConnect:byte {
                WAIT = 0,                                               //Ожидание подключения клиента к серверу
                CONNECTION = 1,                                         //Получение первых байт запроса
                LOADING = 2,                                            //Получение всего запроса
                PROCESSING = 3,                                         //Обработка запроса и отправка ответа
            };                                                          //Статус обработки клиента

            enum Method:byte {
                NONE_METHOD = 0,
                GET = 1,
                POST = 2
            };                                                          //Список методов HTTP-запроса

            enum ContentType:byte {
                NONE_TYPE = 0,
                APPLICATION_X_WWW_FORM_URLENCODED = 1,
                TEXT_PLAIN = 2
            };                                                          //Тип кодировки полезной нагрузки

            const char notValue = '\0';                                 //Заглушка на отсутствие данных
            WiFiClient client;                                          //Объект подключенного клиента
            Method method = NONE_METHOD;                                //Метод подключения
            ContentType contentType = NONE_TYPE;                        //Тип POST запроса
            StatusConnect statusConnect = WAIT;                         //Переменная указывающая на состояние подключения
            const char **httpRequests = nullptr;                        //Список HTTP-запросов к серверу
            void (**functions)(WebServer &server) = nullptr;            //Список функций-обработчиков для HTTP-запросов
            void (*isNotWriteRequest)(WebServer &server) = nullptr;     //Функция-обработчик, если запрос не найден
            byte countHttpRequest = 0;                                  //Количество зарегистрированных запросов
            char *URL;                                                  //URL
            uint16_t contentLength = 0;                                 //Длина передаваемого сообщения
            char *listArgs;                                             //Строка с аргументами
            char **arg;                                                 //Массив указателей на значения аргументов
            char **nameArg;                                             //Массив указателей на имена аргументов
            uint16_t args = 0;                                          //Количество аргументов
            uint16_t sizeHttpData = 0;                                  //Размер поступившего HTTP-запроса
            uint32_t timeLoading = 0;                                   //Таймер для ожидания догрузки запроса

            //Освобождение памяти
            virtual void clearMemory();
            //Опрос на входящее соединение от клиента
            virtual char* handleHTTP(bool isContinue = false);
            //Возвращает строку статуса по коду requestCode
            virtual const __FlashStringHelper* getTypeRequestCode(uint16_t requestCode);
            //Функция отправляющая HTTP-заголовки клиенту
            virtual void sendHeaders(uint16_t requestCode, const char *ContentType, size_t size, bool isContinue = false);
            //Функция определяет конечный символ URL запроса
            uint16_t searchEndCharURL(const char *data, uint16_t size);
            //Функция определяет начальный символ передаваемого параметра
            uint16_t searchStartChar(char *data, const char *param, uint16_t size, byte sizeParam);
            //Функция определяет конечный символ передаваемого параметра
            uint16_t searchEndChar(char *data, uint16_t startChar, uint16_t size);
            //Функция выделяет отдельные аргументы и их имена
            void getArguments(uint16_t sizeArgs);
            //Функция ищет HTTP-запрос в списке и вызывает нужный обработчик
            void isHttpRequest();
            //Декодер HEX-последовательностей вида %XX
            char* HEXDecoder(char* str);
    };
};