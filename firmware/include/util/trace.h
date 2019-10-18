#ifndef UTIL_TRACE_H_
#define UTIL_TRACE_H_

#include "Arduino.h"

#define SerialPort Serial

namespace trace {

std::string format( const char* format, ... )  __attribute__ ((format (printf, 1, 2)));
std::string format( const __FlashStringHelper* format, ... );

void log( const char* level, const char* file, int line, const char *msg );

uint64_t timeInterval( const std::string& id );

inline void traceTimeInterval( const std::string& id ) {
    Serial.printf( "%s time interval %llu ms\n", id.c_str(), timeInterval(id) / 1000 );
}

}


#define EMPTY()

#define TRACE_ERROR(...) \
		std::string _formated##__LINE__ = trace::format(__VA_ARGS__);						\
		::trace::log( "Error", __FILE__, __LINE__, _formated##__LINE__.c_str() );

#define TRACE(...) \
		::trace::log( "Info", __FILE__, __LINE__, trace::format(__VA_ARGS__).c_str() )

#define TRACE_ERROR_MSG_AND_RETURN(msg,v) { \
		TRACE_ERROR(msg); \
		return v; }

#define TRACE_ERROR_AND_RETURN(v) { \
		TRACE_ERROR(""); \
		return v; }

#define TRACE_ESP_ERROR_CHECK(x) do {                               \
        esp_err_t __err_rc = (x);                                   \
        if (__err_rc != ESP_OK) {                                   \
            TRACE_ERROR("ESP error: %d", __err_rc);                 \
        }                                                           \
    } while(0);

#endif /* INCLUDE_UTIL_TRACE_H_ */
