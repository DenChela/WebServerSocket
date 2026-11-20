/*
    SimpleHTTP — минимальный HTTP-сервер на WebServerSocket.

    Демонстрирует:
      - регистрацию маршрутов через on();
      - чтение GET-аргументов (getArg / getNameArg);
      - обработчик "не найдено" через setNotWriteRequest().

    Маршруты:
      GET /                 — приветственная страница
      GET /led?state=ON|OFF — управление встроенным светодиодом

    Перед загрузкой задайте SSID и PASSWORD.
*/

#include <WebServer.h>

using namespace DenChela;

//Данные вашей Wi-Fi сети
const char *SSID PROGMEM = "SSID";
const char *PASSWORD PROGMEM = "PASSWORD";

//Главная HTML-страница
const char *indexPage PROGMEM = R"rawliteral(<!DOCTYPE html><html><head>
<meta charset="UTF-8">
<title>Led ON || OFF</title>
</head><body>
<h1>LED ON || OFF</h1>
<p>Status: <span id="status"></span></p>
<input type="button" onclick="setState('ON')" value="ON">
<input type="button" onclick="setState('OFF')" value="OFF">
<script type="application/javascript">
function setState(value) {
const xhr = new XMLHttpRequest()
xhr.open("GET", `/led?state=${value}`)
xhr.onload = function(){document.getElementById("status").innerText = xhr.responseText}
xhr.send()}
setState("OFF")
</script></body></html>)rawliteral";

//Список HTTP-запросов для выполнения
const char* routes[] PROGMEM = {"/", "/led"};

//Функции для выполнение под HTTP-запросы
void (*handlers[])(WebServer &s) PROGMEM = {[](WebServer &s) {
    s.send(200, PSTR("text/html"), indexPage);
}, [](WebServer &s) {
    if (s.arguments() && !strcmp_P(s.getNameArg(0), PSTR("state"))) {
        if (strcmp_P(s.getArg(0), PSTR("ON")) == 0) {
            digitalWrite(LED_BUILTIN, LOW);
            s.send(200, PSTR("text/plain"), PSTR("ON"));
        } else if (strcmp_P(s.getArg(0), PSTR("OFF")) == 0) {
            digitalWrite(LED_BUILTIN, HIGH);
            s.send(200, PSTR("text/plain"), PSTR("OFF"));
        } else {
            s.send(400, PSTR("text/plain"), PSTR("Use /led?state=ON|OFF"));
        }
    }
    else s.send(400, PSTR("text/plain"), PSTR("Use /led?state=ON|OFF"));
}};

//Объект экземпляра сервера
WebServer server(80);

void setup() {
    Serial.begin(115200);
    pinMode(LED_BUILTIN, OUTPUT);
    WiFi.begin(SSID, PASSWORD);
    while (WiFi.status() != WL_CONNECTED) delay(500);
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    server.on(routes, handlers, 2);
    server.setNotWriteRequest([](WebServer &s) {
        s.send(404, PSTR("text/plain"), PSTR("Not found"));
    });
    server.begin();
}

void loop() {
    server.handle();
}