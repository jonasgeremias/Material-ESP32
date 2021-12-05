#include "nvs_flash.h"
#include "nvs.h"

/******************************************************************************
 * Configurações do módulo
 *****************************************************************************/
typedef uint32_t nvs_handle_t;
static SemaphoreHandle_t mutex_nvs;

#define memoria "nvs"

static esp_err_t app_nvs_init();

/* leitura e gravação das configurações do módulo. */
static esp_err_t nvs_app_read_module(device_t *device);
static esp_err_t nvs_app_write_config_module(device_t *device);

/* Carregar as configurações default. */
static esp_err_t carrega_configuracao_default(device_t *device);
