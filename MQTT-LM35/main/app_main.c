#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
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
#include "driver/gpio.h"

// Biblioteca do MQTT
#include "mqtt_client.h"
#include "app_main.h"
#include "NVS/nvs_app.c"

#ifdef DEBUG_UDP
#include "../../LOG UDP/LOG_UDP.c"
#endif

/******************************************************************************
 *  Protótipos das funções 
 * ***************************************************************************/
// MQTT
static void log_error_if_nonzero(const char *message, int error_code);
static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event);
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);
static void mqtt_app_start();
static void mqtt_app_unsubscribe();
static void mqtt_app_subscribe_init();
static void mqtt_app_loop();
static void mqtt_app_publish_realtime();
static void mqtt_app_recebe_config(char *data, int id);
static void mqtt_app_recebe_realtime(char *data, int id);
// Aplicação
static void tela_inicial();
static void lcd_splash_image_128_64(uint8_t *img);
static void atualiza_display_oled(int *tela);
static void inicia_hardware();
static bool app_recebe_config(); // Recebe configuração via queue e salva na NVS.
static bool monitora_botao_mais();
static void task_controller(void *pv);

/******************************************************************************
 *  Funções
 * ***************************************************************************/
static void mqtt_app_recebe_config(char *data, int id)
{
    device_t device_temp;
    memset(&device_temp, 0, sizeof(device_t));
    cJSON *param = NULL;     // Para ler o parametro da config
    cJSON *sub_param = NULL; // Para ler o parametro da config
    cJSON *config = cJSON_Parse(data);

    if (config == NULL)
        goto erro;

    // Lendo a longitude
    param = cJSON_GetObjectItem(config, "long");
    if (cJSON_IsString(param))
    {
        if (strlen(param->valuestring) < 13)
            sprintf(device_temp.lg, "%s", param->valuestring);
    }

    // Lendo a latitude
    param = cJSON_GetObjectItem(config, "lat");
    if (cJSON_IsString(param))
    {
        if (strlen(param->valuestring) < 13)
            sprintf(device_temp.lt, "%s", param->valuestring);
    }

    // Lendo o periodo
    param = cJSON_GetObjectItem(config, "period");
    if (cJSON_IsNumber(param))
    {
        if (param->valueint >= 0)
        {
            device_temp.period = param->valueint;
        }
    }

    // Lendo o ID
    param = cJSON_GetObjectItem(config, "id");
    if (cJSON_IsNumber(param))
    {
        if (param->valueint > 0)
        {
            device_temp.id = param->valueint;
        }
    }

    // Lendo a descrição
    param = cJSON_GetObjectItem(config, "description");
    if (cJSON_IsString(param))
    {
        for (uint8_t i = 0; i < 16; i++)
        {
            if (param->valuestring[i] == 0) //Se nulo, coloca espaços.
            {
                device_temp.description[i] = ' ';
            }

            device_temp.description[i] = param->valuestring[i];
            device_temp.description[i + 1] = 0; // Garante o nulo no final
        }
    }

    param = cJSON_GetObjectItem(config, "subscribe_ids");
    if (cJSON_IsArray(param))
    {
        for (int i = 0; i < MAX_DEVICES_LENGTH; i++)
        {
            // Se a posição do array for nula, o array veio menor, atribui -1 (desabilitado).
            sub_param = cJSON_GetArrayItem(param, i);
            if (sub_param != NULL)
            {
                device_temp.subscribe_ids[i] = sub_param->valueint;
            }
            else
                device_temp.subscribe_ids[i] = -1;
        }
    }

    // Envia para a tarefa de controle dos LEDs
    if (xQueueSend(device_queue, &device_temp, portMAX_DELAY) != pdTRUE)
    {
        ESP_LOGE(LOG_MQTT, "erro ao enviar as configuracoes");
    }
    else
    {
        ESP_LOGI(LOG_MQTT, "Configuracoes enviadas.");
    }

    if (config != NULL)
        cJSON_Delete(config); // Liberar a memoria temporária do json.

    return;
erro:
    ESP_LOGE(LOG_MQTT, "Parse JSON error");
}

static void mqtt_app_recebe_realtime(char *data, int id)
{
    device_t device_temp;
    memset(&device_temp, 0, sizeof(device_t));

    cJSON *param = NULL; // para ler o parametro
    cJSON *realtime = cJSON_Parse(data);

    if (realtime == NULL)
        goto erro;

    // ESP_LOGI(LOG_MQTT, "realtime: %s", cJSON_Print(realtime));
    param = cJSON_GetObjectItem(realtime, "long");
    if (cJSON_IsString(param))
    {
        if (strlen(param->valuestring) < 13)
            sprintf(device_temp.lg, "%s", param->valuestring);
    }

    param = cJSON_GetObjectItem(realtime, "lat");
    if (cJSON_IsString(param))
    {
        if (strlen(param->valuestring) < 13)
            sprintf(device_temp.lt, "%s", param->valuestring);
    }

    param = cJSON_GetObjectItem(realtime, "temp");
    if (cJSON_IsNumber(param))
    {
        device_temp.temperature = param->valuedouble;
    }

    param = cJSON_GetObjectItem(realtime, "period");
    if (cJSON_IsNumber(param))
    {
        device_temp.period = param->valueint;
    }

    // Lendo a descrição
    param = cJSON_GetObjectItem(realtime, "description");
    if (cJSON_IsString(param))
    {
        for (uint8_t i = 0; i < 16; i++)
        {
            if (param->valuestring[i] == 0) //Se nulo, coloca espaços.
            {
                device_temp.description[i] = ' ';
            }

            device_temp.description[i] = param->valuestring[i];
            device_temp.description[i + 1] = 0; // Garante o nulo no final
        }
    }

    char mac[4] = "";
    param = cJSON_GetObjectItem(realtime, "mac");
    if (cJSON_IsString(param))
    {
        for (uint8_t i = 0, j = 0; j < 6; i += 2, j++)
        {
            mac[0] = param->valuestring[i];
            mac[1] = param->valuestring[i + 1];
            mac[2] = 0;
            device_temp.mac_address[j] = (uint8_t)strtol(mac, NULL, 16);
        }
    }

    // procura no array, se está cadsatrado,
    int found = -1;
    for (int i = 0; i < MAX_DEVICES_LENGTH; i++)
    {
        if (device_sub[i].id == id)
        {
            found = i;
        }
    }

    // Se encontrou
    if (found != -1)
    {
        memcpy(&device_sub[found].device, &device_temp, sizeof(device_t));
        device_sub[found].act = 1;
        device_sub[found].id = id;
    }
    else
    {
        // Procurando lugar inativo
        for (int i = 0; i < MAX_DEVICES_LENGTH; i++)
        {
            if (device_sub[i].act == 0)
            {
                found = i;
            }
        }

        if (found != -1)
        {
            memcpy(&device_sub[found].device, &device_temp, sizeof(device_t));
            device_sub[found].act = 1;
            device_sub[found].id = id;
        }
    }

    if (realtime != NULL)
        cJSON_Delete(realtime); // Liberar a memoria temporária do json.

    return;
erro:
    ESP_LOGE(LOG_MQTT, "Parse JSON error");
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
    // esp_mqtt_client_handle_t client_ev = event->client;
    char buffer[64];
    int id = -1;

    // your_context_t *context = event->context;
    switch (event->event_id)
    {
    case MQTT_EVENT_CONNECTED:
        status_mqtt = MQTT_CONECTADO; // sinaliza status conectado.
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(LOG_MQTT, "MQTT_EVENT_DISCONNECTED");
        status_mqtt = MQTT_DESCONECTADO;
        modo_led_status(LED_STATUS_DESLIGADO);
        break;
    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(LOG_MQTT, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        // msg_id = esp_mqtt_client_publish(client_ev, "/topic/qos0", "data", 0, 0, 0);
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
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);

        id = -1;
        bool found = 0;
        if (str_transfer_from_to_begin_end(event->topic, buffer, "device/", '/') != NULL)
        {
            id = atol(buffer);
            if (id >= 0)
            {
                // Verifica se está configurado para receber destes devices.
                for (int i = 0; i < MAX_DEVICES_LENGTH; i++)
                {
                    if (id == device.id)
                        continue;
                    if (device.subscribe_ids[i] == id)
                    {
                        found = 1;
                        break;
                    }
                }

                if ((found) && (strstr(event->topic, "/realtime") != NULL))
                {
                    mqtt_app_recebe_realtime((char *)event->data, id);
                }
                else if ((id == device.id) && (strstr(event->topic, "/config") != NULL))
                {
                    mqtt_app_recebe_config((char *)event->data, id);
                }
            }
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

static void mqtt_app_start()
{
    ESP_LOGW(LOG_MQTT, "Configuração do dispositivo");
    ESP_LOGW(LOG_MQTT, "uri:'%s'", config_mqtt.uri);
    ESP_LOGW(LOG_MQTT, "port:'%d'", config_mqtt.port);
    ESP_LOGW(LOG_MQTT, "user:'%s'", config_mqtt.user);
    ESP_LOGW(LOG_MQTT, "pass:'%s'", config_mqtt.pass);
    ESP_LOGW(LOG_MQTT, "id:'%d'", device.id);

    status_mqtt = MQTT_DESCONECTADO;

    esp_mqtt_client_config_t mqtt_cfg = {
        .uri = (char *)&config_mqtt.uri,
        .port = config_mqtt.port,
        .username = (char *)&config_mqtt.user,
        .password = (char *)&config_mqtt.pass};

    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, mqtt_client);
    esp_mqtt_client_start(mqtt_client);
}

static bool app_recebe_config()
{
    device_t config_temp;
    if (xQueueReceive(device_queue, &config_temp, 0) == pdTRUE)
    {
        mqtt_app_unsubscribe(); // desassinar os tópicos antes de mudar as configurações.
        // Transfere a configuração para a estrutura de controle
        memcpy(&device, &config_temp, sizeof(device_t));
        // Grava as configurações no memoria.
        int ret = nvs_app_write_config_module(&device);

        ESP_LOGI(LOG_MQTT, "Recebeu configuracao: %d.", ret);
        return true;
    }
    return false;
}

static void inicia_hardware()
{
    gpio_pad_select_gpio(BUTTON_MAIS);
    gpio_set_direction(BUTTON_MAIS, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BUTTON_MAIS, GPIO_PULLUP_ONLY);
    analog_init(); // Inicia analogica
    estrobo_led_status_ms(100);
}

static bool monitora_botao_mais()
{
    static bool status_anteror = 0;
    bool status = 0;
    status = gpio_get_level(BUTTON_MAIS);

    if ((!status) && (status_anteror))
    {
        status_anteror = status;
        return true;
    }

    status_anteror = status;
    return false;
}

static void mqtt_app_publish_realtime()
{
    int msg_id;
    static char topic[64];

    static char data[250];
    char *p = NULL;

    if (status_mqtt == MQTT_CONECTADO)
    {
        // Montando o json
        p = (char *)&data;
        esp_wifi_get_mac(ESP_IF_WIFI_STA, (uint8_t *)&device.mac_address);
        p += sprintf(p, "{\"id\":%d,", device.id);
        p += sprintf(p, "\"mac\":\"%02X%02X%02X%02X%02X%02X\",", device.mac_address[0], device.mac_address[1], device.mac_address[2], device.mac_address[3], device.mac_address[4], device.mac_address[5]);
        p += sprintf(p, "\"temp\":%.1f,", device.temperature);
        p += sprintf(p, "\"long\":\"%s\",", device.lg);
        p += sprintf(p, "\"lat\":\"%s\",", device.lt);
        p += sprintf(p, "\"description\":\"%s\",", device.description);
        p += sprintf(p, "\"period\": %lld}", (long long int)device.period);

        estrobo_led_status_ms(750);
        sprintf(topic, "device/%d/realtime", device.id);
        msg_id = esp_mqtt_client_publish(mqtt_client, topic, data, 0, 1, 1);
        ESP_LOGI(LOG_MQTT, "sent publish successful, msg_id=%d", msg_id);
    }
    else
    {
        ESP_LOGW(LOG_MQTT, "MQTT ainda não está conectado!");
    }
}

static void atualiza_display_oled(int *tela)
{
    char lineChar[100];
    int length = 0;
    static int tela_auxiliar = 0xff;
    device_t *temp;
    int pos_array = 0;
    static bool carrega_img_realtime = 0;

    temp = &device;
    if (*tela >= MAX_DEVICES_LENGTH)
        *tela = -1;
    else if (*tela < -1)
        *tela = MAX_DEVICES_LENGTH;

    // Analisa qual das memorias está habilitada
    if ((*tela >= 0) && (*tela < MAX_DEVICES_LENGTH))
    {
        do
        {
            pos_array = *tela;
            if (device_sub[pos_array].act)
            {
                temp = &device_sub[pos_array].device;
                break;
            }
            else
                *tela = *tela + 1;
        } while (*tela <= MAX_DEVICES_LENGTH);
    }

    if (*tela >= MAX_DEVICES_LENGTH)
    {
        *tela = -1;
        temp = &device;
    }

    if (*tela != tela_auxiliar)
    {
        tela_auxiliar = *tela;
        ssd1306_clear_screen(&SSD1306, false);
    }

    // ssd1306_clear_line(&SSD1306, 0, false);
    if (*tela == -1)
    {
        if (!carrega_img_realtime) {
            lcd_splash_image_128_64(&image_128_64_realtime);
            carrega_img_realtime = 1;
        }
        
        length = sprintf(lineChar, "%s", temp->description);
        ssd1306_display_text(&SSD1306, 0, lineChar, length, false);
        
        length = sprintf(lineChar, "ID:%02d T:%05lld ", temp->id, temp->period);
        ssd1306_display_text(&SSD1306, 1, lineChar, length, false);

        length = sprintf(lineChar, "Temp:%3.1f~C   ", temp->temperature);
        ssd1306_display_text(&SSD1306, 3, lineChar, length, false);

        int progress = (float) temp->temperature * 92 / 110;
        
        // Cria a barra de progresso
        for (uint8_t i = 0; i < 92; i++)
        {
            if (i < progress) lineChar[i] = 0xff;
            else lineChar[i] = 0;
        }

        i2c_display_image(&SSD1306, 6, 26, (uint8_t*) &lineChar, 92);
    }
    else
    {   
        carrega_img_realtime = 0;

        length = sprintf(lineChar, "%s", temp->description);
        ssd1306_display_text(&SSD1306, 0, lineChar, length, false);

        length = sprintf(lineChar, "MAC:%02X%02X%02X%02X%02X%02X", temp->mac_address[0], temp->mac_address[1], temp->mac_address[2], temp->mac_address[3], temp->mac_address[4], temp->mac_address[5]);
        ssd1306_display_text(&SSD1306, 1, lineChar, length, false);

        length = sprintf(lineChar, "long: %s   ", temp->lg);
        ssd1306_display_text(&SSD1306, 2, lineChar, length, false);

        length = sprintf(lineChar, "lat : %s   ", temp->lt);
        ssd1306_display_text(&SSD1306, 3, lineChar, length, false);
        
        length = sprintf(lineChar, "Periodo: %05lld s ", temp->period);
        ssd1306_display_text(&SSD1306, 4, lineChar, length, false);

        length = sprintf(lineChar, "Temp: %.1f~C   ", temp->temperature);
        ssd1306_display_text(&SSD1306, 6, lineChar, length, false);
    }
}

static void lcd_splash_image_128_64(uint8_t *img)
{
    for (int i = 0; i < 8; i++)
    {
        i2c_display_image(&SSD1306, 7 - i, 0, img, 128);
        img = img + 128;
    }
}

static void tela_inicial()
{
    lcd_splash_image_128_64((uint8_t *)&image_128_64_temperatura_mqtt);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    lcd_splash_image_128_64((uint8_t *)&image_128_64_eng_computacao);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    lcd_splash_image_128_64((uint8_t *)&image_128_64_satc);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
}

static void mqtt_app_unsubscribe()
{
    int msg_id = 0;
    char buffer[64] = "";

    for (int i = 0; i < MAX_DEVICES_LENGTH; i++)
    {
        if (device.subscribe_ids[i] != -1)
        {
            // Assinar o tópico para receber configurações
            sprintf(buffer, "device/%d/realtime", device.subscribe_ids[i]);
            ESP_LOGI(LOG_MQTT, "Assinando o topico: '%s'", buffer);
            msg_id = esp_mqtt_client_unsubscribe(mqtt_client, (char *)&buffer);
            ESP_LOGI(LOG_MQTT, "Unsubscribe successful topic, msg_id=%d", msg_id);
        }
    }

    // Assinando o tópico de configuração
    sprintf(buffer, "device/%d/config", device.id);
    ESP_LOGI(LOG_MQTT, "Assinando o topico: '%s'", buffer);
    msg_id = esp_mqtt_client_unsubscribe(mqtt_client, (char *)&buffer);
    ESP_LOGI(LOG_MQTT, "Unsubscribe successful topic, msg_id=%d", msg_id);
}

static void mqtt_app_subscribe_init()
{
    int msg_id = 0;
    char buffer[64] = "";

    for (int i = 0; i < MAX_DEVICES_LENGTH; i++)
    {
        if (device.subscribe_ids[i] != -1)
        {
            // Assinar o tópico para receber configurações
            sprintf(buffer, "device/%d/realtime", device.subscribe_ids[i]);
            ESP_LOGI(LOG_MQTT, "Assinando o topico: '%s'", buffer);
            msg_id = esp_mqtt_client_subscribe(mqtt_client, (char *)&buffer, 1);
            ESP_LOGI(LOG_MQTT, "Subscribe successful topic, msg_id=%d", msg_id);
        }
    }

    // Assinando o tópico de configuração
    sprintf(buffer, "device/%d/config", device.id);
    ESP_LOGI(LOG_MQTT, "Assinando o topico: '%s'", buffer);
    msg_id = esp_mqtt_client_subscribe(mqtt_client, (char *)&buffer, 1);
    ESP_LOGI(LOG_MQTT, "Subscribe successful topic, msg_id=%d", msg_id);

    modo_led_status(LED_STATUS_LIGADO);
}

static void mqtt_app_loop()
{
    static bool subscribe_init = 0;
    uint64_t timer = 0;
    static uint64_t timer_timeout = 0;

    // Periodicidade da ublicação
    if (status_mqtt == MQTT_CONECTADO)
    {
        if (!subscribe_init)
        {
            subscribe_init = 1;
            mqtt_app_subscribe_init();
        }

        // Para periodicidade do realtime.
        timer = esp_timer_get_time();
        if (timer >= timer_timeout)
        {
            timer_timeout = timer + (device.period * 1000 * 1000);
            mqtt_app_publish_realtime();
        }
    }

    // Monitora o recebimento das configurações
    if (app_recebe_config())
        subscribe_init = 0; // Força assinar os tópicos novamente;
}

static void task_controller(void *pv)
{
    int tela = -1;
    bool btn_mais = 0;

    // Inicia periféricos
    inicia_hardware();
    ssd1306_ini();

    // Delay inicial da tarefa da aplicação
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    for (int i = 0; i < MAX_DEVICES_LENGTH; i++)
    {
        memset(&device_sub[i].device, 0, sizeof(device_t));
    }

    tela_inicial();

    while (1)
    {
        analog_loop();
        device.temperature = read_temperature();

        btn_mais = monitora_botao_mais();
        if (btn_mais)
        {
            tela++;
            ESP_LOGI(LOG_MQTT, "btn_mais");
            estrobo_led_status_ms(100);
        }

        atualiza_display_oled(&tela);

        mqtt_app_loop();

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }

    vTaskDelete(NULL);
}

void app_main(void)
{
    // Iniciando a memoria flash
    app_nvs_init();
    nvs_app_read_module(&device);

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
    device_queue = xQueueCreate(2, sizeof(device_t));
    init_task_led_status_controller();
    xTaskCreatePinnedToCore(task_controller, "task_controller", 15000, NULL, 1, &taskhandle_controller, 1);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(example_connect());

#ifdef DEBUG_UDP
    log_udp_init(LOG_UDP_IP, LOG_UDP_PORT, log_udp_vprintf);
#endif

    mqtt_app_start();
}
