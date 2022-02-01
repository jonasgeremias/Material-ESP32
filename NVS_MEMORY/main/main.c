#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_event.h"
#include "freertos/queue.h"
#include <stdio.h>
#include <string.h>

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"
#define DEBUG_NVS

const char* LOG_APP = "APP";

/******************************************************************************
 * Structs
 *****************************************************************************/
typedef struct event_t
{
   uint16_t pos;
   uint8_t type;
   uint8_t data1;
   uint8_t data2;
   uint8_t data3;
   uint8_t data4;
   uint8_t data5;
   uint8_t data6;
   uint8_t data7;
   uint8_t data8;
   uint16_t data9;
   uint16_t data10;
   uint16_t data11;
   uint16_t data12;
   uint16_t data13;
   uint16_t data14;
   uint16_t data15;
   uint16_t data16;
   uint16_t data17;
   uint16_t data18;
   float data19;
   float data20;
   float data21;
   float data22;
   float data23;
   float data24;
} event_t;

static TaskHandle_t taskhandle_app_controller;

/******************************************************************************
 * Modules
 ******************************************************************************/
#include "modules/_NVS/NVS_V1.0.c"

static esp_err_t update_event(event_t *evento)
{
   // Insert data into structure to save to flash memory
   evento->data1 = 10;
   evento->data2 = 10;
   evento->data18 = 10;
   evento->data22 = 10.123456;
   return ESP_OK;
}

static void app_controller(void *pvParameters)
{
   ESP_LOGI(LOG_APP, "Init app_controller");

   event_t ev_temp;
   int count = 0;

   while (1)
   {
      ESP_LOGI(LOG_APP, "Loop -> %d", count);
      
      // Save 10 Events now
      for (int i = 0; i < 10; i++)
      {
         update_event(&ev_temp);
         ev_temp.type = 100;
         nvs_app_write_event(&ev_temp);
         count++;
      }
      
      if (count > 200) { // Restart after 200 events
         ESP_LOGE(LOG_APP, "Restart now");
         esp_restart(); 
      }
                             
      vTaskDelay(100 / portTICK_PERIOD_MS); // await 100 ms
   }
   vTaskDelete(NULL);
}

void app_main()
{
   esp_log_level_set("*", ESP_LOG_VERBOSE);
   app_nvs_init();
   nvs_app_init_config_events();

   xTaskCreatePinnedToCore(app_controller, "app_controller", 22000, NULL, 1, &taskhandle_app_controller, 1);
}