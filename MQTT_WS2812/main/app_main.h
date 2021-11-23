#define DEBUG_UDP

// Biblioteca do MQTT
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
        .rising = 5000,
        .on = 0,
        .falling = 5000,
        .off = 0}};