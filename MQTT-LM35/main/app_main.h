#define DEBUG_UDP

// Botões
#define BUTTON_MAIS 0 // Boot
// #define BUTTON_MENOS 2

#define LED_STATUS 16
#include "LED_STATUS/LED_STATUS.c"

// Biblioteca para tratar o pacote json recebido
#include "cJSON.c"

#define PIN_ANALOG ADC1_CHANNEL_3
#include "ANALOG/analog.c"

//  Configuração do mqtt
typedef struct config_mqtt_t
{
    char user[32];
    char pass[32];
    char uri[256];
    int port;
    int id;
} config_mqtt_t;

typedef struct device_t
{
    int64_t period;
    char lg[13];
    char lt[13];
    char description[17];
    float temperature;
} device_t;

typedef struct device_sub_t {
    device_t device;
    bool act;
    int id;
}device_sub_t;

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
static QueueHandle_t device_queue;  // Para receber configurações do MQTT na tarefa de controle.
esp_mqtt_client_handle_t client;
static volatile int status_mqtt = MQTT_DESCONECTADO;

static device_t device = {
    .period = 30,
    .lg = "-49.43364752",
    .lt = "-28.701026",
    .description = "Casa do Jonas",
    .temperature = 0};

#define MAX_DEVICES_LENTH 5
device_sub_t device_sub[MAX_DEVICES_LENTH]; // Para assinar e monitorar
int habilita_devices_sub_id[MAX_DEVICES_LENTH] = {2,3,-1,-1,-1};

// Configuração mosquitto local
// static volatile config_mqtt_t config_mqtt = {
//     .uri = "mqtt://192.168.1.167",
//     .port = 1883,
//     .user = "device",
//     .pass = "device123",
//     .id = 1};

static volatile config_mqtt_t config_mqtt = {
    .uri = "mqtt://e066f399.us-east-1.emqx.cloud",
    .port = 15584,
    .user = "clientJonas123",
    .pass = "clientJonas123",
    .id = 1};

/******************************************************************************
 * biblioteca Display SSD1306 
 * ***************************************************************************/
#include "SSD1306/ssd1306_app.c"
#include "_libs/_serial_utilities_.c"
#include "IMAGES/images.h"