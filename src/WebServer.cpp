#include "WebServer.h"
using namespace DenChela;

static bool strncmp_PP (const char *str1, const char *str2, uint16_t length) {
    for (uint16_t i = 0; i < length; i++) {
        if (pgm_read_byte(&str1[i]) == '\0') return false;
        if (pgm_read_byte(&str1[i]) != pgm_read_byte(&str2[i])) return false;
    }
    return true;
}

WebServer::WebServer(const uint16_t port):WiFiServer(port) {
    arg = nullptr;
    nameArg = nullptr;
    listArgs = nullptr;
    URL = nullptr;
}

WebServer::~WebServer() {
    clearMemory();
    client.stop();
}

void WebServer::handle() {
    switch (statusConnect) {
        case WAIT:
            if (client && client.connected()) {
                statusConnect = CONNECTION;
                timeLoading = millis();
            } else client = accept();
            break;
        case CONNECTION:
            if (client.available() > 0) {
                sizeHttpData = client.available();
                statusConnect = LOADING;
                timeLoading = millis();
            }
            else if (!client || !client.connected() || millis() - timeLoading > TIME_RESPONSE) {
                statusConnect = WAIT;
                client.stop();
                client = accept();
            }
            break;
        case LOADING:
            if (client && client.connected()) {
                if (millis() - timeLoading > TIME_WAITING) {
                    if (sizeHttpData != client.available()) {
                        sizeHttpData = client.available();
                        timeLoading = millis();
                    } else statusConnect = PROCESSING;
                }
            } else {
                statusConnect = WAIT;
                client.stop();
                client = accept();
            }
            break;
        case PROCESSING:
            clearMemory();
            if (sizeHttpData <= MAX_LENGTH_DATA) {
                if (handleHTTP()) isHttpRequest();
                else send(400, PSTR("text/plain"), PSTR("Not data"));
            }
            client.stop();
            client = accept();
            statusConnect = WAIT;
            break;
        default:
            client.stop();
            statusConnect = WAIT;
    }
}

void WebServer::on(const char** _httpRequests, void(**_functions)(WebServer &server), uint8_t _countHttpRequest) {
    httpRequests = _httpRequests;
    functions = _functions;
    countHttpRequest = _countHttpRequest;
}

void WebServer::setNotWriteRequest(void (*function)(WebServer &server)) {
    isNotWriteRequest = function;
}

void WebServer::send(uint16_t requestCode, const char *contentType, const char *data) {
    sendHeaders(requestCode, contentType, 0);
    uint16_t length = strlen_P(data);
    if (requestCode != 101) client.write_P(data, length);
}

void WebServer::send(uint16_t requestCode, const char *contentType, File &file) {
    uint32_t size = file.available();
    sendHeaders(requestCode, contentType, size);
    uint8_t buff[128];
    uint16_t parts = size / 128;
    for (uint16_t i = 0; i <= parts && client.connected(); i++) {
        if (i != parts) {
            uint16_t byteBuff = file.read(buff, 128);
            if (byteBuff != 128) {
                client.stop();
                return;
            }
            client.write(buff, 128);
        } else if (size % 128 > 0) {
            uint16_t byteBuff = (uint16_t)file.read(buff, size % 128);
            if (size % 128 != byteBuff) {
                client.stop();
                return;
            } else
            client.write(buff, size % 128);
        }
    }
}

const char* WebServer::getURL() {
    return URL;
}

const uint16_t WebServer::arguments() {
    return args;
}

const char* WebServer::getNameArg(uint16_t index) {
    if (index < args && nameArg && nameArg[index]) return nameArg[index];
    else return &notValue;
}

const char* WebServer::getArg(uint16_t index) {
    if (index < args && arg && arg[index]) return arg[index];
    else return &notValue;
}

void WebServer::clearMemory() {
    if (arg != nullptr) {
        delete [] arg;
        arg = nullptr;
    }
    if (nameArg != nullptr) {
        delete [] nameArg;
        nameArg = nullptr;
    }
    if (listArgs != nullptr) {
        delete [] listArgs;
        listArgs = nullptr;
    }
    if (URL != nullptr) {
        delete [] URL;
        URL = nullptr;
    }
    args = 0;
    contentLength = 0;
}

char* WebServer::handleHTTP(bool isContinue) {
    //Выделяем память в стеке и записываем новые заголовки
    char data[sizeHttpData + 1];
    client.readBytes(data, sizeHttpData);
    data[sizeHttpData] = '\0';
    //Делаем запись типа запроса
    if (strncmp_P(data, PSTR("GET"), 3) == 0) {
        method = GET;
    } else if (strncmp_P(data, PSTR("POST"), 4) == 0) {
        method = POST;
    } else {
        method = NONE_METHOD;
        return nullptr;
    }
    //Записываем URL в строку
    uint16_t charStart = method + 3;
    uint16_t charEnd = searchEndCharURL(data, sizeHttpData);
    if (charEnd >= charStart) {
        URL = new char[charEnd - charStart + 1];
        strncpy(URL, data + charStart, charEnd - charStart);
        URL[charEnd - charStart] = '\0';
    } else {
        return nullptr;
    }
    //Если метод POST, то определяем тип кодировки отправляемых данных
    if (method == POST) {
        charStart = searchStartChar(data, PSTR("Content-Type"), sizeHttpData, 12);
        if (charStart) {
            if (strncmp_P(data + charStart, PSTR("application/x-www-form-urlencoded"), 33) == 0)
                contentType = APPLICATION_X_WWW_FORM_URLENCODED;
            else if (strncmp_P(data + charStart, PSTR("text/plain"), 10) == 0)
                contentType = TEXT_PLAIN;
            else {
                contentType = NONE_TYPE;
                return nullptr;
            }
        } else {
            contentType = NONE_TYPE;
            return nullptr;
        }
        charStart = searchStartChar(data, PSTR("Content-Length"), sizeHttpData, 14);
        if (charStart) contentLength = atoi(data + charStart);
        else {
            contentType = NONE_TYPE;
            return nullptr;
        }
    }
    //Записываем аргументы (если есть) в строку запроса GET
    if (method == GET) {
        contentType = APPLICATION_X_WWW_FORM_URLENCODED;
        charStart = 0;
        charEnd = 0;
        args = 0;
        for (uint16_t i = 4 + method; i < sizeHttpData; i++) {
            if (data[i] == ' ') break;
            else if (data[i] == '?') {
                charStart = i + 1;
                args = 1;
                break;
            }
        }
        if (charStart) {
            for (uint16_t i = charStart; i < sizeHttpData; i++) {
                if (data[i] == ' ' || data[i] == '\r' || data[i] == '\n') {
                    charEnd = i;
                    break;
                } 
            }
            if (charEnd > charStart) {
                listArgs = new char[charEnd - charStart + 1];
                strncpy(listArgs, data + charStart, charEnd - charStart);
                listArgs[charEnd - charStart] = '\0';
                getArguments(charEnd - charStart);
            } else args = 0;
        }
    //Записываем аргументы (если они есть) в строку запроса POST
    } else if (method == POST && (contentType == APPLICATION_X_WWW_FORM_URLENCODED ||
                                                    contentType == TEXT_PLAIN) && contentLength) {
        charStart = 0;
        for (uint16_t i = 15 + strlen(URL); i < sizeHttpData - 4; i++) {
            if (strncmp_P(data + i, PSTR("\r\n\r\n"), 4) == 0) {
                charStart = i + 4;
                break;
            }
        }
        if (charStart && contentLength == sizeHttpData - charStart) {
            listArgs = new char[contentLength + 1];
            strncpy(listArgs, data + charStart, contentLength);
            listArgs[contentLength] = '\0';
            args = 1;
            getArguments(contentLength);
        } else args = 0;
    } else args = 0;
    //Мостик для переноса запроса в WebSocket
    if (isContinue) {
        char *dataCopy = new char[sizeHttpData + 1];
        dataCopy[0] = '\0';
        strcpy(dataCopy, data);
        dataCopy[sizeHttpData] = '\0';
        return dataCopy;
    } else return URL;
}

const __FlashStringHelper* WebServer::getTypeRequestCode(uint16_t requestCode) {
    switch (requestCode) {
        case 200:
            return F(" Ok\r\n");
        case 400:
            return F(" Bad Request\r\n");
        case 404:
            return F(" Not Found\r\n");
    }
    return F(" \r\n");
}

void WebServer::sendHeaders(uint16_t requestCode, const char *contentType, size_t size, bool isContinue) {
    if (!isContinue) {client.print(F("HTTP/1.1 ")); client.print(requestCode); client.print(getTypeRequestCode(requestCode));}
    client.printf_P(PSTR("Content-type: %s\r\n"), contentType);
    client.print(F("Connection: close\r\n"));
    if (strncmp_PP(contentType, PSTR("image"), 5) || strncmp_PP(contentType, PSTR("audio"), 5)) {
        client.print(F("Accept-Ranges: bytes\r\n"));
        client.print(F("Cache-Control: max-age=86400\r\n"));
        client.printf_P(PSTR("Content-Length: %u\r\n"), size);
    }
    client.print(F("\r\n"));
}

uint16_t WebServer::searchEndCharURL(const char *data, uint16_t size) {
    if (method == GET) {
        for (uint16_t i = 4; i < size; i++) {
            if (data[i] == ' ' || data[i] == '?') return i;
        }
    } else if (method == POST) {
        for (uint16_t i = 5; i < size; i++) {
            if (data[i] == ' ') return i;
        }
    }
    return (uint16_t)0;
}

uint16_t WebServer::searchStartChar(char *data, const char *param, uint16_t size, byte sizeParam) {
    uint16_t startCharParam = 0;
    for (uint16_t i = 0; i < size; i++) {
        if (data[i] == '\n') {
            if (i != size - 1) i++;
            else return 0;
            if (strncmp_P(data + i, param, sizeParam) == 0) {
                startCharParam = i;
                break;
            }
        }
    }
    if (!startCharParam) return 0;
    for (uint16_t i = startCharParam; i < size; i++) {
        if (data[i] == ':') {
            if (data[i + 1] == ' ') {
                return i + 2;
            } else return i + 1;
        } else if (data[i] == '\r') return 0;
    }
    return 0;
}

uint16_t WebServer::searchEndChar(char *data, uint16_t startChar, uint16_t size) {
    for (uint16_t i = startChar + 1; i < size; i++) {
        if (data[i] == '\n') return i;
    }
    return 0;
}

void WebServer::getArguments(uint16_t sizeArgs) {
    if (!args) return;
    //Выделяем аргументы, если тип кодировки "application/x-www-form-urlencoded"
    if (contentType == APPLICATION_X_WWW_FORM_URLENCODED) {
        for (uint16_t i = 0; i < sizeArgs; i++) {
            if (listArgs[i] == '&') args++;
        }
        nameArg = new char*[args];
        arg = new char*[args];
        for (uint16_t i = 0; i < args; i++) {
            nameArg[i] = nullptr;
            arg[i] = nullptr;
        }
        uint16_t position = 0;
        uint16_t count = 0;
        for (; position < sizeArgs; position++) {
            if (listArgs[position] == '&') {
                listArgs[position] = '\0';
                count++;
                if (position == sizeArgs - 1) return;
            } else break;
        }
        nameArg[count++] = &listArgs[position++];
        for (; position < sizeArgs; position++) {
            if (listArgs[position] == '&') {
                listArgs[position] = '\0';
                if (position != sizeArgs - 1) nameArg[count++] = listArgs + position + 1;
            }
        }
        for (uint16_t i = 0; i < args; i++) {
            if (nameArg[i] == nullptr || nameArg[i][0] == '=') continue;
            uint16_t argLength = strlen(nameArg[i]);
            for (uint16_t j = 0; j <= argLength; j++) {
                if (nameArg[i][j] == '=') {
                    nameArg[i][j] = '\0';
                    if (j != argLength) {
                        arg[i] = &nameArg[i][j + 1];
                        break;
                    }
                }
            }
        }
        for (uint16_t i = 0; i < args; i++) {
            if (nameArg[i] != nullptr) {
                char *str = HEXDecoder(nameArg[i]);
                nameArg[i] = str;
                if (str == nullptr) {
                    arg[i] = nullptr;
                    continue;
                }
                if (arg[i] != nullptr) {
                    str = HEXDecoder(arg[i]);
                    arg[i] = str;
                }
            }
        }
    //Выделяем аргументы, если тип кодировки text/plain
    } else if (contentType == TEXT_PLAIN) {
        //Ошибка: аргумент начинается с переноса строки либо состоит из одного возврата каретки
        if (listArgs[0] == '\n' || (sizeArgs == 1 && listArgs[0] == '\r')) {args = 0;return;}
        else if (sizeArgs == 1) { 
            nameArg = new char*[1];
            if (listArgs[0] != '=') nameArg[0] = listArgs;
            else nameArg[0] = nullptr;
            arg = new char*[1];
            arg[0] = nullptr;
            return;
        //Проверяем правильность структуры терминатора: если он разделён неправильно, аргументы откидываем
        } else {
            for (uint16_t i = 0; i < sizeArgs; i++) {
                if (listArgs[i] == '\r') {
                    if (listArgs[i + 1] != '\n') {args = 0;return;}
                } else if (listArgs[i + 1] == '\n') {
                    if (listArgs[i] != '\r') {args = 0;return;}
                }
            }
        }
        for (uint16_t i = 0; i < sizeArgs; i++) {
            if (listArgs[i] == '\r' && listArgs[i + 1] == '\n') args++;
        }
        nameArg = new char*[args];
        arg = new char*[args];
        uint16_t count = 1;
        if (listArgs[0] != '\r') nameArg[0] = listArgs;
        else nameArg[0] = nullptr;
        for (uint16_t i = 0; i < sizeArgs - 1; i++) {
            if (listArgs[i] == '\r' && listArgs[i + 1] == '\n') {
                listArgs[i] = '\0';
                listArgs[i + 1] = '\0';
                if (listArgs[i + 2] == '\r') nameArg[count++] = nullptr;
                else nameArg[count++] = listArgs + i + 2;
            }
        }
        for (uint16_t i = 0; i < args; i++) {
            arg[i] = nullptr;
            if (nameArg[i] == nullptr) {arg[i] = nullptr;continue;}
            uint16_t sizeArg = strlen(nameArg[i]);
            for (uint16_t j = 0; j < sizeArg; j++) {
                if (nameArg[i][j] == '=') {
                    if (j == sizeArg - 1) {
                        nameArg[i][j] = '\0';
                        if (sizeArg == 1) nameArg[i] = nullptr;
                    } else {
                        arg[i] = &nameArg[i][j + 1];
                        nameArg[i][j] = '\0';
                    }
                    break;
                }
            }
        }
        //Если тип кодировки какой-то другой — аргументы не разбираем
    } else args = 0;
}

void WebServer::isHttpRequest() {
    if (countHttpRequest && URL != nullptr) {
        for (byte i = 0; i < countHttpRequest; i++) {
            if (strcmp_P(URL, httpRequests[i]) == 0) {
                functions[i](*this);
                return;
            }
        }
    }
    if (isNotWriteRequest != nullptr) isNotWriteRequest(*this);
}

char* WebServer::HEXDecoder(char* str) {
    uint16_t length = strlen(str) + 1;
    uint16_t position = 0;
    for (uint16_t i = 0; i < length; i++) {
        if (str[i] == '\0') {
            str[position] = '\0';
            return str;
        }
        if (str[i] != '%') str[position++] = str[i];
        else {
            if (i < length - 3) {
                i++;
                uint8_t symbol;
                if (str[i] >= '0' && str[i] <= '9') symbol = (str[i] - '0') * 16;
                else if (str[i] >= 'A' && str[i] <= 'F' ) symbol = (str[i] - 'A' + 10) * 16;
                else if (str[i] >= 'a' && str[i] <= 'f') symbol = (str[i] - 'a' + 10) *16;
                else return nullptr;
                i++;
                if (str[i] >= '0' && str[i] <= '9') symbol += str[i] - '0';
                else if (str[i] >= 'A' && str[i] <= 'F' ) symbol += str[i] - 'A' + 10;
                else if (str[i] >= 'a' && str[i] <= 'f') symbol += str[i] - 'a' + 10;
                else return nullptr;
                str[position++] = symbol;
            } else return nullptr;
        }
    }
    return nullptr;
}