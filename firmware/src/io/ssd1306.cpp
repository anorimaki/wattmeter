#include "io/ssd1306.h"
#include "util/trace.h"
#include "driver/i2c.h"

#define ssd1306_swap(a, b) \
    (((a) ^= (b)), ((b) ^= (a)), ((a) ^= (b))) ///< No-temp-var swap operation


#define SSD1306_MEMORYMODE          0x20 ///< See datasheet
#define SSD1306_COLUMNADDR          0x21 ///< See datasheet
#define SSD1306_PAGEADDR            0x22 ///< See datasheet
#define SSD1306_SETCONTRAST         0x81 ///< See datasheet
#define SSD1306_CHARGEPUMP          0x8D ///< See datasheet
#define SSD1306_SEGREMAP            0xA0 ///< See datasheet
#define SSD1306_DISPLAYALLON_RESUME 0xA4 ///< See datasheet
#define SSD1306_DISPLAYALLON        0xA5 ///< Not currently used
#define SSD1306_NORMALDISPLAY       0xA6 ///< See datasheet
#define SSD1306_INVERTDISPLAY       0xA7 ///< See datasheet
#define SSD1306_SETMULTIPLEX        0xA8 ///< See datasheet
#define SSD1306_DISPLAYOFF          0xAE ///< See datasheet
#define SSD1306_DISPLAYON           0xAF ///< See datasheet
#define SSD1306_COMSCANINC          0xC0 ///< Not currently used
#define SSD1306_COMSCANDEC          0xC8 ///< See datasheet
#define SSD1306_SETDISPLAYOFFSET    0xD3 ///< See datasheet
#define SSD1306_SETDISPLAYCLOCKDIV  0xD5 ///< See datasheet
#define SSD1306_SETPRECHARGE        0xD9 ///< See datasheet
#define SSD1306_SETCOMPINS          0xDA ///< See datasheet
#define SSD1306_SETVCOMDETECT       0xDB ///< See datasheet

#define SSD1306_SETLOWCOLUMN        0x00 ///< Not currently used
#define SSD1306_SETHIGHCOLUMN       0x10 ///< Not currently used
#define SSD1306_SETSTARTLINE        0x40 ///< See datasheet

#define SSD1306_EXTERNALVCC         0x01 ///< External display voltage source
#define SSD1306_SWITCHCAPVCC        0x02 ///< Gen. display voltage from 3.3V

#define SSD1306_RIGHT_HORIZONTAL_SCROLL              0x26 ///< Init rt scroll
#define SSD1306_LEFT_HORIZONTAL_SCROLL               0x27 ///< Init left scroll
#define SSD1306_VERTICAL_AND_RIGHT_HORIZONTAL_SCROLL 0x29 ///< Init diag scroll
#define SSD1306_VERTICAL_AND_LEFT_HORIZONTAL_SCROLL  0x2A ///< Init diag scroll
#define SSD1306_DEACTIVATE_SCROLL                    0x2E ///< Stop scroll
#define SSD1306_ACTIVATE_SCROLL                      0x2F ///< Start scroll
#define SSD1306_SET_VERTICAL_SCROLL_AREA             0xA3 ///< Set scroll range


static esp_err_t i2c_master_init() {
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = GPIO_NUM_4;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_io_num = GPIO_NUM_15;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = 400000;             //Max 600000 Hz
    i2c_param_config(I2C_NUM_0, &conf);
    return i2c_driver_install(I2C_NUM_0, conf.mode, 0, 0, 0);
}

static esp_err_t send(const uint8_t *data, size_t size) {
    esp_err_t ret;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, 0x3C << 1 | 0, 1);
    i2c_master_write_byte(cmd, 0, 1);       // Co = 0, D/C = 0
    i2c_master_write(cmd, const_cast<uint8_t *>(data), size, 1);
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}


SSD1306::SSD1306(uint8_t w, uint8_t h): Adafruit_GFX(w, h), 
        m_buffer(NULL), 
        m_size(w * ((h + 7) / 8)) {
}

SSD1306::~SSD1306() {
    i2c_driver_delete(I2C_NUM_0);
    if ( m_buffer ) {
        free( m_buffer );
    }
}

void SSD1306::init() {
    m_buffer = (uint8_t *)malloc(m_size);
    clearDisplay();

    i2c_master_init();

// Init sequence
    static const uint8_t init1[] = {
        SSD1306_DISPLAYOFF,         // 0xAE
        SSD1306_SETDISPLAYCLOCKDIV, // 0xD5
        0x80,                       // the suggested ratio 0x80
        SSD1306_SETMULTIPLEX,       // 0xA8
        static_cast<uint8_t>(HEIGHT - 1)};       
    TRACE_ESP_ERROR_CHECK(send(init1, sizeof(init1)));

    static const uint8_t init2[] = {
        SSD1306_SETDISPLAYOFFSET,   // 0xD3
        0x0,                        // no offset
        SSD1306_SETSTARTLINE | 0x0, // line #0
        SSD1306_CHARGEPUMP,         // 0x8D
        0x14};
    TRACE_ESP_ERROR_CHECK(send(init2, sizeof(init2)));

    static const uint8_t init3[] = {
        SSD1306_MEMORYMODE, // 0x20
        0x00,               // 0x0 act like ks0108
        SSD1306_SEGREMAP | 0x1,
        SSD1306_COMSCANDEC};
    TRACE_ESP_ERROR_CHECK(send(init3, sizeof(init3)));

    static const uint8_t init4b[] = {
        SSD1306_SETCOMPINS, // 0xDA
        0x12,
        SSD1306_SETCONTRAST, // 0x81
        0xCF};
    TRACE_ESP_ERROR_CHECK(send(init4b, sizeof(init4b)));

    static const uint8_t init4c[] = {
        SSD1306_SETPRECHARGE, // 0xd9
        0xF1};
    TRACE_ESP_ERROR_CHECK(send(init4c, sizeof(init4c)));

    static const uint8_t init5[] = {
        SSD1306_SETVCOMDETECT, // 0xDB
        0x40,
        SSD1306_DISPLAYALLON_RESUME, // 0xA4
        SSD1306_NORMALDISPLAY,       // 0xA6
        SSD1306_DEACTIVATE_SCROLL,
        SSD1306_DISPLAYON}; // Main screen turn on
    TRACE_ESP_ERROR_CHECK(send(init5, sizeof(init5)));

}

void SSD1306::clearDisplay() {
    memset(m_buffer, 0, m_size);
}

void SSD1306::display() {
     static uint8_t dlist1[] = {
        SSD1306_PAGEADDR,
        0,                         // Page start address
        0xFF,                      // Page end (not really, but works here)
        SSD1306_COLUMNADDR,
        0,
        static_cast<uint8_t>(WIDTH-1) };                       // Column start address
    TRACE_ESP_ERROR_CHECK(send(dlist1, sizeof(dlist1)));
    
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, 0x3C << 1 | 0, 1);
    i2c_master_write_byte(cmd, 0x40, 1);       // Co = 0, D/C = 0
    i2c_master_write(cmd, m_buffer, m_size, 1);
    i2c_master_stop(cmd);
    TRACE_ESP_ERROR_CHECK(i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_RATE_MS));
    i2c_cmd_link_delete(cmd);
}

void SSD1306::drawPixel(int16_t x, int16_t y, uint16_t color) {
    if ((x >= 0) && (x < width()) && (y >= 0) && (y < height()))  {
        // Pixel is in-bounds. Rotate coordinates if needed.
        switch (getRotation()) {
        case 1:
            ssd1306_swap(x, y);
            x = WIDTH - x - 1;
            break;
        case 2:
            x = WIDTH - x - 1;
            y = HEIGHT - y - 1;
            break;
        case 3:
            ssd1306_swap(x, y);
            y = HEIGHT - y - 1;
            break;
        }
        switch (color) {
        case White:
            m_buffer[x + (y / 8) * WIDTH] |= (1 << (y & 7));
            break;
        case Black:
            m_buffer[x + (y / 8) * WIDTH] &= ~(1 << (y & 7));
            break;
        case Inverse:
            m_buffer[x + (y / 8) * WIDTH] ^= (1 << (y & 7));
            break;
        }
    }
}
