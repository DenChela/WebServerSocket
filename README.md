# WebServerSocket

Компактный неблокирующий HTTP-сервер с WebSocket для **ESP8266** и **ESP32**.
Один класс опрашивает входящие соединения в `loop()`, а WebSocket поднимается
на том же порту поверх HTTP-хендшейка. Собственные реализации SHA1 и Base64 —
без зависимостей от ядра, `String` и динамической памяти.

## Возможности

- Неблокирующая машина состояний: `handle()` никогда не ждёт сеть дольше констант тайм-аутов
- Маршрутизация HTTP-запросов через `on()` и обработчик «не найдено» через `setNotWriteRequest()`
- Разбор GET- и POST-аргументов (`application/x-www-form-urlencoded`, `text/plain`) с HEX-декодированием `%XX`
- Ответы строкой из PROGMEM или файлом из файловой системы
- WebSocket: текстовые кадры, PING/PONG, закрытие с кодами RFC 6455, колбэки на сообщение и закрытие
- Собственные `SHA1` и `Base64` — одинаковый API на обеих платформах
- Без `String`, аллокаций только под разбор текущего запроса

## Установка

```bash
git clone https://github.com/DenChela/WebServerSocket.git
```

## Быстрый старт (HTTP)

```cpp
#include <WebServer.h>
using namespace DenChela;

const char routeRoot[] PROGMEM = "/";
const char* routes[] = {routeRoot};

void handleRoot(WebServer &server) {
    server.send(200, PSTR("text/html"), PSTR("<h1>Hello!</h1>"));
}

void (*handlers[])(WebServer &) = {handleRoot};

WebServer server(80);

void setup() {
    WiFi.begin("SSID", "PASSWORD");
    while (WiFi.status() != WL_CONNECTED) delay(500);
    server.on(routes, handlers, 1);
}

void loop() {
    server.handle();
}
```

## Быстрый старт (WebSocket)

```cpp
#include <WebSocket.h>
using namespace DenChela;

void onMessage(const char *data) {
    Serial.println(data);
}

WebSocket ws(80, onMessage);

void loop() {
    ws.handle();
    if (ws.isConnected()) ws.sendMessage("hello");
}
```

Хендшейк запускается из обработчика маршрута вызовом `send(101, PSTR("websocket"), PSTR(""))`
— см. пример `WebSocketEcho`.

## API

### WebServer

| Метод | Описание |
|---|---|
| `WebServer(uint16_t port)` | Создание сервера на порту |
| `void handle()` | Неблокирующий опрос клиентов, вызывается из `loop()` |
| `void on(const char** requests, void(**functions)(WebServer&), uint8_t count)` | Регистрация маршрутов; строки маршрутов в PROGMEM |
| `void setNotWriteRequest(void (*f)(WebServer&))` | Обработчик запросов вне списка маршрутов |
| `void send(uint16_t code, const char* type, const char* data)` | Ответ строкой (PROGMEM) |
| `void send(uint16_t code, const char* type, File& file)` | Ответ файлом; файл закрывает вызывающий |
| `const char* getURL()` | URL текущего запроса |
| `uint16_t arguments()` | Количество разобранных аргументов |
| `const char* getNameArg(uint16_t i)` / `getArg(uint16_t i)` | Имя и значение аргумента по индексу (имя аргумента может нести пустое значения) |

### WebSocket (наследует WebServer)

| Метод | Описание |
|---|---|
| `WebSocket(uint16_t port, void (*f)(const char* data))` | Создание с колбэком входящих сообщений |
| `bool isConnected()` | Установлено ли WebSocket-соединение |
| `void sendMessage(const char* message)` | Отправка текстового кадра |
| `void socketClose(uint16_t code)` | Закрытие с кодом |
| `void socketClose(uint16_t code, const char* text = ...)` | Закрытие с кодом и текстом (до 123 байт) |
| `void setSocketDisconnect(void (*f)(uint16_t code, const char* text))` | Колбэк закрытия: и по инициативе клиента, и по ошибке, корме вызова метода socketClose |

Коды закрытия — перечисление `WebSocket::CodeClose`: `NORMAL_CLOSURE`,
`GOING_AWAY`, `PROTOCOL_ERROR`, `UNSUPPORTED_DATA`, `INVALID_PAYLOAD_DATA`,
`POLICY_VIOLATION`, `MESSAGE_TOO_BIG`, `NEGOTIATION_FAILED`, `INTERNAL_ERROR`.

## Ограничения

- Максимальный размер HTTP-запроса — `MAX_LENGTH_DATA` (1536 байт по умолчанию)
- Один клиент на сервер; большие запросы сверх лимита отбрасываются
- Кадры свыше 1536 байт не поддерживаются (`MAX_LENGTH_PAYLOAD`) — соединение закрывается с `MESSAGE_TOO_BIG`
- WebSocket-кадры принимаются как с маской так и без

## Примеры

- **SimpleHTTP** — маршруты, GET-аргументы, управление светодиодом
- **FilesFromFlash** — раздача страниц и стилей из LittleFS
- **WebSocketEcho** — эхо-сервер с HTML-клиентом, PING/PONG и обработкой закрытия

## Лицензия

MIT — см. [LICENSE](LICENSE).
