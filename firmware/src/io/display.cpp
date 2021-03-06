#include "io/display.h"
#include "util/trace.h"
#include "driver/i2c.h"
#include "Adafruit_GFX.h"
#include "font/dialog8pt.h"
#include "font/dialog13pt.h"
#include "font/dialog17pt.h"


namespace io {


void justify( SSD1306& lcd, int16_t y, const String& text,
            std::function<int16_t (int16_t)> f ) {
    int16_t x1, y1;
    uint16_t w, h;
    lcd.getTextBounds( text, 0, y, &x1, &y1, &w, &h );
    lcd.setCursor( f(w), y );
    lcd.print(text);
}

static void center( SSD1306& lcd, int16_t y, const String& text ) {
    justify( lcd, y, text, [](int16_t width) { return Display::ScreenWidth/2 - (width/2); } );
}


static void right( SSD1306& lcd, int16_t y, const String& text ) {
    justify( lcd, y, text, [](int16_t width) { return (Display::ScreenWidth - width)-3; } );
}


static String adjustUnit( float value, const char* unit ) {
    return (abs(value) < 0.1) ? 
                String(value * 1000, 2) + " m" + unit :
                String(value, 2) + " " + unit ;
}

Display::Display(): m_lcd(ScreenWidth, ScreenHeight) {
}

void Display::init() {
#if 0
    Wire.begin( 27, 26 );
    if(!m_lcd.begin(SSD1306_SWITCHCAPVCC, 0x3C, false, false)) {
        TRACE_ERROR( "Error in LCD initialization" );
        return;
    }
#else
    m_lcd.init();
#endif
}


void Display::update( const meter::CalculatedMeasures& measures ) {
    m_lcd.clearDisplay();

    mainHeader( measures.signalFrequency(), measures.sampleRate() );

    m_lcd.setFont(&Dialog_plain_13);
    m_lcd.setCursor( 0, 31 );
    float voltage = (measures.signalFrequency() == 0) ? 
                    measures.voltage().mean() :
                    measures.voltage().rms();
    m_lcd.print( String(voltage, 2) + " V" );
 
    m_lcd.setCursor( 0, 47 );
    float current = (measures.signalFrequency() == 0) ? 
                    measures.current().mean() :
                    measures.current().rms();
    m_lcd.print( adjustUnit(current, "A" ) );

    const meter::PowerMeasure& power = measures.power();
    m_lcd.setCursor( 0, 63 );
    m_lcd.print( adjustUnit(power.active(), "W" ) );

    m_lcd.setFont(&Dialog_plain_8);
    right( m_lcd, 34, adjustUnit(power.apparent(), "VA") );
    right( m_lcd, 47, adjustUnit(power.reactive(), "VAR") );
    right( m_lcd, 59, String("PF: ") + String( power.factor(), 2 ) );

    m_lcd.display();
}

void Display::mainHeader( uint32_t signalFrequency, uint32_t sampleRate ) {
    m_lcd.setTextColor( SSD1306::White );
    
    String title;
    if ( signalFrequency==0 )  {
        title = "DC";
    }
    else {
        String str = String(signalFrequency / 100.0, 1);
        if ( str.endsWith(".0") ) {
            str = str.substring( 0, str.length() - 2 );
        }
        title = str + " Hz";
    }
    m_lcd.setFont(&Dialog_plain_17);
    justify( m_lcd, 15, title, [](int16_t width) {
        return Display::ScreenWidth/3 - (width/2); 
    } );

    m_lcd.setFont(&Dialog_plain_8);
    right( m_lcd, 8, String(sampleRate, 10) );

    static bool displayIndicator=false;
    if ( displayIndicator ) {
        m_lcd.drawRect( ScreenWidth-3, 13, 2, 2, SSD1306::White );
    }
    displayIndicator = !displayIndicator;
}


}