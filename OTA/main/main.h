// Configurações da aplicação alteradas via OTA -------------------------------
// Rede Wifi
#define SSID_WIFI_AP "OTA-WEBSERVER"
#define PASS_WIFI_AP "123456789"
#define WIFI_AP_MAX_CON 1

// Rede local
#define SSID_WIFI_STA "Jesus_te_ama"
#define PASS_WIFI_STA "quesenha"

// LED Status -----------------------------------------------------------------
#define PIN_LED 2
#define TEMPO_BLINK 100

// Controle de logs das bibliotecas criadas -----------------------------------
#include "esp_log.h"
#define DEBUG_TASK_MONITOR
#define DEBUG_AP_SERVER
#define DEBUG_WIFI
#define DEBUG_OTA
#define DEBUG_MAIN

#ifdef DEBUG_MAIN
const char *LOG_MAIN = "MAIN";
#endif

// Funçoes MAKE - aso Precise pra algo ----------------------------------------
#define MAKE8(a, i) ((uint8_t)(a >> (8 * (3 - i)) & 0xff))
#define MAKE32(a, b, c, d) (((uint32_t)((a)&0xff) << 24) | ((uint32_t)((b)&0xff) << 16) | ((uint32_t)((c)&0xff) << 8) | (uint32_t)((d)&0xff))
#define BIT_TEST(a, p) ((a & ((int)1 << p)) >> p)
#define BIT_SET(a, b) (a |= 1 << b)
#define BIT_CLEAR(a, b) (a &= ~(1 << b))

// Controle e monitoramento das tarefas
static TaskHandle_t taskhandle_app_controller;
static EventGroupHandle_t app_event_group;
#define CONECTADO_NO_WIFI BIT0
#define RECEBEU_IP_DO_ROTEADOR BIT1

void print_heap(int i);
void app_controller(void *pvParameters);

// Wifi -----------------------------------------------------------------------
#include "libs/_Wifi/Wifi.h"
#include "libs/_AP_Server/AP_Server.h"
#include "libs/_Wifi/Wifi.c"
#include "libs/_AP_Server/AP_Server.c"