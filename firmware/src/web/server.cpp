#include "web/server.h"
#include "util/trace.h"


const size_t EncodedSampleSize = sizeof(float) * 2;
const size_t EncodedSamplesSize = model::SamplesSize * EncodedSampleSize;
static const size_t SendSamplesPacketSize = 3;

void* transfer( const model::Samples& samples, void* buffer ) {
	float* pos = reinterpret_cast<float*>(buffer);
	std::for_each( samples.begin(), samples.end(), [&](const model::Sample& sample) {
		*pos = sample.voltage();
		*(pos+1) = sample.current();
		pos += 2;
	});
	return pos;
}

namespace web {

Server::Server( uint16_t port ): m_ws( new websocket::Server(port) ), m_sendBufferPos(NULL) {
}


Server::~Server() {
    delete m_ws;
}


void Server::begin() {
    m_ws->begin();
}


void Server::send( const model::Samples& samples ) {
    if ( (m_sendBufferPos==NULL) && 
        ((m_ws->count() == 0) || !m_ws->availableForWrite()) ) {
    //    TRACE( "Samples has been discarded" );
        return;
    }

    if ( m_sendBufferPos == NULL ) {
        m_sendBuffer = m_ws->makeBuffer( EncodedSamplesSize * SendSamplesPacketSize );
        (*m_sendBuffer)++;
        m_sendBufferPos = m_sendBuffer->get();
        m_sendBufferEnd = m_sendBufferPos + (EncodedSamplesSize * SendSamplesPacketSize);
    }

    transfer( samples, m_sendBufferPos );
    m_sendBufferPos += EncodedSamplesSize;
    
    if ( m_sendBufferPos == m_sendBufferEnd ) {
        m_ws->send( m_sendBuffer );
        (*m_sendBuffer)--;
        m_sendBufferPos = NULL;
    }
}

}