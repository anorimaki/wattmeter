#ifndef WEB_WEBSERVER_H
#define WEB_WEBSERVER_H

#include "model/sample.h"
#include "web/defaultwebsocketserver.h"
#include "util/circularbuffer.h"
#include <stdint.h>
#include <array>
#include <deque>
#include <functional>


namespace web {

class Server {
private:
    typedef CircularBuffer<model::Samples, 8> SamplesBuffer;

public:
    Server( uint16_t port );
    ~Server();

    void begin();
    void send( const model::Samples& samples );

private:
    Server( const Server& ) = delete;
    Server& operator=( const Server& ) = delete; 

private:
    websocket::Server* m_ws;
    uint8_t* m_sendBufferPos;
    uint8_t* m_sendBufferEnd;    
    websocket::Buffer* m_sendBuffer;
};

}

#endif