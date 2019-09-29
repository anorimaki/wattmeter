#include "web/defaultwebsocketserver.h"
#include "util/trace.h"

namespace web {

namespace websocket {

void onEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, 
            AwsEventType type, void * arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_CONNECT) {
        TRACE("Client %d connected", client->id() );
    }
    else if (type == WS_EVT_DISCONNECT) {
        TRACE("Client %d disconnected", client->id() );
    }
    else if (type == WS_EVT_ERROR) {
        TRACE("Client %d error: (%u) %s", client->id(), *((uint16_t*)arg), (char*)data );
    }
    else {
        TRACE("Other event: %d", type);
    }
}


Server::Server( uint16_t port ): m_web(port), m_ws("/ws") {
    m_ws.onEvent(onEvent);
    m_web.addHandler(&m_ws);
}

}

}