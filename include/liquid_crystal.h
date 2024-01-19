#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C"
{
#endif

// commands
#define LCD_CLEARDISPLAY 0x01
#define LCD_RETURNHOME 0x02
#define LCD_ENTRYMODESET 0x04
#define LCD_DISPLAYCONTROL 0x08
#define LCD_CURSORSHIFT 0x10
#define LCD_FUNCTIONSET 0x20
#define LCD_SETCGRAMADDR 0x40
#define LCD_SETDDRAMADDR 0x80

// flags for display entry mode
#define LCD_ENTRYRIGHT 0x00
#define LCD_ENTRYLEFT 0x02
#define LCD_ENTRYSHIFTINCREMENT 0x01
#define LCD_ENTRYSHIFTDECREMENT 0x00

// flags for display on/off control
#define LCD_DISPLAYON 0x04
#define LCD_DISPLAYOFF 0x00
#define LCD_CURSORON 0x02
#define LCD_CURSOROFF 0x00
#define LCD_BLINKON 0x01
#define LCD_BLINKOFF 0x00

// flags for display/cursor shift
#define LCD_DISPLAYMOVE 0x08
#define LCD_CURSORMOVE 0x00
#define LCD_MOVERIGHT 0x04
#define LCD_MOVELEFT 0x00

// flags for function set
#define LCD_8BITMODE 0x10
#define LCD_4BITMODE 0x00
#define LCD_2LINE 0x08
#define LCD_1LINE 0x00
#define LCD_5x10DOTS 0x04
#define LCD_5x8DOTS 0x00

    typedef enum
    {
        LIQUID_CRYSTAL_FOUR_BITs,
        LIQUID_CRYSTAL_EIGHT_BITS,
    } liquid_crystal_kind_connection_t;

    typedef struct
    {
        uint8_t rs;            // GPIO Connected on RS (DATA or Instrution)
        uint8_t enable;        // GPIO Connected on enable
        uint8_t _data_pins[8]; // GPIO Connected on data pins
    } liquid_crystal_connection_t;

    typedef struct
    {
        liquid_crystal_connection_t liquid_crystal_connection;
        liquid_crystal_kind_connection_t liquid_crystal_kind_connection;
        uint8_t cols;
        uint8_t rows;
        uint8_t charsize;
        uint8_t _displayfunction;
        uint8_t _displaycontrol;
        uint8_t _displaymode;
        uint8_t _row_offsets[4];
    } liquid_crystal_t;

    esp_err_t liquid_crystal_init(liquid_crystal_t *liquid_crystal);

    void liquid_crystal_clear(liquid_crystal_t *liquid_crystal);
    void liquid_crystal_home(liquid_crystal_t *liquid_crystal);
    void liquid_crystal_no_display(liquid_crystal_t *liquid_crystal);
    void liquid_crystal_display(liquid_crystal_t *liquid_crystal);
    void liquid_crystal_no_blink(liquid_crystal_t *liquid_crystal);
    void liquid_crystal_blink(liquid_crystal_t *liquid_crystal);
    void liquid_crystal_no_cursor(liquid_crystal_t *liquid_crystal);
    void liquid_crystal_cursor(liquid_crystal_t *liquid_crystal);
    void liquid_crystal_scroll_display_left(liquid_crystal_t *liquid_crystal);
    void liquid_crystal_scroll_display_right(liquid_crystal_t *liquid_crystal);
    void liquid_crystal_left_to_right(liquid_crystal_t *liquid_crystal);
    void liquid_crystal_right_to_left(liquid_crystal_t *liquid_crystal);
    void liquid_crystal_autoscroll(liquid_crystal_t *liquid_crystal);
    void liquid_crystal_no_auto_scroll(liquid_crystal_t *liquid_crystal);
    void liquid_crystal_print(liquid_crystal_t *liquid_crystal, char *str);

    void liquid_crystal_set_cursor(liquid_crystal_t *liquid_crystal, uint8_t col, uint8_t row);

#ifdef __cplusplus
}
#endif