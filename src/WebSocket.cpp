#include "WebSocket.h"
using namespace DenChela;

WebSocket::WebSocket(const uint16_t port, void (*_readMessage)(const char* data)):WebServer(port) {
    readMessage = _readMessage;
    socketDisconnect = nullptr;
    connection = nullptr;
    upgrade = nullptr;
    secWebsocketKey = nullptr;
}

WebSocket::~WebSocket() {
    socketClose(NORMAL_CLOSURE);
    clearMemory();
}

void WebSocket::handle() {
    switch (statusSocket) {
        case CLOSE:
            if (clientSocket && clientSocket.connected()) socketClose(PROTOCOL_ERROR);
            break;
        case START:
            timePing = millis();
            statusSocket = CONNECTED;
            break;
        case CONNECTED:
            if (clientSocket && clientSocket.connected()) {
                if (clientSocket.available() >= 4) {
                    getIsMaskAndSizeFrame();
                    if (statusSocket == CONNECTED) {
                        statusSocket = LOADING_FRAME;
                        timeLimitLoadingFrame = millis();
                    }
                }
                pingPong();
            } else {
                statusSocket = CLOSE;
                if (socketDisconnect != nullptr) socketDisconnect(GOING_AWAY, &notValue);
                clientSocket.stop();
            }
            break;
        case LOADING_FRAME:
            if ((isMask && clientSocket.available() >= sizeFrame + 4) || (!isMask && clientSocket.available() >= sizeFrame)) {
                readPayload();
                if (statusSocket == LOADING_FRAME) statusSocket = CONNECTED;
            } else if (!clientSocket || !clientSocket.connected()) {
                if (socketDisconnect != nullptr) socketDisconnect(GOING_AWAY, &notValue);
                clientSocket.stop();
                statusSocket = CLOSE;
            } else if (millis() - timeLimitLoadingFrame > TIME_LIMIT_READ_FRAME) {
                if (socketDisconnect != nullptr) socketDisconnect(GOING_AWAY, &notValue);
                socketClose(GOING_AWAY);
            }
            break;
        default:
            statusSocket = CLOSE;
    }
    WebServer::handle();
}

void WebSocket::sendMessage(const char *message) {
    if (statusSocket == CLOSE) return;
    uint16_t length = strlen_P(message);
    clientSocket.write(0x81);
    if (length < 126) clientSocket.write((uint8_t)length);
    else {
        clientSocket.write((uint8_t)126);
        clientSocket.write((uint8_t)((length & 0xFF00) >> 8));
        clientSocket.write((uint8_t)(length & 0X00FF));
    }
    clientSocket.write_P(message, length);
}

bool WebSocket::isConnected() {
    return statusSocket != CLOSE;
}

void WebSocket::socketClose(uint16_t codeClose) {
    if (statusSocket == CLOSE) return;
    if (codeClose < 1000 || codeClose >= 5000) codeClose = 1000;
    clientSocket.write(0x88);
    clientSocket.write(0x02);
    clientSocket.write((codeClose & 0xFF00) >> 8);
    clientSocket.write(codeClose & 0x00FF);
    clientSocket.stop();
    statusSocket = CLOSE;
}

void WebSocket::socketClose(uint16_t codeClose, const char* text) {
    if (statusSocket == CLOSE) return;
    if (codeClose < 1000 || codeClose >= 5000) codeClose = 1000;
    uint16_t length = strlen_P(text);
    if (length > 123) {
        socketClose(codeClose);
        return;
    }
    clientSocket.write((uint8_t)0x88);
    clientSocket.write((uint8_t)length + 2);
    clientSocket.write((uint8_t)((codeClose & 0xFF00) >> 8));
    clientSocket.write((uint8_t)(codeClose & 0x00FF));
    clientSocket.write_P(text, length);
    clientSocket.stop();
    statusSocket = CLOSE;
}

void WebSocket::setSocketDisconnect(void (*function)(uint16_t codeClose, const char* text)) {
    socketDisconnect = function;
}

void WebSocket::clearMemory() {
    if (secWebsocketKey != nullptr) {
        delete [] secWebsocketKey;
        secWebsocketKey = nullptr;
    }
    if (upgrade != nullptr) {
        delete [] upgrade;
        upgrade = nullptr;
    }
    if (connection != nullptr) {
        delete [] connection;
        connection = nullptr;
    }
    WebServer::clearMemory();
}

char* WebSocket::handleHTTP(bool isContinue) {
    char *dataCopy = WebServer::handleHTTP(true);
    if (dataCopy == nullptr) return nullptr;
    char data[sizeHttpData + 1];
    strcpy(data, dataCopy);
    delete [] dataCopy;
    dataCopy = nullptr;
    uint16_t charStart = 0;
    uint16_t charEnd = 0;
    //Записываем значение Connection в строку
    charStart = searchStartChar(data, PSTR("Connection"), sizeHttpData, 10);
    if (charStart) {
        charEnd = searchEndChar(data, charStart, sizeHttpData);
        if (charEnd > charStart) {
            connection = new char[charEnd - charStart];
            strncpy(connection, data + charStart, charEnd - charStart - 1);
            connection[charEnd - charStart - 1] = '\0';
        }
    }
    //Записываем значение Upgrade, если оно есть в заголовках
    if (connection != nullptr && strstr_P(connection, PSTR("Upgrade")) != 0) {
        charStart = searchStartChar(data, PSTR("Upgrade"), sizeHttpData, 7);
        if (charStart) {
            charEnd = searchEndChar(data, charStart, sizeHttpData);
            if (charEnd > charStart) {
                upgrade = new char[charEnd - charStart];
                strncpy(upgrade, data + charStart, charEnd - charStart - 1);
                upgrade[charEnd - charStart - 1] = '\0';
            }
        }
        if (upgrade != nullptr && strcmp_P(upgrade, PSTR("websocket")) == 0) {
            charStart = searchStartChar(data, PSTR("Sec-WebSocket-Key"), sizeHttpData, 17);
            if (charStart) {
                charEnd = searchEndChar(data, charStart, sizeHttpData);
                if (charEnd > charStart) {
                    secWebsocketKey = new char[charEnd - charStart + 37];
                    strncpy(secWebsocketKey, data + charStart, charEnd - charStart - 1);
                    secWebsocketKey[charEnd - charStart - 1] = '\0';
                    strcat_P(secWebsocketKey, PSTR("258EAFA5-E914-47DA-95CA-C5AB0DC85B11"));
                    if (statusSocket == CLOSE) statusSocket = START;
                    else if (statusSocket == START) statusSocket = CLOSE;
                } else {
                    delete [] upgrade;
                    upgrade = nullptr;
                }
            }
        }
    }
    return URL;
}

const __FlashStringHelper* WebSocket::getTypeRequestCode(uint16_t requestCode) {
    switch (requestCode) {
        case 101:
            return F(" Switching Protocols\r\n");
        case 200:
            return F(" Ok\r\n");
        case 400:
            return F(" Bad Request\r\n");
        case 404:
            return F(" Not Found\r\n");
        case 503:
            return F(" Service Unavailable\r\n");
    }
    return F(" \r\n");
}

void WebSocket::sendHeaders(uint16_t requestCode, const char *contentType, size_t size, bool isContinue) {
    if (requestCode == 101 && statusSocket != START) requestCode = 503;
    client.print(F("HTTP/1.1 ")); client.print(requestCode); client.print(getTypeRequestCode(requestCode));
    if (requestCode == 101 && statusSocket == START) {
        uint8_t hash[20];
        SHA1::hash(secWebsocketKey, strlen(secWebsocketKey), hash);
        char key[Base64::encodedLength(20) + 1];
        Base64::encode(hash, 20, key);
        key[Base64::encodedLength(20)] = '\0';
        client.print(F("Upgrade: websocket\r\n"));
        client.print(F("Connection: Upgrade\r\n"));
        client.print(F("Sec-WebSocket-Version: 13\r\n"));
        client.printf_P(PSTR("Sec-WebSocket-Accept: %s\r\n"), key);
        client.print(F("\r\n"));
        clientSocket = client;
        client = WiFiClient();
        statusSocket = CONNECTED;
    } else {
        WebServer::sendHeaders(requestCode, contentType, size, true);
    }
}

void WebSocket::pingPong() {
    //Отправляем сигнал PING с текстом "0" и ставим таймер на ожидание ответа
    if (!isPing && millis() - timePing >= TIME_PERIOD_PING) {
        clientSocket.write_P((const char*)SOCKET_PING, sizeof(SOCKET_PING));
        timePing = millis();
        isPing = true;
    //Если сигнал PONG не пришёл в указанное время, разрываем соединение
    } else if (isPing && millis() - timePing >= TIME_PING) {
        if (socketDisconnect != nullptr) socketDisconnect(PROTOCOL_ERROR, &notValue);
        socketClose(PROTOCOL_ERROR);
    }
}

void WebSocket::getIsMaskAndSizeFrame() {
    typeFrame = (uint8_t)clientSocket.read() & 0x0F;
    sizeFrame = (uint8_t)clientSocket.read();
    isMask = (sizeFrame & (uint8_t)0x80) >> 7;
    sizeFrame &= (uint16_t)0x7F;
    if (sizeFrame == 126) {
        sizeFrame = (uint16_t)clientSocket.read() << 8;
        sizeFrame |= (uint16_t)clientSocket.read();
        if (sizeFrame > MAX_LENGTH_PAYLOAD) {
            socketClose(MESSAGE_TOO_BIG);
            if (socketDisconnect != nullptr) socketDisconnect(MESSAGE_TOO_BIG, &notValue);
        }
    } else if (sizeFrame >= 127) {
        socketClose(MESSAGE_TOO_BIG);
        if (socketDisconnect != nullptr) socketDisconnect(MESSAGE_TOO_BIG, &notValue);
    }
}

void WebSocket::readPayload() {
    //Выделяем память в стеке под размер нашего сообщения и нулевой символ
    char textPayload[sizeFrame + 1];
    if (isMask) {
        //Если есть маска, считываем её и декодируем сообщение
        uint8_t mask[4];
        clientSocket.read(mask, 4);
        clientSocket.read(textPayload, sizeFrame);
        uint8_t posMask = 0;
        for (uint16_t i = 0; i < sizeFrame; i++) {
            if (posMask >= 4) posMask = 0;
            textPayload[i] ^= mask[posMask++];
        }
    } else {
        //Если маски нет, просто считываем сообщение
        clientSocket.read(textPayload, sizeFrame);
    }
    textPayload[sizeFrame] = '\0';
    //Обрабатываем считанное сообщение по типу кадра
    switch (typeFrame) {
        case 1:
            readMessage(textPayload);
            break;
        case 8: {
            uint16_t codeClose = NORMAL_CLOSURE;
            if (sizeFrame >= 2) {
                codeClose = (uint16_t)textPayload[0] << 8;
                codeClose |= textPayload[1];
            }
            if (socketDisconnect != nullptr)
                socketDisconnect(codeClose, sizeFrame > 2 ? textPayload + 2 : &notValue);
            socketClose(codeClose);
        } break;
        case 9:
            if (sizeFrame > 125) {
                socketClose(PROTOCOL_ERROR);
                if (socketDisconnect != nullptr) socketDisconnect(PROTOCOL_ERROR, &notValue);
                break;
            }
            clientSocket.write(0x8A);
            clientSocket.write(sizeFrame);
            clientSocket.write(textPayload, sizeFrame);
            break;
        case 10:
            if (sizeFrame != 1 || textPayload[0] != 0) {
                socketClose(PROTOCOL_ERROR);
                if (socketDisconnect != nullptr) socketDisconnect(PROTOCOL_ERROR, &notValue);
            } else {
                isPing = false;
                timePing = millis();
            }
            break;
        default:
            socketClose(UNSUPPORTED_DATA);
            if (socketDisconnect != nullptr) socketDisconnect(UNSUPPORTED_DATA, &notValue);
    }
}