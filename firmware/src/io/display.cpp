#include "io/display.h"
#include "util/trace.h"
#include "Adafruit_GFX.h"

namespace io {

void show( float voltage, float current, float power ) {
	TRACE( "Voltage: %f, Current: %f, Power: %f", voltage, current, power);
}

}