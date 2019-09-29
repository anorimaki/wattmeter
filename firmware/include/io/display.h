#ifndef DISPLAY_H
#define DISPLAY_H

#include "util/trace.h"
#include "Adafruit_SSD1306.h"
#include <Wire.h>


namespace io {

class Display {
private:
    static const int ScreenWidth = 128;
    static const int ScreenHeight = 64;

public:
    Display(): m_impl(ScreenWidth, ScreenHeight, &Wire) {}

    void init() {
        Wire.begin( 27, 26, 400000UL );
        if(!m_impl.begin(SSD1306_SWITCHCAPVCC, 0x3C, false, false)) {
            TRACE_ERROR( "Error in LCD initialization" );
            return;
        }
            // Clear the buffer.
        m_impl.clearDisplay();
        m_impl.display();
    }

private:
    Adafruit_SSD1306 m_impl;
};

}

#endif