#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "mqtt_client.h"
#define BUTTON 0

// Para pegar o tamanho do array
#define ARRAY_SIZE_OF(a) (sizeof(a) / sizeof(a[0]))
#define QTD_MODOS 17
// Biblioteca de LEDs
#define WS2812_LED_PIN GPIO_NUM_19
#define WS2812_QTD_LEDS 72
#include "SPI_WS2812/SPI_WS2812.c"

#define LED_STATUS 2
#include "LED_STATUS/LED_STATUS.c"

// Biblioteca para tratar o pacote json recebido
#include "cJSON.c"

typedef enum effects_types_t
{
    EFFECTS_MODE_MONO = 0,
    EFFECTS_MODE_MONO_SEQUENCE,
    EFFECTS_MODE_ARRAY_COLORS,
    EFFECTS_MODE_ARRAY_SPIN,
    EFFECTS_MODE_MAX
} effects_mode_t;

typedef struct config_device_t
{
    char user[32];
    char pass[32];
    char uri[256];
    int port;
    int id;
} config_device_t;

typedef union
{
    struct
    {
        uint32_t red : 8;
        uint32_t green : 8;
        uint32_t blue : 8;
        uint32_t nop : 8;
    };
    struct
    {
        uint32_t r : 8;
        uint32_t g : 8;
        uint32_t b : 8;
        uint32_t n : 8;
    };
    uint32_t rgb;
} RGB_t;

typedef enum sequence_operation_mode_t
{
    SEQ_OPERATON_MODE_INIT = 0,
    SEQ_OPERATON_MODE_RISING,
    SEQ_OPERATON_MODE_ON,
    SEQ_OPERATON_MODE_FALLING,
    SEQ_OPERATON_MODE_OFF
} sequence_operation_mode_t;

typedef struct config_cycle_timers_t
{
    uint64_t rising;
    uint64_t on;
    uint64_t falling;
    uint64_t off;
} config_cycle_timers_t;

/******************************************************************************
 *  Estruturas de controle 
 * ***************************************************************************/
typedef struct config_WS2812_t
{
    uint16_t mode;
    uint16_t qtd_colors;
    char description[64];
    RGB_t colors[WS2812_QTD_LEDS];
    config_cycle_timers_t timer;
} config_WS2812_t;

/******************************************************************************
 *  Variáveis Globais 
 * ***************************************************************************/
static const char *LOG_MQTT = "MQTT_WS2812";   // Imprimir no LOG
static TaskHandle_t taskhandle_led_controller; // handle da tarefa de contrle dos leds.
static QueueHandle_t config_led_queue;         // Para receber configurações do MQTT na tarefa de controle.
uint64_t cycle_timer[4] = {0, 0, 0, 0};
uint64_t cycle_timer_timeout = 0;
uint64_t timer_init = 0; // para conseguir a porcentagem
sequence_operation_mode_t sequence_operation_mode = SEQ_OPERATON_MODE_INIT;
uint16_t count_sequence_cycles = 0;
RGB_t demo_color = {.rgb = RED};

static volatile config_device_t config_device = {
    .uri = "mqtt://192.168.1.167",
    .port = 1883,
    .user = "device",
    .pass = "device123",
    .id = 1};

static config_WS2812_t config_leds = {
    .mode = 0,
    .qtd_colors = 10,
    .description = "Fill",
    .timer = {
        .rising = 250,
        .on = 500,
        .falling = 250,
        .off = 500}};

/******************************************************************************
 *  Protótipos das funções 
 * ***************************************************************************/
static void atualiza_leds(uint8_t mode);
static bool recebe_config_led();
static void task_ws2812_controller(void *pv);
static void mqtt_recebe_config(char *data);

/******************************************************************************
 *  Funções
 * ***************************************************************************/
static void mqtt_recebe_config(char *data)
{
    float color = 0;
    config_WS2812_t config_temp;
    memset(&config_temp, 0, sizeof(config_WS2812_t));
    cJSON *param = NULL; // para ler o parametro
    cJSON *config = cJSON_Parse(data);
    cJSON *array_item = NULL; // Para o array

    if (config == NULL)
        goto erro;
    ESP_LOGI(LOG_MQTT, "config: %s", cJSON_Print(config));

    param = cJSON_GetObjectItem(config, "mode");
    if (cJSON_IsNumber(param))
    {
        config_temp.mode = param->valueint;
    }

    param = cJSON_GetObjectItem(config, "timer_on");
    if (cJSON_IsNumber(param))
    {
        config_temp.timer.on = param->valueint;
    }

    param = cJSON_GetObjectItem(config, "timer_rising");
    if (cJSON_IsNumber(param))
    {
        config_temp.timer.rising = param->valueint;
    }

    param = cJSON_GetObjectItem(config, "timer_off");
    if (cJSON_IsNumber(param))
    {
        config_temp.timer.off = param->valueint;
    }

    param = cJSON_GetObjectItem(config, "timer_falling");
    if (cJSON_IsNumber(param))
    {
        config_temp.timer.falling = param->valueint;
    }

    param = cJSON_GetObjectItem(config, "colors");
    if (cJSON_IsArray(param))
    {

        config_temp.qtd_colors = cJSON_GetArraySize(param);
        ESP_LOGI(LOG_MQTT, "array colors size: %d", config_temp.qtd_colors);

        for (int i = 0; i < WS2812_QTD_LEDS; i++)
        {
            array_item = cJSON_GetArrayItem(param, i);
            if (array_item != NULL)
            {
                color = strtol(array_item->valuestring, NULL, 16);
                if (!isnan(color))
                {
                    config_temp.colors[i].rgb = color;
                }
                else
                    config_temp.colors[i].rgb = 0;
            }
            else
                config_temp.colors[i].rgb = 0;
        }
    }

    param = cJSON_GetObjectItem(config, "description");
    if (cJSON_IsString(param))
    {
        sprintf(config_temp.description, "%*s", 60, param->valuestring);
    }

    // Envia para a tarefa de controle dos LEDs
    if (xQueueSend(config_led_queue, &config_temp, portMAX_DELAY) != pdTRUE)
    {
        ESP_LOGE(LOG_MQTT, "erro ao enviar as configuracoes");
    }
    else
    {
        ESP_LOGI(LOG_MQTT, "Configuracoes enviadas.");
    }

    if (config != NULL)
        cJSON_Delete(config); // Liberar a memoria teporário do json.

    return;
erro:
    ESP_LOGE(LOG_MQTT, "Parse JSON error");
}

static void mqtt_recebe_modo(char *data)
{
    // float color = 0;
    // config_WS2812_t config_temp;
    // memset(&config_temp, 0, sizeof(config_WS2812_t));
    // cJSON *param = NULL; // para ler o parametro
    cJSON *config = cJSON_Parse(data);
    // cJSON *array_item = NULL; // Para o array

    if (config == NULL)
        goto erro;
    ESP_LOGI(LOG_MQTT, "config: %s", cJSON_Print(config));

    if (config != NULL)
        cJSON_Delete(config); // Liberar a memoria teporário do json.

    return;
erro:
    ESP_LOGE(LOG_MQTT, "Parse JSON mode error");
}

static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0)
    {
        ESP_LOGE(LOG_MQTT, "Last error %s: 0x%x", message, error_code);
    }
}

static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event)
{
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    char buffer[64];

    // your_context_t *context = event->context;
    switch (event->event_id)
    {
    case MQTT_EVENT_CONNECTED: 
        // Assinar o tópico para receber configurações
        sprintf(buffer, "/lampadas/%d/config", config_device.id);
        ESP_LOGI(LOG_MQTT, "Assinando o topico: '%s'", buffer);
        msg_id = esp_mqtt_client_subscribe(client, (char *)&buffer, 1);
        ESP_LOGI(LOG_MQTT, "Subscribe successful config, msg_id=%d", msg_id);

        sprintf(buffer, "/lampadas/%d/mode", config_device.id);
        ESP_LOGI(LOG_MQTT, "Assinando o topico: '%s'", buffer);
        msg_id = esp_mqtt_client_subscribe(client, (char *)&buffer, 1);
        ESP_LOGI(LOG_MQTT, "Subscribe successful mode, msg_id=%d", msg_id);
        
        modo_led_status(LED_STATUS_LIGADO);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(LOG_MQTT, "MQTT_EVENT_DISCONNECTED");
        modo_led_status(LED_STATUS_DESLIGADO);
        break;
    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(LOG_MQTT, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        // msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
        // ESP_LOGI(LOG_MQTT, "sent publish successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(LOG_MQTT, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(LOG_MQTT, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(LOG_MQTT, "MQTT_EVENT_DATA");
        //printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        //printf("DATA=%.*s\r\n", event->data_len, event->data);

        if (strstr(event->topic, "config") != NULL)
        {
            mqtt_recebe_config((char *)event->data);
        }
        else if (strstr(event->topic, "mode") != NULL)
        {
            mqtt_recebe_modo((char *)event->data);
        }

        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(LOG_MQTT, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT)
        {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno", event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(LOG_MQTT, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
        }
        break;
    default:
        ESP_LOGI(LOG_MQTT, "Other event id:%d", event->event_id);
        break;
    }
    return ESP_OK;
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(LOG_MQTT, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    mqtt_event_handler_cb(event_data);
}

static void mqtt_app_start(void)
{
    ESP_LOGW(LOG_MQTT, "Configuração do dispositivo");
    ESP_LOGW(LOG_MQTT, "uri:'%s'", config_device.uri);
    ESP_LOGW(LOG_MQTT, "port:'%d'", config_device.port);
    ESP_LOGW(LOG_MQTT, "user:'%s'", config_device.user);
    ESP_LOGW(LOG_MQTT, "pass:'%s'", config_device.pass);
    ESP_LOGW(LOG_MQTT, "id:'%d'", config_device.id);

    esp_mqtt_client_config_t mqtt_cfg = {
        .uri = (char *)&config_device.uri,
        .port = config_device.port,
        .username = (char *)&config_device.user,
        .password = (char *)&config_device.pass};

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);
    esp_mqtt_client_start(client);
}

uint32_t corAleatoria()
{
    return (random() >> 3) && 0xFFFFFF;
}

void update_color_rgb(sequence_operation_mode_t *sequence, float *percent_time, RGB_t *color)
{

    RGB_t color_temp;
    switch (*sequence)
    {
    case SEQ_OPERATON_MODE_OFF:
        fillColor(0);
        break;
    case SEQ_OPERATON_MODE_RISING:
        color_temp.r = PERCENT(color->r, *percent_time);
        color_temp.g = PERCENT(color->g, *percent_time);
        color_temp.b = PERCENT(color->b, *percent_time);
        fillColor((uint32_t)color_temp.rgb);
        break;
    case SEQ_OPERATON_MODE_ON:
        fillColor((uint32_t)color->rgb);
        break;
    case SEQ_OPERATON_MODE_FALLING:
        color_temp.r = PERCENT(color->r, (100 - *percent_time));
        color_temp.g = PERCENT(color->g, (100 - *percent_time));
        color_temp.b = PERCENT(color->b, (100 - *percent_time));
        fillColor((uint32_t)color_temp.rgb);
        break;
    default:
        fillColor(0);
        break;
    }
}

void led_controller(bool demo)
{
    uint64_t timer = esp_timer_get_time(); // #define millis esp_timer_get_time
    float percent_time = 0;

    // Controle de mode de operação, carrega os valores se no inicio d mode
    if (sequence_operation_mode == 0)
    {
        cycle_timer[0] = config_leds.timer.rising;
        cycle_timer[1] = config_leds.timer.on;
        cycle_timer[2] = config_leds.timer.falling;
        cycle_timer[3] = config_leds.timer.off;
        cycle_timer_timeout = cycle_timer[0] * 1000 + timer;
        timer_init = timer;
        sequence_operation_mode++;
    }

    // Controle de tempo e porcentagem de tempo
    if (timer >= cycle_timer_timeout)
    {
        if (++sequence_operation_mode > 4)
        {
            sequence_operation_mode = 0;

            // Isso é para modo sequencia
            if (++count_sequence_cycles >= config_leds.qtd_colors) {
                count_sequence_cycles = 0;
            }

            ESP_LOGW("COLOR", "count_sequence_cycles: %d",count_sequence_cycles);
        }
        cycle_timer_timeout = cycle_timer[sequence_operation_mode - 1] * 1000 + timer;
        timer_init = timer;
    }
    else
    {
        percent_time = (float)(timer - timer_init) / (cycle_timer_timeout - timer_init);
        percent_time *= 100;
        if (percent_time > 100.0)
            percent_time = 100;
    }
    

    switch (config_leds.mode) {
        case EFFECTS_MODE_MONO: 
            update_color_rgb(&sequence_operation_mode, &percent_time, (demo)? &demo_color: &config_leds.colors[0].rgb);
        break;
        case EFFECTS_MODE_MONO_SEQUENCE:
            update_color_rgb(&sequence_operation_mode, &percent_time, (demo)? &demo_color: &config_leds.colors[count_sequence_cycles].rgb);
        break;
        case EFFECTS_MODE_ARRAY_COLORS:
            // @pending criar modo.
        break;
        case EFFECTS_MODE_ARRAY_SPIN:
            // @pending criar modo.
        break;
        default: 
            update_color_rgb(&sequence_operation_mode, &percent_time, (demo)? &demo_color: &config_leds.colors[0]);
        break;
    }

    // Aqui deende do mode
    
}

static void atualiza_leds(uint8_t mode)
{
    static uint8_t modo_anterior = 0xff;

    if (mode != modo_anterior)
    {
        modo_anterior = mode;
        count_sequence_cycles = 0;
        sequence_operation_mode = SEQ_OPERATON_MODE_OFF;
    }

    switch (mode)
    {
    case 0:
        demo_color.rgb = BLACK;
        led_controller(1);
        break; 
    case 1:
        demo_color.rgb = RED;
        led_controller(1);
        break;
    case 2:
        demo_color.rgb = GREEN;
        led_controller(1);
        break;
    case 3:
        demo_color.rgb = BLUE;
        led_controller(1);
        break;
    case 4:
        demo_color.rgb = WHITE;
        led_controller(1);
        break;
    case 5:
        demo_color.rgb = YELLOW;
        led_controller(1);
        break;
    case 6:
        demo_color.rgb = CYAN;
        led_controller(1);
        break;
    case 7:
        demo_color.rgb = MAGENTA;
        led_controller(1);
        break;
    case 8:
        demo_color.rgb = PURPLE;
        led_controller(1);
        break;
    case 9:
        demo_color.rgb = ORANGE;
        led_controller(1);
        break;
    case 10:
        demo_color.rgb = PINK;
        led_controller(1);
        break;
    case 11:
        led_controller(0);
        break;
    case 12:
        for (int i = 0; i < WS2812_QTD_LEDS; i++)
        {
            int color = (uint32_t)config_leds.colors[i].rgb;
            updateColorLed(color, i);
        }
        break;
    case 13:
        fillBuffer((uint32_t)&config_leds.colors, WS2812_QTD_LEDS);
        break;
    default:
        fillColor(0);
        break;
    }

    led_strip_update();
}

static bool recebe_config_led()
{
    config_WS2812_t config_temp;
    if (xQueueReceive(config_led_queue, &config_temp, 0) == pdTRUE)
    {
        // Transfere a configuração para a estrutura de controle
        memcpy(&config_leds, &config_temp, sizeof(config_WS2812_t));
        ESP_LOGI(LOG_MQTT, "Recebeu configuracao");

        return true;
    }

    return false;
}

static bool monitora_botao()
{
    static bool status_anteror = 0;
    bool status = 0;
    status = gpio_get_level(BUTTON);

    if ((!status) && (status_anteror))
    {
        status_anteror = status;
        return true;
    }

    status_anteror = status;
    return false;
}

static void task_ws2812_controller(void *pv)
{
    uint8_t mode = 0;

    while (1)
    {
        recebe_config_led();
        if (monitora_botao())
        {
            if (++mode > QTD_MODOS)
                mode = 0;
            ESP_LOGI(LOG_MQTT, "recebe_config_led => %lld => mode : %d", esp_timer_get_time(), mode);
        }
        atualiza_leds(mode);
    }

    vTaskDelete(NULL);
}

void inicia_hardware()
{
    // Primeira coisa a se fazer é desligar a fita
    initSPIws2812();
    fillColor(0);
    led_strip_update();

    gpio_pad_select_gpio(BUTTON);
    gpio_set_direction(BUTTON, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BUTTON, GPIO_PULLUP_ONLY);

    estrobo_led_status_ms(100);
}

void app_main(void)
{
    inicia_hardware();

    ESP_LOGI(LOG_MQTT, "[APP] Startup..");
    ESP_LOGI(LOG_MQTT, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(LOG_MQTT, "[APP] IDF version: %s", esp_get_idf_version());
    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("MQTT_CLIENT", ESP_LOG_VERBOSE);
    esp_log_level_set("MQTT_EXAMPLE", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_TCP", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_SSL", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT", ESP_LOG_VERBOSE);
    esp_log_level_set("OUTBOX", ESP_LOG_VERBOSE);

    // Fila de 2 configurações.
    config_led_queue = xQueueCreate(2, sizeof(config_WS2812_t));
    init_task_led_status_controller();
    xTaskCreatePinnedToCore(task_ws2812_controller, "task_ws2812_controller", 15000, NULL, 1, &taskhandle_led_controller, 1);

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(example_connect());
    mqtt_app_start();
}
