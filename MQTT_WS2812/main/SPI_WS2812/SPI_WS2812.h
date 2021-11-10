#ifndef __SPI_WS2812_H__
#define __SPI_WS2812_H__
#include <stdio.h>
#include <string.h>
#include "driver/spi_master.h"
#include "driver/gpio.h"

#define RED (uint32_t)0xFF0000
#define GREEN (uint32_t)0x00FF00
#define BLUE (uint32_t)0x0000FF
#define WHITE (uint32_t)0xFFFFFF
#define BLACK (uint32_t)0x000000
#define YELLOW (uint32_t)0xFFFF00
#define CYAN (uint32_t)0x00FFFF
#define MAGENTA (uint32_t)0xFF00FF
#define PURPLE (uint32_t)0x400080
#define ORANGE (uint32_t)0xFF3000
#define PINK (uint32_t)0xFF1493
#define PERCENT(c, p) (c * p / 100)



typedef struct CRGB
{
    union
    {
        uint8_t r;
        uint8_t red;
    };
    union
    {
        uint8_t g;
        uint8_t green;
    };
    union
    {
        uint8_t b;
        uint8_t blue;
    };  
} CRGB;

void initSPIws2812();
void fillColor(uint32_t col);
void fillBuffer(uint32_t *bufLed, int Count);
void led_strip_update();

#endif
