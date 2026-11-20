/*
    FilesFromFlash — раздача страниц и стилей из файловой системы.

    Демонстрирует:
      - ответы файлом через send(200, тип, file);
      - файл открывается в обработчике и закрывается после отправки.

    Подготовка:
      1. Создайте в корне файловой системы index.html и другие файлы верстки, включая картинки и mp3
         (для ESP8266: инструмент "LittleFS Data Upload", для ESP32:
         плагин "ESP32 LittleFS Data Upload").
      2. Задайте SSID и PASSWORD.

    Маршруты будут указаны прямо в вашем URL
*/

#include <WebServer.h>
#include <LittleFS.h>
using namespace DenChela;

//Для подключение к сети используйте свой SSID и пароль вашей сети
const char *SSID PROGMEM = "SSID";
const char *PASSWORD PROGMEM = "PASSWORD";

//Список содержание заголовка 
const char *TEXT_HTML PROGMEM = "text/html;charset=\"utf-8\"";
const char *TEXT_CSS PROGMEM = "text/css;charset=\"utf-8\"";
const char *TEXT_JAVASCRIPT PROGMEM = "text/javascript;charset=\"utf-8\"";
const char *TEXT_JSON PROGMEM = "text/json;charset=\"utf-8\"";
const char *IMAGE_PGN PROGMEM = "image/png";
const char *IMAGE_JPG PROGMEM = "image/jpg";
const char *AUDIO_MP3 PROGMEM = "audio/mp3";
const char *FONT_OTF PROGMEM = "font/otf";
const char *TEXT_PLAIN PROGMEM = "text/plain;charset=\"utf-8\"";

WebServer server(80);

const char* getContentType(const char *type) {
    if (type == nullptr) return TEXT_PLAIN;
    else if (strcmp_P(type, PSTR("html")) == 0) return TEXT_HTML;
    else if (strcmp_P(type, PSTR("css")) == 0) return TEXT_CSS;
    else if (strcmp_P(type, PSTR("js")) == 0) return TEXT_JAVASCRIPT;
    else if (strcmp_P(type, PSTR("json")) == 0) return TEXT_JSON;
    else if (strcmp_P(type, PSTR("png")) == 0) return IMAGE_PGN;
    else if (strcmp_P(type, PSTR("jpg")) == 0) return IMAGE_JPG;
    else if (strcmp_P(type, PSTR("mp3")) == 0) return AUDIO_MP3;
    else if (strcmp_P(type, PSTR("otf")) == 0) return FONT_OTF;
    else return TEXT_PLAIN;
}

void setup() {
    Serial.begin(115200);
    if (!LittleFS.begin()) {
        Serial.println("LittleFS mount failed — запишите образ файловой системы");
        while (true) delay(1000);
    }
    WiFi.begin(SSID, PASSWORD);
    while (WiFi.status() != WL_CONNECTED) delay(500);
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    server.setNotWriteRequest([](WebServer &s){
        const char *URL;
        if (strcmp_P(s.getURL(), PSTR("/")) == 0) URL = "index.html";
        else URL = s.getURL();
        uint16_t lengthURL = strlen(URL);
        const char *type = nullptr;
        for (uint16_t i = 0; i < lengthURL; i++) {
            if (URL[i] == '.') type = &URL[i + 1];
        }
        File file = LittleFS.open(URL, "r");
        if (file) {
            s.send(200, getContentType(type), file);
            file.close();
        } else s.send(404, TEXT_PLAIN, PSTR("File not found"));
    });
    server.begin();
    LittleFS.begin();
}

void loop() {
    server.handle();
}