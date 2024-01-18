#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_check.h"
#include "esp_log.h"

#include "driver/gpio.h"

#include "liquid_crystal.h"

static const char *TAG = "Liquid Crystal";

/************ low level data pushing commands **********/

// write either command or data, with automatic 4/8-bit selection
void send(liquid_crystal_t *liquid_crystal, uint8_t value, uint8_t mode)
{
    digitalWrite(liquid_crystal->liquid_crystal_connection.rs, mode);

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

void pulseEnable(liquid_crystal_t *liquid_crystal)
{
    gpio_set_level(liquid_crystal->liquid_crystal_connection.enable, 0);
    vTaskDelay(pdMS_TO_TICKS(1))
        gpio_set_level(liquid_crystal->liquid_crystal_connection.enable, 1);
    vTaskDelay(pdMS_TO_TICKS(1)) // enable pulse must be >450 ns
        gpio_set_level(liquid_crystal->liquid_crystal_connection.enable, 0);
    vTaskDelay(pdMS_TO_TICKS(1)) // commands need >37 us to settle
}

void write4bits(liquid_crystal_t *liquid_crystal, uint8_t value)
{
    for (int i = 0; i < 4; i++)
    {
        gpio_set_level(liquid_crystal->liquid_crystal_connection._data_pins[i], (value >> i) & 0x01);
    }

    pulseEnable(liquid_crystal);
}

write8bits(liquid_crystal_t *liquid_crystal, uint8_t value)
{
    for (int i = 0; i < 8; i++)
    {
        gpio_set_level(liquid_crystal->liquid_crystal_connection._data_pins[i], (value >> i) & 0x01);
    }

    pulseEnable(liquid_crystal);
}

/*********** mid level commands, for sending data/cmds */
void command(liquid_crystal_t *liquid_crystal, uint8_t value)
{
    send(liquid_crystal, value, LOW);
}

void write(liquid_crystal_t *liquid_crystal, uint8_t value)
{
    send(liquid_crystal, value, HIGH);
    return 1; // assume success
}

/********** high level commands, for the user! */
print(liquid_crystal_t *liquid_crystal, const char str[])
{
  return write(liquid_crystal, str);
}

void clear(liquid_crystal_t *liquid_crystal)
{
    command(liquid_crystal, LCD_CLEARDISPLAY); // clear display, set cursor position to zero
    vTaskDelay(pdMS_TO_TICKS(2000);                   // this command takes a long time!
}

void home(liquid_crystal_t *liquid_crystal)
{
    command(liquid_crystal, LCD_RETURNHOME); // set cursor position to zero
    vTaskDelay(pdMS_TO_TICKS(2000);                 // this command takes a long time!
}

void setCursor(liquid_crystal_t *liquid_crystal, uint8_t col, uint8_t row)
{
    const size_t max_lines = sizeof((liquid_crystal->_row_offsets)) / sizeof(*liquid_crystal->_row_offsets);
    if (row >= max_lines)
    {
        row = max_lines - 1; // we count rows starting w/ 0
    }
    if (row >= _numlines)
    {
        row = _numlines - 1; // we count rows starting w/ 0
    }

    command(liquid_crystal, LCD_SETDDRAMADDR | (col + liquid_crystal->_row_offsets[row]));
}

// Turn the display on/off (quickly)
void noDisplay(liquid_crystal_t *liquid_crystal)
{
    liquid_crystal->_displaycontrol &= ~LCD_DISPLAYON;
    command(liquid_crystal, LCD_DISPLAYCONTROL | liquid_crystal->_displaycontrol);
}
void display(liquid_crystal_t *liquid_crystal)
{
    liquid_crystal->_displaycontrol |= LCD_DISPLAYON;
    command(liquid_crystal, LCD_DISPLAYCONTROL | liquid_crystal->_displaycontrol);
}

// Turns the underline cursor on/off
void noCursor(liquid_crystal_t *liquid_crystal)
{
    liquid_crystal->_displaycontrol &= ~LCD_CURSORON;
    command(liquid_crystal, LCD_DISPLAYCONTROL | liquid_crystal->_displaycontrol);
}
void cursor(liquid_crystal_t *liquid_crystal)
{
    liquid_crystal->_displaycontrol |= LCD_CURSORON;
    command(liquid_crystal, LCD_DISPLAYCONTROL | liquid_crystal->_displaycontrol);
}

// Turn on and off the blinking cursor
void noBlink(liquid_crystal_t *liquid_crystal)
{
    liquid_crystal->_displaycontrol &= ~LCD_BLINKON;
    command(liquid_crystal, LCD_DISPLAYCONTROL | liquid_crystal->_displaycontrol);
}
void blink(liquid_crystal_t *liquid_crystal)
{
    liquid_crystal->_displaycontrol |= LCD_BLINKON;
    command(liquid_crystal, LCD_DISPLAYCONTROL | liquid_crystal->_displaycontrol);
}

// These commands scroll the display without changing the RAM
void scrollDisplayLeft(liquid_crystal_t *liquid_crystal)
{
    command(liquid_crystal, LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVELEFT);
}
void scrollDisplayRight(liquid_crystal_t *liquid_crystal)
{
    command(liquid_crystal, LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVERIGHT);
}

// This is for text that flows Left to Right
void leftToRight(liquid_crystal_t *liquid_crystal)
{
    liquid_crystal->_displaymode |= LCD_ENTRYLEFT;
    command(liquid_crystal, LCD_ENTRYMODESET | liquid_crystal->_displaymode);
}

// This is for text that flows Right to Left
void rightToLeft(liquid_crystal_t *liquid_crystal)
{
    liquid_crystal->_displaymode &= ~LCD_ENTRYLEFT;
    command(liquid_crystal, LCD_ENTRYMODESET | liquid_crystal->_displaymode);
}

// This will 'right justify' text from the cursor
void autoscroll(liquid_crystal_t *liquid_crystal)
{
    liquid_crystal->_displaymode |= LCD_ENTRYSHIFTINCREMENT;
    command(liquid_crystal, LCD_ENTRYMODESET | liquid_crystal->_displaymode);
}

// This will 'left justify' text from the cursor
void noAutoscroll(liquid_crystal_t *liquid_crystal)
{
    liquid_crystal->_displaymode &= ~LCD_ENTRYSHIFTINCREMENT;
    command(liquid_crystal, LCD_ENTRYMODESET | liquid_crystal->_displaymode);
}

// Allows us to fill the first 8 CGRAM locations
// with custom characters
void createChar(liquid_crystal_t *liquid_crystal, uint8_t location, uint8_t charmap[])
{
    location &= 0x7; // we only have 8 locations 0-7
    command(liquid_crystal, LCD_SETCGRAMADDR | (location << 3));
    for (int i = 0; i < 8; i++)
    {
        write(liquid_crystal, charmap[i]);
    }
}

/************ INIT liquid_crystal **********/

void setRowOffsets(liquid_crystal_t *liquid_crystal, int row0, int row1, int row2, int row3)
{
    liquid_crystal->_row_offsets[0] = row0;
    liquid_crystal->_row_offsets[1] = row1;
    liquid_crystal->_row_offsets[2] = row2;
    liquid_crystal->_row_offsets[3] = row3;
}

esp_err init(liquid_crystal_t *liquid_crystal)
{
    /* Check the input pointer */
    ESP_GOTO_ON_FALSE(liquid_crystal, ESP_ERR_INVALID_ARG, err, TAG, "invalid argument");

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

    setRowOffsets(0x00, 0x40, 0x00 + liquid_crystal->cols, 0x40 + liquid_crystal->cols);

    // for some 1 line displays you can select a 10 pixel high font
    if ((liquid_crystal->charsize != LCD_5x8DOTS) && (liquid_crystal->rows == 1))
    {
        _displayfunction |= LCD_5x10DOTS;
    }

    esp_rom_gpio_pad_select_gpio(liquid_crystal->liquid_crystal_connection.rs);
    gpio_set_direction(liquid_crystal->liquid_crystal_connection.rs, GPIO_MODE_OUTPUT);

    esp_rom_gpio_pad_select_gpio(liquid_crystal->liquid_crystal_connection.enable);
    gpio_set_direction(liquid_crystal->liquid_crystal_connection.enable, GPIO_MODE_OUTPUT);

    for (int i = 0; i < ((_displayfunction & LCD_8BITMODE) ? 8 : 4); i++)
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
    if (!(_displayfunction & LCD_8BITMODE))
    {
        // this is according to the Hitachi HD44780 datasheet
        // figure 24, pg 46

        // we start in 8bit mode, try to set 4 bit mode
        write4bits(0x03);
        vTaskDelay(pdMS_TO_TICKS(5)); // wait min 5ms

        // second try
        write4bits(0x03);
        vTaskDelay(pdMS_TO_TICKS(5)); // wait min 5ms

        // third go!
        write4bits(0x03);
        vTaskDelay(pdMS_TO_TICKS(2));

        // finally, set to 4-bit interface
        write4bits(0x02);
    }
    else
    {
        // this is according to the Hitachi HD44780 datasheet
        // page 45 figure 23

        // Send function set command sequence
        command(LCD_FUNCTIONSET | _displayfunction);
        vTaskDelay(pdMS_TO_TICKS(5)); // wait more than 4.1 ms

        // second try
        command(LCD_FUNCTIONSET | _displayfunction);
        vTaskDelay(pdMS_TO_TICKS(2));

        // third go
        command(LCD_FUNCTIONSET | _displayfunction);
    }

    // finally, set # lines, font size, etc.
    command(LCD_FUNCTIONSET | _displayfunction);

    // turn the display on with no cursor or blinking default
    _displaycontrol = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;
    display();

    // clear it off
    clear();

    // Initialize to default text direction (for romance languages)
    _displaymode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;
    // set the entry mode
    command(LCD_ENTRYMODESET | _displaymode);

    return ESP_OK
}