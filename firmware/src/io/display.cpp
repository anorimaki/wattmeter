#include "io/display.h"
#include "util/trace.h"
#include "Adafruit_GFX.h"
#include "driver/i2c.h"

namespace io {


Display::Display(): m_impl(ScreenWidth, ScreenHeight) {

}

void Display::init() {
#if 0
    Wire.begin( 27, 26 );
    if(!m_impl.begin(SSD1306_SWITCHCAPVCC, 0x3C, false, false)) {
        TRACE_ERROR( "Error in LCD initialization" );
        return;
    }
#else
    m_impl.init();
#endif
}

void Display::mainView( const meter::CalculatedMeasures& measures ) {
    m_impl.clearDisplay();
    m_impl.setTextSize(1);
    m_impl.setTextColor(WHITE);
#if 1
    m_impl.setCursor( 1, 16 );
    m_impl.printf("Voltage: %.2f mV", measures.voltage());
    m_impl.setCursor( 1, 25 );
    m_impl.printf("Sample r: %u Hz", measures.sampleRate());
    m_impl.display();
#else
    m_impl.drawPixel(0, 0, 1);
    m_impl.drawPixel(1, 1, 1);
    m_impl.drawPixel(2, 2, 1);
    m_impl.display();
#endif
}

}