#ifndef WEB_WEBSERVER_H
#define WEB_WEBSERVER_H

#include "meter/sampledmeter.h"
#include "web/defaultwebsocketserver.h"
#include <stdint.h>
#include <array>
#include <deque>
#include <functional>


namespace web {

class Server {
public:
    Server( uint16_t port );
    ~Server();

    void begin();
    void send( uint64_t time, const std::pair<float, float>& scaleFactors,
                const meter::SampleBasedMeter::Measures& samples );

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