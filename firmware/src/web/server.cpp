#include "web/server.h"
#include "util/trace.h"



static const size_t EncodedSampleSize = sizeof(int16_t) * 2;
static const size_t EncodedSamplesSize = meter::SampleBasedMeter::MeasuresSize * EncodedSampleSize;
static const size_t MeasuresSentSize = 
                    sizeof(uint64_t) +      // Time
                    sizeof(float) * 2 +     // scale factors
                    EncodedSamplesSize;
static const size_t MeasuresPerPacketSent = 3;
static const size_t PacketSentSize = MeasuresSentSize * MeasuresPerPacketSent;




namespace web {

Server::Server( uint16_t port ): m_ws( new websocket::Server(port) ), m_sendBufferPos(NULL) {
}


Server::~Server() {
    delete m_ws;
}


void Server::begin() {
    m_ws->begin();
}


inline void transfer( const meter::SampleBasedMeter::Measures& samples, void* buffer ) {
	std::transform( samples.begin(), samples.end(),
                    reinterpret_cast<std::pair<int16_t, int16_t>*>(buffer), 
                    [](const meter::SampleBasedMeter::Measure& sample) {
                        return std::make_pair( sample.voltage(), sample.current() );
                    } );

/*    int16_t* pos = reinterpret_cast<int16_t*>(buffer);
	std::for_each( samples.begin(), samples.end(),
        [&](const meter::SampleBasedMeter::Measure& sample) {
		    *pos = sample.voltage();
		    *(pos+1) = sample.current();
		    pos += 2;
	    });
	return pos; */
}


void Server::send( uint64_t time, const std::pair<float, float>& scaleFactors,
                    const meter::SampleBasedMeter::Measures& samples ) {
    if ( (m_sendBufferPos==NULL) && 
        ((m_ws->count() == 0) || !m_ws->availableForWrite()) ) {
    //    TRACE( "Samples has been discarded" );
        return;
    }

    if ( m_sendBufferPos == NULL ) {
        m_sendBuffer = m_ws->makeBuffer( PacketSentSize );
        (*m_sendBuffer)++;
        m_sendBufferPos = m_sendBuffer->get();
        m_sendBufferEnd = m_sendBufferPos + PacketSentSize;
    }

    memcpy( m_sendBufferPos, &time, sizeof(time) );
    m_sendBufferPos += sizeof(time);

    memcpy( m_sendBufferPos, &scaleFactors, sizeof(scaleFactors) );
    m_sendBufferPos += sizeof(scaleFactors);

    transfer( samples, m_sendBufferPos );
    m_sendBufferPos += EncodedSamplesSize;
    
    if ( m_sendBufferPos == m_sendBufferEnd ) {
        m_ws->send( m_sendBuffer );
        (*m_sendBuffer)--;
        m_sendBufferPos = NULL;
    }
}

}