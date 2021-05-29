
#ifdef DEBUG_WIFI
const char *LOG_WIFI = "Wifi";
#endif

esp_netif_t *app_sta_netif;
esp_netif_t *app_ap_netif;
#include "mdns.h"

static TaskHandle_t taskhandle_wifi_app_control;
uint8_t mac_address[6] = {0, 0, 0, 0, 0, 0};

static volatile bool esp_restart_now = 0; // Para reset do ESP32, setar para 1 e adicionar tempo

// Protótipos de funções ------------------------------------------------------

/******************************************************************************
 * Trata Os eventos do wifi.
 *****************************************************************************/
static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data);
//static esp_err_t event_handler(void *ctx, system_event_t *event);

/******************************************************************************
 * Configura o ponto de acesso.
 *****************************************************************************/
static esp_err_t wifi_ap_config();

/******************************************************************************
 * Configura o IP do ponto de acesso como fixo.
 *****************************************************************************/
static void config_tcpip_ap_fix();

/******************************************************************************
 * Configura o login e senha para conectar o ESP (STA) em um Wi-Fi disponivel.
 *****************************************************************************/
static esp_err_t wifi_sta_config();

/******************************************************************************
 * Inicia o modulo Wi-Fi do ESP no modo AP-STA.
 *****************************************************************************/
static void wifi_init();

/******************************************************************************
 * Função para desconectar do Wi-Fi e desabilitar a reconexão.
 *****************************************************************************/
static void desconectar_do_wifi();

/******************************************************************************
 * Função que verifique periodicamente os status do wifi e tarfas relacionadas.
 *****************************************************************************/
static void wifi_app_control(void *pvParameters);

/******************************************************************************
 * Inicia tarefa de cntole do Wi-Fi .
 *****************************************************************************/
static void wifi_app_init();