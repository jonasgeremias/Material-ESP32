
/******************************************************************************
 * Trata Os eventos do wifi.
 *****************************************************************************/
static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
   if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
#ifdef DEBUG_WIFI
      ESP_LOGI(LOG_WIFI, "AP Start");
#endif
      xEventGroupClearBits(app_event_group, CONECTADO_NO_WIFI);
      xEventGroupClearBits(app_event_group, RECEBEU_IP_DO_ROTEADOR);
      esp_wifi_connect();
   }
   else if (event_base == WIFI_EVENT && event_id == SYSTEM_EVENT_AP_STACONNECTED) {
#ifdef DEBUG_WIFI
      ESP_LOGI(LOG_WIFI, "Disp. conectou no ESP");
      wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *) event_data;
      ESP_LOGI(LOG_WIFI, "station " MACSTR " join, AID=%d",
               MAC2STR(event->mac), event->aid);
#endif
   }
   else if (event_base == WIFI_EVENT && event_id == SYSTEM_EVENT_AP_STADISCONNECTED) { // Dispositivo desconectou do ESP
#ifdef DEBUG_WIFI
      wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *) event_data;
      ESP_LOGI(LOG_WIFI, "Disp. desconectou do ESP");
      ESP_LOGI(LOG_WIFI, MACSTR "leave, AID=%d",
               MAC2STR(event->mac), event->aid);
#endif
   }
   else if (event_base == WIFI_EVENT && event_id == SYSTEM_EVENT_STA_CONNECTED) { // Conectou no roteador
#ifdef DEBUG_WIFI
      ESP_LOGI(LOG_WIFI, "Conectou no Rot.");
#endif
      xEventGroupSetBits(app_event_group, CONECTADO_NO_WIFI);
   }
   else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) { // IPV4 ----------------------

#ifdef DEBUG_WIFI
      ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
      ESP_LOGI(LOG_WIFI, "Recebeu ip:" IPSTR, IP2STR(&event->ip_info.ip));
#endif

      xEventGroupSetBits(app_event_group, RECEBEU_IP_DO_ROTEADOR);
   }
   else if (event_base == WIFI_EVENT && event_id == SYSTEM_EVENT_STA_DISCONNECTED) { // Desconectou do Roteador
#ifdef DEBUG_WIFI
      ESP_LOGI(LOG_WIFI, "Desconectou do Rot.");
#endif
      xEventGroupClearBits(app_event_group, CONECTADO_NO_WIFI);
      xEventGroupClearBits(app_event_group, RECEBEU_IP_DO_ROTEADOR);
      esp_wifi_connect();
#ifdef DEBUG_WIFI
      ESP_LOGI(LOG_WIFI, "Tentando conectar novamente.");
#endif
   }

   else if (event_base == WIFI_EVENT && event_id == SYSTEM_EVENT_AP_STAIPASSIGNED) { // Dispositivo conectou no ESP e recebeu IP
#ifdef DEBUG_WIFI
      ESP_LOGI(LOG_WIFI, "Disp. Conectou.");
#endif
   }
   else if (event_base == WIFI_EVENT && event_id == SYSTEM_EVENT_STA_START) {
#ifdef DEBUG_WIFI
      ESP_LOGI(LOG_WIFI, "Evento STA start.");
#endif
   }
   else if (event_base == WIFI_EVENT && event_id == SYSTEM_EVENT_SCAN_DONE) {
   }
   else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_LOST_IP) {
#ifdef DEBUG_WIFI
      ESP_LOGI(LOG_WIFI, "IP ruim.");
#endif
      esp_wifi_disconnect();
   }
   else {
#ifdef DEBUG_WIFI
      ESP_LOGE(LOG_WIFI, "Evento Inválido ->%d, %d", (int) event_base, event_id);
#endif
   }
}

/******************************************************************************
 * Configura o ponto de acesso.
 *****************************************************************************/
static esp_err_t wifi_ap_config() {
   wifi_config_t wifi_conf = {
       .ap = {
           .max_connection = WIFI_AP_MAX_CON,
           .authmode = WIFI_AUTH_WPA2_PSK,
           .ssid = SSID_WIFI_AP,
           .password = PASS_WIFI_AP}};

   int len = strlen((char *) wifi_conf.ap.ssid);
   wifi_conf.ap.ssid_len = len;
   if (len == 0) wifi_conf.ap.authmode = WIFI_AUTH_OPEN;

   return esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_conf);
}

/******************************************************************************
 * Configura o IP do ponto de acesso como fixo.
 *****************************************************************************/
static void config_tcpip_ap_fix() {
   esp_netif_ip_info_t net;
   esp_netif_dns_info_t dns;

   // Configura de IP fixo.
   esp_netif_set_ip4_addr(&net.ip, 192, 168, 0, 1);
   esp_netif_set_ip4_addr(&net.gw, 192, 168, 0, 1);
   esp_netif_set_ip4_addr(&net.netmask, 255, 255, 255, 0);

   memset(&dns.ip, 8, sizeof(dns.ip));
   esp_netif_set_ip4_addr(&dns.ip.u_addr.ip4, 4, 4, 4, 4);

   esp_netif_dhcps_stop(app_ap_netif);
   esp_netif_set_ip_info(app_ap_netif, &net);
   esp_netif_set_dns_info(app_ap_netif, ESP_NETIF_DNS_MAIN, &dns);

   esp_netif_dhcps_start(app_ap_netif);

   esp_netif_get_dns_info(app_ap_netif, ESP_NETIF_DNS_MAIN, &dns);
#ifdef DEBUG_WIFI
   ESP_LOGI(LOG_WIFI, "ESP_NETIF_DNS_MAIN->%d", dns.ip.u_addr.ip4.addr);
#endif
}

/******************************************************************************
 * Configura o login e senha para conectar o ESP (STA) em um Wi-Fi disponivel.
 *****************************************************************************/
static esp_err_t wifi_sta_config() {
   wifi_config_t wifi_conf_sta = {
      .sta = {
         .scan_method = WIFI_FAST_SCAN,
         .sort_method = WIFI_CONNECT_AP_BY_SIGNAL,
         .listen_interval = WIFI_PS_NONE,
         .ssid = SSID_WIFI_STA,
         .password = PASS_WIFI_STA}};

#ifdef DEBUG_WIFI
   ESP_LOGI(LOG_WIFI, "wifi_sta_config");
#endif

   // Verifica se o serviço de cliente DHCP está ativado.
   esp_netif_dhcp_status_t status = 0;
   esp_netif_dhcpc_get_status(app_sta_netif, &status);
#ifdef DEBUG_WIFI
   ESP_LOGI(LOG_WIFI, "Status DHCPC:%d", status);
#endif

   if (status != ESP_NETIF_DHCP_STARTED) {
      esp_netif_dhcpc_start(app_sta_netif);
   }

   return esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_conf_sta);
}

static void add_mdns_services() {
   // Configurando o hostname da aplicação
   char hostname[60];
   sprintf((char *) hostname, "%s", SSID_WIFI_AP);

   //initialize mDNS service
   esp_err_t err = mdns_init();
   if (err) {
      printf("MDNS Init failed: %d\n", err);
      return;
   }

   // Seta o hostname
   mdns_hostname_set(hostname);
   mdns_instance_name_set(hostname);
   mdns_service_add(NULL, "_http", "_tcp", 80, NULL, 0);
   mdns_service_instance_name_set("_http", "_tcp", "OTA");
   esp_netif_set_hostname(app_sta_netif, hostname);
   esp_netif_set_hostname(app_ap_netif, hostname);
}

static void app_netif_init() {
   ESP_ERROR_CHECK(esp_netif_init());
   ESP_ERROR_CHECK(esp_event_loop_create_default());

   // Criar a camada física
   app_sta_netif = esp_netif_create_default_wifi_sta();
   app_ap_netif = esp_netif_create_default_wifi_ap();
   assert(app_sta_netif);
   assert(app_ap_netif);

   // Configuração de hostname e serviços.
   add_mdns_services();

   ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
   ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));

   // Inicializa driver TCP/IP
   config_tcpip_ap_fix();
}

/******************************************************************************
 * Inicia o modulo Wi-Fi do ESP no modo AP-STA.
 *****************************************************************************/
static void wifi_init() {
   app_netif_init();

   // Inicicou a aprte de camada netif, agora em relação ao wifi
   wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
   ESP_ERROR_CHECK(esp_wifi_init(&cfg));

#ifdef DEBUG_WIFI
   ESP_LOGI(LOG_WIFI, "APP config");
#endif

   ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
   wifi_ap_config();
   wifi_sta_config();

   // Configurações Gerais de wifi
   ESP_ERROR_CHECK(esp_wifi_set_protocol(ESP_IF_WIFI_STA, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N));
   ESP_ERROR_CHECK(esp_wifi_set_protocol(ESP_IF_WIFI_AP, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N));
   ESP_ERROR_CHECK(esp_wifi_set_bandwidth(ESP_IF_WIFI_STA, WIFI_BW_HT20));
   ESP_ERROR_CHECK(esp_wifi_set_bandwidth(ESP_IF_WIFI_AP, WIFI_BW_HT20));
   ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
   ESP_ERROR_CHECK(esp_wifi_start());
}

/******************************************************************************
 * Função para desconectar do Wi-Fi e desabilitar a reconex?o.
 *****************************************************************************/
static void desconectar_do_wifi() {
   esp_wifi_disconnect();
}

static void wifi_app_control(void *pvParameters) {
   wifi_init(); // Inicia o Wi-Fi
   esp_wifi_get_mac(ESP_IF_WIFI_STA, mac_address);

#ifdef DEBUG_WIFI
   ESP_LOGI(LOG_WIFI, "mac:%02X:%02X:%02X:%02X:%02X:%02X", mac_address[0], mac_address[1], mac_address[2], mac_address[3], mac_address[4], mac_address[5]);
#endif
   start_webserver();
   while (1) {
      // Inserir Código do Wifi manager
      if (esp_restart_now) {
         vTaskDelay(1500 / portTICK_PERIOD_MS);
         esp_restart_now = 0;
         desconectar_do_wifi();
         esp_wifi_stop();
         // fflush(stdout);
         esp_restart();
      }

      vTaskDelay(1000 / portTICK_PERIOD_MS);
   }
}

// Inicia tarefa de cntole do Wi-Fi .
static void wifi_app_init() {
   xTaskCreatePinnedToCore(wifi_app_control, "wifi_app_control", 3500, NULL, 1, &taskhandle_wifi_app_control, 0);
}