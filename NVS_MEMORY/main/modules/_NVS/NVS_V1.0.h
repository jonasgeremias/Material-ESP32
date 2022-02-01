#define memoria "config"
#define report "report"

#define MAX_QUANTITY_EVENTS 5000

#ifdef DEBUG_NVS
const char *LOG_NVS = "NVS";
#endif

/* Global NVS structs and variables */
typedef struct config_events_t
{
   uint16_t index_ev;
   uint16_t qty_ev;
} config_events_t;

typedef uint32_t nvs_handle_t;
static SemaphoreHandle_t mutex_nvs;
config_events_t config_events;

/* writing and reading functions with protection */
esp_err_t app_nvs_get_blob(nvs_handle_t handle, const char *key, void *out_value, size_t *length);
esp_err_t app_nvs_get_i8(nvs_handle_t handle, const char *key, int8_t *out_value);
esp_err_t app_nvs_set_blob(nvs_handle_t handle, const char *key, const void *value, size_t length);
esp_err_t app_nvs_set_i8(nvs_handle_t handle, const char *key, int8_t value);

/* Aplication functions init */
static esp_err_t nvs_app_init_config_events();
static esp_err_t app_init_partition_report();
static esp_err_t app_init_partition_config();
static esp_err_t app_nvs_init();

/* Read and write events functions */
esp_err_t nvs_app_read_event(event_t *ev, int end);
esp_err_t nvs_app_write_event(event_t *ev);
