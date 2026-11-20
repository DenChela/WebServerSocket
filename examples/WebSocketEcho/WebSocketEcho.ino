/*
    WebSocketEcho — эхо-сервер с HTML-клиентом.

    Демонстрирует:
      - WebSocket на том же порту, что и раздача HTML-страницы;
      - запуск хендшейка из обработчика маршрута через sendHeaders(101, ...);
      - колбэки входящих сообщений и закрытия сокета;
      - отправку сообщений сервером (sendMessage).

    Маршруты:
      GET /   — страница с клиентом WebSocket
      GET /ws — upgrade до WebSocket (эхо)

    Перед загрузкой задайте SSID и PASSWORD.
*/

#include <WebSocket.h>

using namespace DenChela;

//Логин и пароль вашей Wi-Fi сети
const char *SSID PROGMEM = "SSID";
const char *PASSWORD PROGMEM = "PASSWORD";

//Содержание HTML страницы 
const char *indexPage PROGMEM = R"rawliteral(<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<title>WebSocket Echo</title>
</head>
<body>
<h1>WebSocket Echo</h1>
<input id="msg" type="text" placeholder="message">
<button onclick="sendMsg()">Send</button>
<pre id="log" style="border:1px solid #999; padding:8px; min-height:100px;"></pre>
<script>
const log = document.getElementById("log");
const ws = new WebSocket("ws://" + location.host + "/ws");
ws.onopen = () => log.append("connected\n");
ws.onclose = e => log.append("closed: " + e.code + "\n");
ws.onmessage = e => log.append("< " + e.data + "\n");
function sendMsg() {
    const m = document.getElementById("msg");
    if (m.value) { log.append("> " + m.value + "\n"); ws.send(m.value); m.value = ""; }
}
</script>
</body>
</html>)rawliteral";

//Список HTTP-запросов для обработки
const char* routes[] PROGMEM = {"/", "/ws"};

//Список функции для обработки зарегистрированных HTTP-запросов
void (*handlers[])(WebServer &server) PROGMEM = {
    [](WebServer &s) {
        s.send(200, PSTR("text/html"), indexPage);
    }, [](WebServer &s) {
        // Хендшейк WebSocket: 101 выполняет Upgrade и переводит сокет в WS-режим.
        // Повторное подключение при живом соединении получит 503.
        s.send(101, PSTR("websocket"), PSTR(""));
    }
};

//Объект WebSocket с CallBack для обработки входящего Фрема
WebSocket ws(80, [](const char *data) {
    Serial.print(F("received: "));
    Serial.println(data);
    ws.sendMessage(data);
});

void setup() {
    Serial.begin(115200);
    WiFi.begin(SSID, PASSWORD);
    while (WiFi.status() != WL_CONNECTED) delay(500);
    Serial.print(F("IP: "));
    Serial.println(WiFi.localIP());
    ws.setNotWriteRequest([](WebServer &server){
        Serial.printf_P((PSTR("Not data: %s\r\n")), server.getURL());
        server.send(404, PSTR("text/plain"), PSTR("Not Found"));
    });
    ws.on(routes, handlers, 2);
    ws.setSocketDisconnect([](uint16_t codeClose, const char *text) {
        Serial.print(F("Socket closed, code: "));
        Serial.print(codeClose);
        Serial.print(F(" ,text "));
        Serial.println(text);
    });
    ws.begin();
}

void loop() {
    ws.handle();
}
