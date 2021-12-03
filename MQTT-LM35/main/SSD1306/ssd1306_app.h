

/******************************************************************************
 You have to set this config value with menuconfig
 CONFIG_INTERFACE

 para i2c
 CONFIG_MODEL
 CONFIG_SDA_GPIO
 CONFIG_SCL_GPIO
 CONFIG_RESET_GPIO

 para SPI
 CONFIG_CS_GPIO
 CONFIG_DC_GPIO
 CONFIG_RESET_GPIO
******************************************************************************/
#define CONFIG_I2C_INTERFACE 1
#define CONFIG_SDA_GPIO 5
#define CONFIG_SCL_GPIO 4
#define CONFIG_RESET_GPIO -1
#define CONFIG_SSD1306_128x64 1
#define CONFIG_FLIP 0

#include "esp_log.h"
#include "include/ssd1306.c"

const char *LOG_SSD1306 = "SSD1306";
SSD1306_t SSD1306;

void ssd1306_ini(void);
void ssd1306_demo();