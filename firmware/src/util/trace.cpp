#include "util/trace.h"
#include "pgmspace.h"
#include <stdio.h>
#include <stdarg.h>
#include <map>
#include <tuple>

namespace trace
{

std::string format( const char * format, ... ) {
	va_list arg;
	va_start(arg, format);
	char msg[128];
	vsnprintf(msg, sizeof(msg), format, arg);
	va_end(arg);

	return msg;
}


std::string format( const __FlashStringHelper* format, ... ) {
	va_list arg;
	va_start(arg, format);
	char msg[128];
	vsnprintf_P(msg, sizeof(msg), reinterpret_cast<PGM_P>(format), arg);
	va_end(arg);

	return msg;
}


std::string string_to_hex(const std::string& input) {
    static const char* const lut = "0123456789ABCDEF";
    size_t len = input.length();

    std::string output;
    output.reserve(2 * len);
    for (size_t i = 0; i < len; ++i)
    {
        const unsigned char c = input[i];
        output.push_back(lut[c >> 4]);
        output.push_back(lut[c & 15]);
    }
    return output;
}


void log( const char* level, const char* file, int line, const char* msg ) {
	Serial.printf( "%s (%s:%d): %s\n", level, file, line, msg );
}


std::map<std::string, uint64_t> lastTimes;

uint32_t timeInterval( const std::string& id ) {
    uint64_t currentTime = esp_timer_get_time();

    std::map<std::string, uint64_t>::iterator it;
    bool inserted;
    std::tie(it, inserted) = lastTimes.insert( std::make_pair(id, currentTime) );
    if ( inserted ) {
        return currentTime;
    }

    uint32_t interval = currentTime - it->second;
    it->second = currentTime;
    return interval;
}


}
