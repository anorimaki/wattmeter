#ifndef WEB_DEFAULTWEBSOCKETSERVER_H
#define WEB_DEFAULTWEBSOCKETSERVER_H

#include "AsyncWebSocket.h"
#include "ESPAsyncWebserver.h"

namespace web {

namespace websocket {

typedef AsyncWebSocketMessageBuffer Buffer;

class Server {
public:
    Server( uint16_t port );

    void begin() {
        m_web.begin();
    }

    Buffer* makeBuffer( size_t size ) {
        return m_ws.makeBuffer(size);
    }

    void send( Buffer* buffer ) {
        m_ws.binaryAll( buffer );
    }

    bool availableForWrite() {
        return m_ws.availableForWriteAll();
    }

    size_t count() {
        return m_ws.count();
    }

public:
    AsyncWebServer m_web;
    AsyncWebSocket m_ws;
};

}

}

#endif