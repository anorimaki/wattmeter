#ifndef OTA_H
#define OTA_H

namespace ota {

extern volatile bool inProgress;

void init( const char* hostName );
void handle();

}

#endif