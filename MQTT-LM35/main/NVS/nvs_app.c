#include "nvs_app.h"

static esp_err_t app_nvs_init() {
   mutex_nvs = xSemaphoreCreateMutex();
   esp_err_t err;
   err = nvs_flash_init_partition(memoria);
   ESP_ERROR_CHECK(err);
   return err;
}

static esp_err_t nvs_app_write_config_module(device_t *device) {
   xSemaphoreTake(mutex_nvs, portMAX_DELAY);
   nvs_handle_t nvs_handle;
   esp_err_t err;

   err = nvs_open_from_partition(memoria, memoria, NVS_READWRITE, &nvs_handle);
   if (err != ESP_OK)
      goto fim;

   err = nvs_set_blob(nvs_handle, "device", device, sizeof(device_t));
   if (err != ESP_OK)
      goto fim;

   err = nvs_commit(nvs_handle); // Publica

fim:
   nvs_close(nvs_handle);
   xSemaphoreGive(mutex_nvs);
   return err;
}

static esp_err_t carrega_configuracao_default(device_t *device) {
   if (device->versao_firmware == 0) {
      // @audit-info Carregar configurações iniciais aqui 
      ESP_LOGW(memoria, "Carregou as configurações iniciais");
   }

   device->versao_firmware = VERSAO_FIRMWARE;

   xSemaphoreTake(mutex_nvs, portMAX_DELAY);
   nvs_handle_t nvs_handle;
   esp_err_t err;

   err = nvs_open_from_partition(memoria, memoria, NVS_READWRITE, &nvs_handle);
   if (err != ESP_OK)
      goto fim;

   err = nvs_set_blob(nvs_handle, "device", (void *) &device, sizeof(device));
   err = nvs_commit(nvs_handle); // Garante que gravou corretamente

fim:
   nvs_close(nvs_handle);
   xSemaphoreGive(mutex_nvs);
   return err;
}


// Leitura NVS
static esp_err_t nvs_app_read_module(device_t *device) {
   xSemaphoreTake(mutex_nvs, portMAX_DELAY);
   nvs_handle_t nvs_handle;
   esp_err_t err;

   err = nvs_open_from_partition(memoria, memoria, NVS_READONLY, &nvs_handle);
   if (err != ESP_OK)
      goto fim;

   size_t size = sizeof(device_t);
   err = nvs_get_blob(nvs_handle, "device", device, &size);

fim:
   nvs_close(nvs_handle);
   xSemaphoreGive(mutex_nvs);

   if (device->versao_firmware != VERSAO_FIRMWARE) {
      if (err != ESP_OK) {
        carrega_configuracao_default(device);
      }
   }

   return err;
}
