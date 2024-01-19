#include <stdio.h>
#include <string.h>

#include "liquid_crystal.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_check.h"
#include "esp_log.h"

#include "driver/gpio.h"

static const char *TAG = "Liquid Crystal";

/************ low level data pushing commands **********/

void pulseEnable(liquid_crystal_t *liquid_crystal)
{
    gpio_set_level(liquid_crystal->liquid_crystal_connection.enable, 0);
    vTaskDelay(pdMS_TO_TICKS(1));
    gpio_set_level(liquid_crystal->liquid_crystal_connection.enable, 1);
    vTaskDelay(pdMS_TO_TICKS(1)); // enable pulse must be >450 ns
    gpio_set_level(liquid_crystal->liquid_crystal_connection.enable, 0);
    vTaskDelay(pdMS_TO_TICKS(1)); // commands need >37 us to settle
}

void write4bits(liquid_crystal_t *liquid_crystal, uint8_t value)
{
    for (int i = 0; i < 4; i++)
    {
        gpio_set_level(liquid_crystal->liquid_crystal_connection._data_pins[i], (value >> i) & 0x01);
    }

    pulseEnable(liquid_crystal);
}

void write8bits(liquid_crystal_t *liquid_crystal, uint8_t value)
{
    for (int i = 0; i < 8; i++)
    {
        gpio_set_level(liquid_crystal->liquid_crystal_connection._data_pins[i], (value >> i) & 0x01);
    }

    pulseEnable(liquid_crystal);
}

// write either liquid_crystal_command or data, with automatic 4/8-bit selection
void send(liquid_crystal_t *liquid_crystal, uint8_t value, uint8_t mode)
{
    gpio_set_level(liquid_crystal->liquid_crystal_connection.rs, mode);

    if (liquid_crystal->liquid_crystal_kind_connection & LIQUID_CRYSTAL_EIGHT_BITS)
    {
        write8bits(liquid_crystal, value);
    }
    else
    {
        write4bits(liquid_crystal, value >> 4);
        write4bits(liquid_crystal, value);
    }
}

/*********** mid level commands, for sending data/cmds */
void liquid_crystal_command(liquid_crystal_t *liquid_crystal, uint8_t value)
{
    send(liquid_crystal, value, 0);
}

void liquid_crystal_write(liquid_crystal_t *liquid_crystal, uint8_t value)
{
    send(liquid_crystal, value, 1);
}

/********** high level commands, for the user! */
void liquid_crystal_print(liquid_crystal_t *liquid_crystal, char *str)
{
    for (int i = 0; i < strlen(str); i++)
    {
        liquid_crystal_write(liquid_crystal, str[i]);
    }
}

void liquid_crystal_clear(liquid_crystal_t *liquid_crystal)
{
    liquid_crystal_command(liquid_crystal, LCD_CLEARDISPLAY); // clear display, set cursor position to zero
    vTaskDelay(pdMS_TO_TICKS(2000));           // this liquid_crystal_command takes a long time!
}

void liquid_crystal_home(liquid_crystal_t *liquid_crystal)
{
    liquid_crystal_command(liquid_crystal, LCD_RETURNHOME); // set cursor position to zero
    vTaskDelay(pdMS_TO_TICKS(2000));         // this liquid_crystal_command takes a long time!
}

void liquid_crystal_set_cursor(liquid_crystal_t *liquid_crystal, uint8_t col, uint8_t row)
{
    const size_t max_lines = sizeof((liquid_crystal->_row_offsets)) / sizeof(*liquid_crystal->_row_offsets);
    if (row >= max_lines)
    {
        row = max_lines - 1; // we count rows starting w/ 0
    }
    if (row >= liquid_crystal->rows)
    {
        row = liquid_crystal->rows - 1; // we count rows starting w/ 0
    }

    liquid_crystal_command(liquid_crystal, LCD_SETDDRAMADDR | (col + liquid_crystal->_row_offsets[row]));
}

// Turn the display on/off (quickly)
void liquid_crystal_no_display(liquid_crystal_t *liquid_crystal)
{
    liquid_crystal->_displaycontrol &= ~LCD_DISPLAYON;
    liquid_crystal_command(liquid_crystal, LCD_DISPLAYCONTROL | liquid_crystal->_displaycontrol);
}
void liquid_crystal_display(liquid_crystal_t *liquid_crystal)
{
    liquid_crystal->_displaycontrol |= LCD_DISPLAYON;
    liquid_crystal_command(liquid_crystal, LCD_DISPLAYCONTROL | liquid_crystal->_displaycontrol);
}

// Turns the underline cursor on/off
void liquid_crystal_no_cursor(liquid_crystal_t *liquid_crystal)
{
    liquid_crystal->_displaycontrol &= ~LCD_CURSORON;
    liquid_crystal_command(liquid_crystal, LCD_DISPLAYCONTROL | liquid_crystal->_displaycontrol);
}
void liquid_crystal_cursor(liquid_crystal_t *liquid_crystal)
{
    liquid_crystal->_displaycontrol |= LCD_CURSORON;
    liquid_crystal_command(liquid_crystal, LCD_DISPLAYCONTROL | liquid_crystal->_displaycontrol);
}

// Turn on and off the blinking cursor
void liquid_crystal_no_blink(liquid_crystal_t *liquid_crystal)
{
    liquid_crystal->_displaycontrol &= ~LCD_BLINKON;
    liquid_crystal_command(liquid_crystal, LCD_DISPLAYCONTROL | liquid_crystal->_displaycontrol);
}
void liquid_crystal_blink(liquid_crystal_t *liquid_crystal)
{
    liquid_crystal->_displaycontrol |= LCD_BLINKON;
    liquid_crystal_command(liquid_crystal, LCD_DISPLAYCONTROL | liquid_crystal->_displaycontrol);
}

// These commands scroll the display without changing the RAM
void liquid_crystal_scroll_display_left(liquid_crystal_t *liquid_crystal)
{
    liquid_crystal_command(liquid_crystal, LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVELEFT);
}
void liquid_crystal_scrollDisplayRight(liquid_crystal_t *liquid_crystal)
{
    liquid_crystal_command(liquid_crystal, LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVERIGHT);
}

// This is for text that flows Left to Right
void liquid_crystal_left_to_right(liquid_crystal_t *liquid_crystal)
{
    liquid_crystal->_displaymode |= LCD_ENTRYLEFT;
    liquid_crystal_command(liquid_crystal, LCD_ENTRYMODESET | liquid_crystal->_displaymode);
}

// This is for text that flows Right to Left
void liquid_crystal_right_to_left(liquid_crystal_t *liquid_crystal)
{
    liquid_crystal->_displaymode &= ~LCD_ENTRYLEFT;
    liquid_crystal_command(liquid_crystal, LCD_ENTRYMODESET | liquid_crystal->_displaymode);
}

// This will 'right justify' text from the cursor
void liquid_crystal_autoscroll(liquid_crystal_t *liquid_crystal)
{
    liquid_crystal->_displaymode |= LCD_ENTRYSHIFTINCREMENT;
    liquid_crystal_command(liquid_crystal, LCD_ENTRYMODESET | liquid_crystal->_displaymode);
}

// This will 'left justify' text from the cursor
void liquid_crystal_no_autoscroll(liquid_crystal_t *liquid_crystal)
{
    liquid_crystal->_displaymode &= ~LCD_ENTRYSHIFTINCREMENT;
    liquid_crystal_command(liquid_crystal, LCD_ENTRYMODESET | liquid_crystal->_displaymode);
}

// Allows us to fill the first 8 CGRAM locations
// with custom characters
void liquid_crystal_create_char(liquid_crystal_t *liquid_crystal, uint8_t location, uint8_t charmap[])
{
    location &= 0x7; // we only have 8 locations 0-7
    liquid_crystal_command(liquid_crystal, LCD_SETCGRAMADDR | (location << 3));
    for (int i = 0; i < 8; i++)
    {
        liquid_crystal_write(liquid_crystal, charmap[i]);
    }
}

/************ INIT liquid_crystal **********/

void set_row_offsets(liquid_crystal_t *liquid_crystal, int row0, int row1, int row2, int row3)
{
    liquid_crystal->_row_offsets[0] = row0;
    liquid_crystal->_row_offsets[1] = row1;
    liquid_crystal->_row_offsets[2] = row2;
    liquid_crystal->_row_offsets[3] = row3;
}

esp_err_t liquid_crystal_init(liquid_crystal_t *liquid_crystal)
{
    /* Check the input pointer */
    ESP_RETURN_ON_FALSE(liquid_crystal, ESP_ERR_INVALID_ARG, TAG, "invalid argument");

    if (liquid_crystal->liquid_crystal_kind_connection == LIQUID_CRYSTAL_FOUR_BITs)
    {
        liquid_crystal->_displayfunction = LCD_4BITMODE | LCD_5x8DOTS;
    }
    else
    {
        liquid_crystal->_displayfunction = LCD_8BITMODE | LCD_5x8DOTS;
    }

    if (liquid_crystal->rows > 1)
    {
        liquid_crystal->_displayfunction |= LCD_2LINE;
    }

    set_row_offsets(liquid_crystal, 0x00, 0x40, 0x00 + liquid_crystal->cols, 0x40 + liquid_crystal->cols);

    // for some 1 line displays you can select a 10 pixel high font
    if ((liquid_crystal->charsize != LCD_5x8DOTS) && (liquid_crystal->rows == 1))
    {
        liquid_crystal->_displayfunction |= LCD_5x10DOTS;
    }

    esp_rom_gpio_pad_select_gpio(liquid_crystal->liquid_crystal_connection.rs);
    gpio_set_direction(liquid_crystal->liquid_crystal_connection.rs, GPIO_MODE_OUTPUT);

    esp_rom_gpio_pad_select_gpio(liquid_crystal->liquid_crystal_connection.enable);
    gpio_set_direction(liquid_crystal->liquid_crystal_connection.enable, GPIO_MODE_OUTPUT);

    for (int i = 0; i < ((liquid_crystal->_displayfunction & LCD_8BITMODE) ? 8 : 4); i++)
    {
        esp_rom_gpio_pad_select_gpio(liquid_crystal->liquid_crystal_connection._data_pins[i]);
        gpio_set_direction(liquid_crystal->liquid_crystal_connection._data_pins[i], GPIO_MODE_OUTPUT);
    }

    // SEE PAGE 45/46 FOR INITIALIZATION SPECIFICATION!
    // according to datasheet, we need at least 40 ms after power rises above 2.7 V
    vTaskDelay(pdMS_TO_TICKS(40));

    // Now we pull both RS and enable low to begin commands
    gpio_set_level(liquid_crystal->liquid_crystal_connection.rs, 0);
    gpio_set_level(liquid_crystal->liquid_crystal_connection.enable, 0);

    // put the LCD into 4 bit or 8 bit mode
    if (!(liquid_crystal->_displayfunction & LCD_8BITMODE))
    {
        // this is according to the Hitachi HD44780 datasheet
        // figure 24, pg 46

        // we start in 8bit mode, try to set 4 bit mode
        write4bits(liquid_crystal, 0x03);
        vTaskDelay(pdMS_TO_TICKS(5)); // wait min 5ms

        // second try
        write4bits(liquid_crystal, 0x03);
        vTaskDelay(pdMS_TO_TICKS(5)); // wait min 5ms

        // third go!
        write4bits(liquid_crystal, 0x03);
        vTaskDelay(pdMS_TO_TICKS(2));

        // finally, set to 4-bit interface
        write4bits(liquid_crystal, 0x02);
    }
    else
    {
        // this is according to the Hitachi HD44780 datasheet
        // page 45 figure 23

        // Send function set liquid_crystal_command sequence
        liquid_crystal_command(liquid_crystal, LCD_FUNCTIONSET | liquid_crystal->_displayfunction);
        vTaskDelay(pdMS_TO_TICKS(5)); // wait more than 4.1 ms

        // second try
        liquid_crystal_command(liquid_crystal, LCD_FUNCTIONSET | liquid_crystal->_displayfunction);
        vTaskDelay(pdMS_TO_TICKS(2));

        // third go
        liquid_crystal_command(liquid_crystal, LCD_FUNCTIONSET | liquid_crystal->_displayfunction);
    }

    // finally, set # lines, font size, etc.
    liquid_crystal_command(liquid_crystal, LCD_FUNCTIONSET | liquid_crystal->_displayfunction);

    // turn the display on with no cursor or blinking default
    liquid_crystal->_displaycontrol = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;
    liquid_crystal_display(liquid_crystal);

    // clear it off
    liquid_crystal_clear(liquid_crystal);

    // Initialize to default text direction (for romance languages)
    liquid_crystal->_displaymode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;
    // set the entry mode
    liquid_crystal_command(liquid_crystal, LCD_ENTRYMODESET | liquid_crystal->_displaymode);

    return ESP_OK;
}