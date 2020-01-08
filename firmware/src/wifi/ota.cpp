#include "wifi/ota.h"
#include "util/trace.h"
#include <ArduinoOTA.h>

namespace ota {

void init( const char* hostname, std::function<void()> onStartCallback ) {
    static unsigned int previousProgressPerCent;

	ArduinoOTA.onStart([onStartCallback]() {
        onStartCallback();
		TRACE("OTA: Update start");
	});

	ArduinoOTA.onEnd([]() {
        TRACE("OTA: Update end");
	});

	ArduinoOTA.onProgress([&](unsigned int progress, unsigned int total) {
        unsigned int progressPerCent = progress / (total / 100);
        if ( previousProgressPerCent != progressPerCent ) {
		    TRACE("OTA: Progress %u%%", progressPerCent);
            previousProgressPerCent = progressPerCent;
        }
	});

	ArduinoOTA.onError([](ota_error_t error) {
        if (error == OTA_AUTH_ERROR) {
			TRACE_ERROR( F("OTA: Auth Failed") );
		}
		else if (error == OTA_BEGIN_ERROR) {
			TRACE_ERROR( F("Begin Failed") );
		}
		else if (error == OTA_CONNECT_ERROR) {
			TRACE_ERROR( F("Connect Failed") );
		}
		else if (error == OTA_RECEIVE_ERROR) {
			TRACE_ERROR( F("Receive Failed") );
		}
		else if (error == OTA_END_ERROR) {
			TRACE_ERROR( F("End Failed") );
		}
	});

	ArduinoOTA.setHostname(hostname);
	ArduinoOTA.begin();
}

void handle() {
	ArduinoOTA.handle();
}

}