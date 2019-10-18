#include "wifi/wifi.h"
#include "util/trace.h"
#include <esp_wifi.h>
#include "wifi_manager.h"

namespace wifi {

void connecting(void *pvParameter){
	TRACE( F("Connecting") );
}

void ipGotten(void *pvParameter){
	TRACE( "IP address: %s", wifi_manager_get_sta_ip_string() );
}



bool init( const char* hostname ) {
    wifi_manager_start();
    wifi_manager_set_callback(ORDER_CONNECT_STA, &connecting);
    wifi_manager_set_callback(EVENT_STA_GOT_IP, &ipGotten);
    delay ( 2500 );
    return true;
}

void stop() {
	esp_wifi_stop();
}

}