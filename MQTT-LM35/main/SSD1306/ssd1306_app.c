#include "ssd1306_app.h"

void ssd1306_ini(void)
{
#if CONFIG_I2C_INTERFACE
	ESP_LOGI(LOG_SSD1306, "INTERFACE is i2c");
	ESP_LOGI(LOG_SSD1306, "CONFIG_SDA_GPIO=%d",CONFIG_SDA_GPIO);
	ESP_LOGI(LOG_SSD1306, "CONFIG_SCL_GPIO=%d",CONFIG_SCL_GPIO);
	ESP_LOGI(LOG_SSD1306, "CONFIG_RESET_GPIO=%d",CONFIG_RESET_GPIO);
	i2c_master_init(&SSD1306, CONFIG_SDA_GPIO, CONFIG_SCL_GPIO, CONFIG_RESET_GPIO);
#elif CONFIG_SPI_INTERFACE
	ESP_LOGI(LOG_SSD1306, "INTERFACE is SPI");
	ESP_LOGI(LOG_SSD1306, "CONFIG_MOSI_GPIO=%d",CONFIG_MOSI_GPIO);
	ESP_LOGI(LOG_SSD1306, "CONFIG_SCLK_GPIO=%d",CONFIG_SCLK_GPIO);
	ESP_LOGI(LOG_SSD1306, "CONFIG_CS_GPIO=%d",CONFIG_CS_GPIO);
	ESP_LOGI(LOG_SSD1306, "CONFIG_DC_GPIO=%d",CONFIG_DC_GPIO);
	ESP_LOGI(LOG_SSD1306, "CONFIG_RESET_GPIO=%d",CONFIG_RESET_GPIO);
	spi_master_init(&SSD1306, CONFIG_MOSI_GPIO, CONFIG_SCLK_GPIO, CONFIG_CS_GPIO, CONFIG_DC_GPIO, CONFIG_RESET_GPIO);
#endif // CONFIG_SPI_INTERFACE

#ifdef CONFIG_FLIP
	SSD1306._flip = true;
	ESP_LOGW(LOG_SSD1306, "Flip upside down");
#endif

#ifdef CONFIG_SSD1306_128x64
	ESP_LOGI(LOG_SSD1306, "Panel is 128x64");
	ssd1306_init(&SSD1306, 128, 64);
#endif // CONFIG_SSD1306_128x64
#ifdef CONFIG_SSD1306_128x32
	ESP_LOGI(LOG_SSD1306, "Panel is 128x32");
	ssd1306_init(&SSD1306, 128, 32);
#endif // CONFIG_SSD1306_128x32

	ssd1306_clear_screen(&SSD1306, false);
	ssd1306_contrast(&SSD1306, 0xff);
}

void ssd1306_demo() {
    int center, top, bottom;
	char lineChar[20];

#ifdef CONFIG_SSD1306_128x64
	top = 2;
	center = 3;
	bottom = 8;
	ssd1306_display_text(&SSD1306, 0, "SSD1306 128x64", 14, false);
	ssd1306_display_text(&SSD1306, 1, "ABCDEFGHIJKLMNOP", 16, false);
	ssd1306_display_text(&SSD1306, 2, "abcdefghijklmnop",16, false);
	ssd1306_display_text(&SSD1306, 3, "Hello World!!", 13, false);
	ssd1306_clear_line(&SSD1306, 4, true);
	ssd1306_clear_line(&SSD1306, 5, true);
	ssd1306_clear_line(&SSD1306, 6, true);
	ssd1306_clear_line(&SSD1306, 7, true);
	ssd1306_display_text(&SSD1306, 4, "SSD1306 128x64", 14, true);
	ssd1306_display_text(&SSD1306, 5, "ABCDEFGHIJKLMNOP", 16, true);
	ssd1306_display_text(&SSD1306, 6, "abcdefghijklmnop",16, true);
	ssd1306_display_text(&SSD1306, 7, "Hello World!!", 13, true);
#endif // CONFIG_SSD1306_128x64

#ifdef CONFIG_SSD1306_128x32
	top = 1;
	center = 1;
	bottom = 4;
	ssd1306_display_text(&SSD1306, 0, "SSD1306 128x32", 14, false);
	ssd1306_display_text(&SSD1306, 1, "Hello World!!", 13, false);
	ssd1306_clear_line(&SSD1306, 2, true);
	ssd1306_clear_line(&SSD1306, 3, true);
	ssd1306_display_text(&SSD1306, 2, "SSD1306 128x32", 14, true);
	ssd1306_display_text(&SSD1306, 3, "Hello World!!", 13, true);
#endif // CONFIG_SSD1306_128x32
	vTaskDelay(3000 / portTICK_PERIOD_MS);
	
	// Display Count Down
	uint8_t image[24];
	memset(image, 0, sizeof(image));
	ssd1306_display_image(&SSD1306, top, (6*8-1), image, sizeof(image));
	ssd1306_display_image(&SSD1306, top+1, (6*8-1), image, sizeof(image));
	ssd1306_display_image(&SSD1306, top+2, (6*8-1), image, sizeof(image));
	for(int font=0x39;font>0x30;font--) {
		memset(image, 0, sizeof(image));
		ssd1306_display_image(&SSD1306, top+1, (7*8-1), image, 8);
		memcpy(image, font8x8_basic_tr[font], 8);
		if (SSD1306._flip) ssd1306_flip(image, 8);
		ssd1306_display_image(&SSD1306, top+1, (7*8-1), image, 8);
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
	
	// Scroll Up
	ssd1306_clear_screen(&SSD1306, false);
	ssd1306_contrast(&SSD1306, 0xff);
	ssd1306_display_text(&SSD1306, 0, "---Scroll  UP---", 16, true);
	//ssd1306_software_scroll(&SSD1306, 7, 1);
	ssd1306_software_scroll(&SSD1306, (SSD1306._pages - 1), 1);
	for (int line=0;line<bottom+10;line++) {
		lineChar[0] = 0x01;
		sprintf(&lineChar[1], " Line %02d", line);
		ssd1306_scroll_text(&SSD1306, lineChar, strlen(lineChar), false);
		vTaskDelay(500 / portTICK_PERIOD_MS);
	}
	vTaskDelay(3000 / portTICK_PERIOD_MS);
	
	// Scroll Down
	ssd1306_clear_screen(&SSD1306, false);
	ssd1306_contrast(&SSD1306, 0xff);
	ssd1306_display_text(&SSD1306, 0, "--Scroll  DOWN--", 16, true);
	//ssd1306_software_scroll(&SSD1306, 1, 7);
	ssd1306_software_scroll(&SSD1306, 1, (SSD1306._pages - 1) );
	for (int line=0;line<bottom+10;line++) {
		lineChar[0] = 0x02;
		sprintf(&lineChar[1], " Line %02d", line);
		ssd1306_scroll_text(&SSD1306, lineChar, strlen(lineChar), false);
		vTaskDelay(500 / portTICK_PERIOD_MS);
	}
	vTaskDelay(3000 / portTICK_PERIOD_MS);

	// Page Down
	ssd1306_clear_screen(&SSD1306, false);
	ssd1306_contrast(&SSD1306, 0xff);
	ssd1306_display_text(&SSD1306, 0, "---Page	DOWN---", 16, true);
	ssd1306_software_scroll(&SSD1306, 1, (SSD1306._pages-1) );
	for (int line=0;line<bottom+10;line++) {
		// if ( (line % 7) == 0) ssd1306_scroll_clear(&SSD1306);
		if ( (line % (SSD1306._pages-1)) == 0) ssd1306_scroll_clear(&SSD1306);
		lineChar[0] = 0x02;
		sprintf(&lineChar[1], " Line %02d", line);
		ssd1306_scroll_text(&SSD1306, lineChar, strlen(lineChar), false);
		vTaskDelay(500 / portTICK_PERIOD_MS);
	}
	vTaskDelay(3000 / portTICK_PERIOD_MS);

	// Horizontal Scroll
	ssd1306_clear_screen(&SSD1306, false);
	ssd1306_contrast(&SSD1306, 0xff);
	ssd1306_display_text(&SSD1306, center, "Horizontal", 10, false);
	ssd1306_hardware_scroll(&SSD1306, SCROLL_RIGHT);
	vTaskDelay(5000 / portTICK_PERIOD_MS);
	ssd1306_hardware_scroll(&SSD1306, SCROLL_LEFT);
	vTaskDelay(5000 / portTICK_PERIOD_MS);
	ssd1306_hardware_scroll(&SSD1306, SCROLL_STOP);
	
	// Vertical Scroll
	ssd1306_clear_screen(&SSD1306, false);
	ssd1306_contrast(&SSD1306, 0xff);
	ssd1306_display_text(&SSD1306, center, "Vertical", 8, false);
	ssd1306_hardware_scroll(&SSD1306, SCROLL_DOWN);
	vTaskDelay(5000 / portTICK_PERIOD_MS);
	ssd1306_hardware_scroll(&SSD1306, SCROLL_UP);
	vTaskDelay(5000 / portTICK_PERIOD_MS);
	ssd1306_hardware_scroll(&SSD1306, SCROLL_STOP);
	
	// Invert
	ssd1306_clear_screen(&SSD1306, true);
	ssd1306_contrast(&SSD1306, 0xff);
	ssd1306_display_text(&SSD1306, center, "  Good Bye!!", 12, true);
	vTaskDelay(5000 / portTICK_PERIOD_MS);
	// Fade Out
	ssd1306_fadeout(&SSD1306);
}