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
#include "driver/gpio.h"

// Biblioteca do MQTT
#include "mqtt_client.h"
#include "app_main.h"

#ifdef DEBUG_UDP
#include "../../LOG UDP/LOG_UDP.c"
#endif

/******************************************************************************
 *  Protótipos das funções 
 * ***************************************************************************/
static bool recebe_config();
static void task_controller(void *pv);
static void mqtt_recebe_config(char *data, int id);
static void mqtt_recebe_realtime(char *data, int id);

/******************************************************************************
 *  Funções
 * ***************************************************************************/
static void mqtt_recebe_config(char *data, int id)
{
    ESP_LOGE(LOG_MQTT, "Parse JSON error");
}

static void mqtt_recebe_realtime(char *data, int id)
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

    param = cJSON_GetObjectItem(realtime, "description");
    if (cJSON_IsString(param))
    {
        if (strlen(param->valuestring) < 17)
            sprintf(device_temp.description, "%s", param->valuestring);
    }

    // procura no array, se está cadsatrado,
    int found = -1;
    for (int i = 0; i < MAX_DEVICES_LENTH; i++)
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
        for (int i = 0; i < MAX_DEVICES_LENTH; i++)
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
        cJSON_Delete(realtime); // Liberar a memoria teporário do json.

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
    esp_mqtt_client_handle_t client_ev = event->client;
    int msg_id;
    char buffer[64];
    int id = -1;
    char *p = NULL;

    // your_context_t *context = event->context;
    switch (event->event_id)
    {
    case MQTT_EVENT_CONNECTED:
        status_mqtt = MQTT_CONECTADO;

        for (int i = 0; i < MAX_DEVICES_LENTH; i++)
        {
            if (habilita_devices_sub_id[i] != -1)
            {
                // Assinar o tópico para receber configurações
                sprintf(buffer, "device/%d/realtime", habilita_devices_sub_id[i]);
                ESP_LOGI(LOG_MQTT, "Assinando o topico: '%s'", buffer);
                msg_id = esp_mqtt_client_subscribe(client_ev, (char *)&buffer, 1);
                ESP_LOGI(LOG_MQTT, "Subscribe successful topic, msg_id=%d", msg_id);
            }
        }

        // Assinando o tópico de configuração
        sprintf(buffer, "device/%d/config", config_mqtt.id);
        ESP_LOGI(LOG_MQTT, "Assinando o topico: '%s'", buffer);
        msg_id = esp_mqtt_client_subscribe(client_ev, (char *)&buffer, 1);
        ESP_LOGI(LOG_MQTT, "Subscribe successful topic, msg_id=%d", msg_id);

        // sprintf(buffer, "/lampadas/%d/mode", config_mqtt.id);
        // ESP_LOGI(LOG_MQTT, "Assinando o topico: '%s'", buffer);
        // msg_id = esp_mqtt_client_subscribe(client_ev, (char *)&buffer, 1);
        // ESP_LOGI(LOG_MQTT, "Subscribe successful mode, msg_id=%d", msg_id);

        modo_led_status(LED_STATUS_LIGADO);

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
                for (int i = 0; i < MAX_DEVICES_LENTH; i++)
                {
                    if (id == config_mqtt.id)
                        continue;
                    if (habilita_devices_sub_id[i] == id)
                    {
                        found = 1;
                        break;
                    }
                }

                if ((found) && (strstr(event->topic, "/realtime") != NULL))
                {
                    mqtt_recebe_realtime((char *)event->data, id);
                }
                else if ((id == config_mqtt.id) && (strstr(event->topic, "/config") != NULL))
                {
                    mqtt_recebe_config((char *)event->data, id);
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

static void mqtt_app_start(void)
{
    ESP_LOGW(LOG_MQTT, "Configuração do dispositivo");
    ESP_LOGW(LOG_MQTT, "uri:'%s'", config_mqtt.uri);
    ESP_LOGW(LOG_MQTT, "port:'%d'", config_mqtt.port);
    ESP_LOGW(LOG_MQTT, "user:'%s'", config_mqtt.user);
    ESP_LOGW(LOG_MQTT, "pass:'%s'", config_mqtt.pass);
    ESP_LOGW(LOG_MQTT, "id:'%d'", config_mqtt.id);

    status_mqtt = MQTT_DESCONECTADO;

    esp_mqtt_client_config_t mqtt_cfg = {
        .uri = (char *)&config_mqtt.uri,
        .port = config_mqtt.port,
        .username = (char *)&config_mqtt.user,
        .password = (char *)&config_mqtt.pass};

    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);
    esp_mqtt_client_start(client);
}

static bool recebe_config()
{
    device_t config_temp;
    if (xQueueReceive(device_queue, &config_temp, 0) == pdTRUE) {
        // Transfere a configuração para a estrutura de controle
        memcpy(&device, &config_temp, sizeof(device_t));
        ESP_LOGI(LOG_MQTT, "Recebeu configuracao");
        return true;
    }
    return false;
}

static void inicia_hardware()
{
    gpio_pad_select_gpio(BUTTON_MAIS);
    gpio_set_direction(BUTTON_MAIS, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BUTTON_MAIS, GPIO_PULLUP_ONLY);
    // gpio_pad_select_gpio(BUTTON_MENOS);
    // gpio_set_direction(BUTTON_MENOS, GPIO_MODE_INPUT);
    // gpio_set_pull_mode(BUTTON_MENOS, GPIO_PULLUP_ONLY);
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

// static bool monitora_botao_menos()
// {
//     static bool status_anteror = 0;
//     bool status = 0;
//     status = gpio_get_level(BUTTON_MENOS);
//     if ((!status) && (status_anteror))
//     {
//         status_anteror = status;
//         return true;
//     }
//     status_anteror = status;
//     return false;
// }

void publicar_realtime()
{
    int msg_id;
    static char topic[64];

    static char data[300];
    char *p = NULL;

    if (status_mqtt == MQTT_CONECTADO)
    {
        // Montando o json
        p = (char *)&data;
        p += sprintf(p, "{\"id\": %d,", config_mqtt.id);
        p += sprintf(p, "\"temp\": %.1f,", device.temperature);
        p += sprintf(p, "\"long\": \"%s\",", device.lg);
        p += sprintf(p, "\"lat\": \"%s\",", device.lt);
        p += sprintf(p, "\"description\":\"%s\",", device.description);
        p += sprintf(p, "\"period\": %lld}", (long long int)device.period);

        estrobo_led_status_ms(1000);
        sprintf(topic, "device/%d/realtime", config_mqtt.id);
        msg_id = esp_mqtt_client_publish(client, topic, data, 0, 1, 1);
        ESP_LOGI(LOG_MQTT, "sent publish successful, msg_id=%d", msg_id);
    }
    else
    {
        ESP_LOGW(LOG_MQTT, "MQTT ainda não está conectado!");
    }
}

static void atualiza_display_oled(int *tela)
{
    char lineChar[35];
    int length = 0;
    static int tela_auxiliar = 0xff;
    int id = -1;
    device_t *temp;
    int pos_array = 0;

    temp = &device;
    id = -1;

    if (*tela > MAX_DEVICES_LENTH)
        *tela = 0;
    else if (*tela < 0)
        *tela = MAX_DEVICES_LENTH;

    // Analisa qual das memorias está habilitada
    if ((*tela > 0) && (*tela <= MAX_DEVICES_LENTH))
    {
        do
        {
            pos_array = *tela - 1;
            if (device_sub[pos_array].act)
            {
                temp = &device_sub[pos_array].device;
                id = device_sub[pos_array].id;
                break;
            }
            else
                *tela = *tela + 1;
        } while (*tela <= MAX_DEVICES_LENTH);

        if (*tela > MAX_DEVICES_LENTH)
        {
            *tela = 0;
            temp = &device;
        }
    }
    else
    {
        *tela = 0;
    }

    if (*tela != tela_auxiliar)
    {
        tela_auxiliar = *tela;
        ssd1306_clear_screen(&SSD1306, false);
    }

    //ssd1306_clear_line(&SSD1306, 0, false);
    if (id == -1)
    {
        length = sprintf(lineChar, "LEITURA LOCAL");
        ssd1306_display_text(&SSD1306, 0, lineChar, length, false);
    }
    else
    {
        length = sprintf(lineChar, "ID: %02d      ", id);
        ssd1306_display_text(&SSD1306, 0, lineChar, length, false);
    }

    //ssd1306_clear_line(&SSD1306, 2, false);
    length = sprintf(lineChar, "%s", temp->description);
    ssd1306_display_text(&SSD1306, 2, lineChar, length, false);

    //ssd1306_clear_line(&SSD1306, 4, false);
    length = sprintf(lineChar, "long: %s   ", temp->lg);
    ssd1306_display_text(&SSD1306, 4, lineChar, length, false);

    //ssd1306_clear_line(&SSD1306, 5, false);
    length = sprintf(lineChar, "lat : %s   ", temp->lt);
    ssd1306_display_text(&SSD1306, 5, lineChar, length, false);

    //ssd1306_clear_line(&SSD1306, 6, false);
    length = sprintf(lineChar, "Temp: %.1f~C   ", temp->temperature);
    ssd1306_display_text(&SSD1306, 6, lineChar, length, false);

    //ssd1306_clear_line(&SSD1306, 6, false);
    length = sprintf(lineChar, "Periodo: %05lld s ", temp->period);
    ssd1306_display_text(&SSD1306, 7, lineChar, length, false);
}

static void  lcd_splash_image_128_64(uint8_t *img) {
    for (int i = 0; i < 8; i++)
    {
       i2c_display_image(&SSD1306, 7-i, 0, img, 128);
       img = img + 128;
    }
}

static void tela_inicial() {     
    lcd_splash_image_128_64(&image_128_64_temperatura_mqtt);
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    lcd_splash_image_128_64(&image_128_64_eng_computacao);
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    lcd_splash_image_128_64(&image_128_64_satc);
    vTaskDelay(2000 / portTICK_PERIOD_MS);
}

static void task_controller(void *pv)
{
    int tela = 0;
    bool btn_mais = 0;
    bool btn_menos = 0;

    // Inicia periféricos
    inicia_hardware();
    ssd1306_ini();

    for (int i = 0; i < MAX_DEVICES_LENTH; i++)
    {
        memset(&device_sub[i].device, 0, sizeof(device_t));
    }
    
    tela_inicial();

    uint64_t timer = esp_timer_get_time();
    uint64_t timer_timeout = timer;
    while (1)
    {
        analog_loop();
        device.temperature = read_temperature();

        recebe_config();

        btn_mais = monitora_botao_mais();
        if (btn_mais)
        {
            tela++;
            ESP_LOGI(LOG_MQTT, "btn_mais");
            estrobo_led_status_ms(100);
        }

        atualiza_display_oled(&tela);

        // Periodicidade da ublicação
        timer = esp_timer_get_time(); // #define millis esp_timer_get_time
        if ((status_mqtt == MQTT_CONECTADO) && (timer >= timer_timeout))
        {
            timer_timeout = timer + (device.period * 1000 * 1000);
            publicar_realtime();
        }

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }

    vTaskDelete(NULL);
}

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    vTaskDelay(2000 / portTICK_PERIOD_MS);
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
