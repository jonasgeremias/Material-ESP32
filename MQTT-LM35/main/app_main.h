// Para debug via UDP
#define DEBUG_UDP

// @audit-info Para testes locais, defina LOCAL_TEST
// #define LOCAL_TEST

// Botões
#define BUTTON_MAIS 0 // Boot

#define LED_STATUS 16
#include "LED_STATUS/LED_STATUS.c"

// Biblioteca para tratar o pacote json recebido
#include "cJSON.c"

// biblioteca da analógica
#define PIN_ANALOG ADC1_CHANNEL_3
#include "ANALOG/analog.c"

#define MAX_DEVICES_LENGTH 5 // Se mudar este define, deve ser limpa a NVS, pois o tamanho das struct vai mudar.

const uint32_t ID_DEVICE = 0;
const uint32_t VERSAO_FIRMWARE = 10;

//  Configuração do mqtt
typedef struct config_mqtt_t
{
    char user[32];
    char pass[32];
    char uri[256];
    int port;
} config_mqtt_t;

typedef struct device_t
{
    uint32_t versao_firmware;
    uint32_t id;
    uint8_t mac_address[6];
    int64_t period;
    char lg[13];
    char lt[13];
    char description[17];
    float temperature;
    int subscribe_ids[MAX_DEVICES_LENGTH];
} device_t;

typedef struct device_sub_t
{
    device_t device;
    bool act;
    int id;
} device_sub_t;

enum
{
    MQTT_DESCONECTADO = 0,
    MQTT_CONECTADO = 1,
    MQTT_MAX
};

/******************************************************************************
 *  Variáveis Globais 
 * ***************************************************************************/
static const char *LOG_MQTT = "MQTT_APP";  // Imprimir no LOG
static TaskHandle_t taskhandle_controller; // handle da tarefa de contrle.
static QueueHandle_t device_queue;         // Para receber configurações do MQTT na tarefa de controle.
esp_mqtt_client_handle_t mqtt_client;
static volatile int status_mqtt = MQTT_DESCONECTADO;

static device_t device = {
    .versao_firmware = 0,
    .id = ID_DEVICE,
    .mac_address = {0, 0, 0, 0, 0, 0},
    .period = 30,
    .lg = "-49.43364752",
    .lt = "-28.701026",
    .description = "Casa do Jonas",
    .subscribe_ids = {-1, -1, -1, -1, -1},
    .temperature = 0};

#ifdef LOCAL_TEST
static volatile config_mqtt_t config_mqtt = {
    .uri = "mqtt://192.168.1.167",
    .port = 1883,
    .user = "device",
    .pass = "device123"};
#else
static volatile config_mqtt_t config_mqtt = {
    .uri = "mqtt://e066f399.us-east-1.emqx.cloud",
    .port = 15584,
    .user = "clientJonas123",
    .pass = "clientJonas123"};
#endif

device_sub_t device_sub[MAX_DEVICES_LENGTH]; // Para assinar e monitorar

/******************************************************************************
 * biblioteca Display SSD1306 
 * ***************************************************************************/
#include "SSD1306/ssd1306_app.c"
#include "_libs/_serial_utilities_.c"
#include "IMAGES/images.h"