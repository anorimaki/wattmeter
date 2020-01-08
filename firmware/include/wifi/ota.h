#ifndef OTA_H
#define OTA_H

#include <functional>

namespace ota {

void init( const char* hostName, std::function<void()> onStart );
void handle();

}

#endif