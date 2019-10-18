#ifndef SSD1306_H
#define SSD1306_H

#include "Adafruit_SSD1306.h"

class SSD1306: public Adafruit_GFX {
public:
    SSD1306(uint8_t w, uint8_t h);
    virtual ~SSD1306();

    void init();
    void drawPixel(int16_t x, int16_t y, uint16_t color);

    void clearDisplay();
    void display();

private:
    uint8_t *m_buffer;
    size_t m_size;
};

#endif