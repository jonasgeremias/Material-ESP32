#ifndef __NVS_APP_C__
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include "NVS_V1.0.h"

// Read -----------------------------------------------------------------------
esp_err_t app_nvs_get_blob(nvs_handle_t handle, const char *key, void *out_value, size_t *length)
{
   esp_err_t err = ESP_OK;
   char key_ant[16] = {0};
   sprintf(key_ant, "%s$$", key); // Memoria de backup.
#ifdef DEBUG_APP_NVS
   ESP_LOGE(LOG_NVS, "app_nvs_get_blob: %s - %s", key, key_ant);
#endif

   err = nvs_get_blob(handle, key, out_value, length);
   if (err != ESP_OK)
   {
      err = nvs_get_blob(handle, key_ant, out_value, length);
      if (err != ESP_OK)
      {
         return err;
      }
      nvs_set_blob(handle, key, out_value, *length);
      nvs_commit(handle);
   }
   return err;
}

esp_err_t app_nvs_get_i8(nvs_handle_t handle, const char *key, int8_t *out_value)
{
   esp_err_t err = ESP_OK;
   char key_ant[16] = {0};
   sprintf(key_ant, "%s$$", key); // Memoria de backup.
#ifdef DEBUG_APP_NVS
   ESP_LOGE(LOG_NVS, "app_nvs_get_i8: %s - %s", key, key_ant);
#endif

   err = nvs_get_i8(handle, key, out_value);
   if (err != ESP_OK)
   {
      err = nvs_get_i8(handle, key_ant, out_value);
      if (err != ESP_OK)
      {
         return err;
      }
      nvs_set_i8(handle, key, *out_value);
      nvs_commit(handle);
   }
   return err;
}

// Write ----------------------------------------------------------------------

esp_err_t app_nvs_set_blob(nvs_handle_t handle, const char *key, const void *value, size_t length)
{
   esp_err_t err = ESP_OK;
   char key_ant[16] = {0};
   sprintf(key_ant, "%s$$", key);

#ifdef DEBUG_APP_NVS
   ESP_LOGE(LOG_NVS, "app_nvs_set_blob: %s - %s", key, key_ant);
#endif

   err = nvs_set_blob(handle, key, value, length);
   nvs_commit(handle);
   nvs_set_blob(handle, key_ant, value, length);

   return err;
}

esp_err_t app_nvs_set_i8(nvs_handle_t handle, const char *key, int8_t value)
{
   esp_err_t err = ESP_OK;
   char key_ant[16] = {0};
   sprintf(key_ant, "%s$$", key);

#ifdef DEBUG_APP_NVS
   ESP_LOGE(LOG_NVS, "app_nvs_set_i8: %s - %s", key, key_ant);
#endif

   nvs_set_i8(handle, key, value);
   nvs_commit(handle);
   nvs_set_i8(handle, key_ant, value);

   return err;
}

//  Procura apenas as chaves de eventos, o iterador retorna o ultimo alterado, logo é o ultimo indice salvo.
static esp_err_t nvs_app_init_config_events()
{
#ifdef DEBUG_NVS
   ESP_LOGI(LOG_NVS, "ini");
#endif
   xSemaphoreTake(mutex_nvs, portMAX_DELAY);
   nvs_iterator_t it = nvs_entry_find(report, report, NVS_TYPE_BLOB);
   uint16_t qty_ev = 0;
   nvs_entry_info_t info;
   char buffer[16];
   bzero(buffer, 15);

   while (it != NULL)
   {
      nvs_entry_info(it, &info);

      it = nvs_entry_next(it);
      if (it == NULL)
      {
         sprintf(buffer, "%s", info.key);
#ifdef DEBUG_NVS
         ESP_LOGI(LOG_NVS, "%s", info.key);
#endif
         break;
      }

      qty_ev++;
   };
   nvs_release_iterator(it);

   // String nula gera numero 0, então sempre pularia o evento 0.
   if (strlen(buffer) > 0)
   {
      if (qty_ev > 0)
      {
         int idx = atol(buffer);
         if (idx >= MAX_QUANTITY_EVENTS)
            idx = 0;
         config_events.index_ev = idx % MAX_QUANTITY_EVENTS; // Converter
      }
      else
      {
         config_events.index_ev = 0;
      }
   }
   else
   {
      config_events.index_ev = 0;
   }
   config_events.qty_ev = qty_ev;

#ifdef DEBUG_NVS
   ESP_LOGI(LOG_NVS, "qty_ev:%u, index:%u, key:'%s'", config_events.qty_ev, config_events.index_ev, buffer);
#endif

   xSemaphoreGive(mutex_nvs);
   return ESP_OK;
}

static esp_err_t app_init_partition_report()
{
   esp_err_t err;
   err = nvs_flash_init_partition(report);

#ifdef DEBUG_NVS
   nvs_stats_t nvs_stats;
   ESP_LOGW(LOG_NVS, "report:  %s", esp_err_to_name(err));
   esp_err_t err_stat = nvs_get_stats(report, &nvs_stats);
   ESP_LOGI(LOG_NVS, "P:%s", report);
   ESP_LOGI(LOG_NVS, "Count: err = (%s)", esp_err_to_name(err_stat));
   ESP_LOGI(LOG_NVS, "UsedEntries = (%d)", nvs_stats.used_entries);
   ESP_LOGI(LOG_NVS, "FreeEntries = (%d)", nvs_stats.free_entries);
   ESP_LOGI(LOG_NVS, "AllEntries = (%d)", nvs_stats.total_entries);
#endif

   return err;
}

static esp_err_t app_init_partition_config()
{
   esp_err_t err;
   err = nvs_flash_init_partition(memoria);

#ifdef DEBUG_NVS
   nvs_stats_t nvs_stats;
   ESP_LOGW(LOG_NVS, "memoria: %s", esp_err_to_name(err));
   esp_err_t err_stat = nvs_get_stats(memoria, &nvs_stats);
   ESP_LOGI(LOG_NVS, "P:%s", memoria);
   ESP_LOGI(LOG_NVS, "Count: err = (%s)", esp_err_to_name(err_stat));
   ESP_LOGI(LOG_NVS, "UsedEntries = (%d)", nvs_stats.used_entries);
   ESP_LOGI(LOG_NVS, "FreeEntries = (%d)", nvs_stats.free_entries);
   ESP_LOGI(LOG_NVS, "AllEntries = (%d)", nvs_stats.total_entries);
#endif
   return err;
}

static esp_err_t app_nvs_init()
{
   mutex_nvs = xSemaphoreCreateMutex();
   esp_err_t err;

   int count = 0;
#ifdef DEBUG_NVS
   ESP_LOGW(LOG_NVS, "MALLOC_CAP_8BIT=%d", heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
   ESP_LOGW(LOG_NVS, "MALLOC_CAP_INTERNAL=%d", heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL));
   ESP_LOGW(LOG_NVS, "MALLOC_CAP_IRAM_8BIT=%d", heap_caps_get_largest_free_block(MALLOC_CAP_IRAM_8BIT));
   ESP_LOGW(LOG_NVS, "MALLOC_CAP_SPIRAM=%d", heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM));
#endif
   count = 0;
   do
   {
      err = nvs_flash_init();
      if (err == ESP_OK)
         break;
      if (++count > 10)
         esp_restart();
      vTaskDelay(100 / portTICK_PERIOD_MS);
   } while (true);

#ifdef DEBUG_NVS
   ESP_LOGW(LOG_NVS, "MALLOC_CAP_8BIT=%d", heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
   ESP_LOGW(LOG_NVS, "MALLOC_CAP_INTERNAL=%d", heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL));
   ESP_LOGW(LOG_NVS, "MALLOC_CAP_IRAM_8BIT=%d", heap_caps_get_largest_free_block(MALLOC_CAP_IRAM_8BIT));
   ESP_LOGW(LOG_NVS, "MALLOC_CAP_SPIRAM=%d", heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM));
#endif

   count = 0;
   do
   {
      err = app_init_partition_report();
      if (err == ESP_OK)
         break;
      if (++count > 10)
         esp_restart();
      vTaskDelay(100 / portTICK_PERIOD_MS);
   } while (true);

#ifdef DEBUG_NVS
   ESP_LOGW(LOG_NVS, "MALLOC_CAP_8BIT=%d", heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
   ESP_LOGW(LOG_NVS, "MALLOC_CAP_INTERNAL=%d", heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL));
   ESP_LOGW(LOG_NVS, "MALLOC_CAP_IRAM_8BIT=%d", heap_caps_get_largest_free_block(MALLOC_CAP_IRAM_8BIT));
   ESP_LOGW(LOG_NVS, "MALLOC_CAP_SPIRAM=%d", heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM));
#endif

   count = 0;
   do
   {
      err = app_init_partition_config();
      if (err == ESP_OK)
         break;
      if (++count > 10)
         esp_restart();
      vTaskDelay(100 / portTICK_PERIOD_MS);
   } while (true);

#ifdef DEBUG_NVS
   ESP_LOGW(LOG_NVS, "MALLOC_CAP_8BIT=%d", heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
   ESP_LOGW(LOG_NVS, "MALLOC_CAP_INTERNAL=%d", heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL));
   ESP_LOGW(LOG_NVS, "MALLOC_CAP_IRAM_8BIT=%d", heap_caps_get_largest_free_block(MALLOC_CAP_IRAM_8BIT));
   ESP_LOGW(LOG_NVS, "MALLOC_CAP_SPIRAM=%d", heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM));
#endif

   return err;
}

esp_err_t nvs_app_read_event(event_t *ev, int idx)
{
   xSemaphoreTake(mutex_nvs, portMAX_DELAY);
   nvs_handle_t nvs_handle;
   esp_err_t err;

   err = nvs_open_from_partition(report, report, NVS_READONLY, &nvs_handle);
   if (err != ESP_OK)
      goto fim;

   // Endereco do evento
   char key[15] = "";

   sprintf(key, "%05d", idx % MAX_QUANTITY_EVENTS); // Generate key with block event overflow

#ifdef DEBUG_NVS
   ESP_LOGI(LOG_NVS, "read_events key: %s", key);
#endif

   event_t ev_temp;
   size_t size = sizeof(event_t);
   err = nvs_get_blob(nvs_handle, key, &ev_temp, &size);

   if (err != ESP_OK)
      goto fim;

   *ev = ev_temp;

fim:
   nvs_close(nvs_handle);
   xSemaphoreGive(mutex_nvs);
#ifdef DEBUG_NVS
   ESP_LOGI(LOG_NVS, "read_events: %s", esp_err_to_name(err));
#endif
   return err;
}

esp_err_t nvs_app_write_event(event_t *ev)
{
   xSemaphoreTake(mutex_nvs, portMAX_DELAY);
   nvs_handle_t nvs_handle;
   esp_err_t err;

   err = nvs_open_from_partition(report, report, NVS_READWRITE, &nvs_handle);
   if (err != ESP_OK)
      goto fim;

   // Index
   char key[15] = "";
   int idx = config_events.index_ev % MAX_QUANTITY_EVENTS;

   // First you must erase the next key before recording the event as it may lose power and miss the idx of the report.
   sprintf(key, "%05d", idx + 1);
   err = nvs_erase_key(nvs_handle, key);
   err = nvs_commit(nvs_handle);

   sprintf(key, "%05d", idx);

   ev->pos = idx;
   err = nvs_set_blob(nvs_handle, key, ev, sizeof(event_t));
   if (err != ESP_OK)
   {
#ifdef DEBUG_NVS
      ESP_LOGE(LOG_NVS, "Event error: id:%d - err: %s", config_events.index_ev, esp_err_to_name(err));
#endif
      goto fim;
   }

   // Event overflow back to 0
   idx++;
   idx %= MAX_QUANTITY_EVENTS;

   if ((err != ESP_OK) && (err != ESP_ERR_NVS_NOT_FOUND))
      goto fim;

   err = nvs_commit(nvs_handle);

   // Increment event counter and check overflow to return to 0.
   if (err == ESP_OK)
   {
#ifdef DEBUG_NVS
      ESP_LOGW(LOG_NVS, "Write event: id:%d, tipo:%d", config_events.index_ev, ev->type);
#endif
      config_events.index_ev++;
      if (config_events.index_ev >= MAX_QUANTITY_EVENTS)
      {
         config_events.index_ev = 0;
      }

      // Always clear the after event to find the idx of the report.
      if (config_events.qty_ev < MAX_QUANTITY_EVENTS - 1)
      {
         config_events.qty_ev++;
      }
   }
   else
   {
#ifdef DEBUG_NVS
      ESP_LOGE(LOG_NVS, "Erro Evento: id:%d - err: %s", config_events.index_ev, esp_err_to_name(err));
#endif
   }

fim:
   nvs_close(nvs_handle);
   xSemaphoreGive(mutex_nvs);

#ifdef DEBUG_NVS
   ESP_LOGI(LOG_NVS, "write_events: %d", err);
#endif

   return err;
}

#define __NVS_APP_C__
#endif