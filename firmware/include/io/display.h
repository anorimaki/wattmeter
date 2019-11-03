#ifndef DISPLAY_H
#define DISPLAY_H

#include "io/ssd1306.h"
#include "meter/calculatedmeter.h"
#include "util/trace.h"
#include <Wire.h>


namespace io {

class Display {
public:
    static const int ScreenWidth = 128;
    static const int ScreenHeight = 64;

public:
    Display();

    void init();

    void update( const meter::CalculatedMeasures& measures );

private:
    void mainHeader( uint32_t signalFrequency, uint32_t sampleRate );
    void mainView( const meter::VariableMeasure& voltage, const meter::VariableMeasure& current );
    void mainView( const meter::PowerMeasure& power );

private:
    SSD1306 m_lcd;
};

}

#endif