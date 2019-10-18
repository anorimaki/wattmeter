#ifndef DISPLAY_H
#define DISPLAY_H

#include "io/ssd1306.h"
#include "meter/calculatedmeter.h"
#include "util/trace.h"
#include <Wire.h>


namespace io {

class Display {
private:
    static const int ScreenWidth = 128;
    static const int ScreenHeight = 64;

public:
    Display();

    void init();

    void mainView( const meter::CalculatedMeasures& measures );

private:
    void initSequence();

private:
    SSD1306 m_impl;
};

}

#endif