#ifndef __LED_BICOLOR__
   #define LED_STATUS_BASE_TEMPO_MS 10

   #ifndef LED_STATUS
      #error "defina o pino do LED_STATUS."
   #endif


typedef enum modo_led_t {
   LED_STATUS_DESLIGADO = 0,
   LED_STATUS_LIGADO,
   LED_STATUS_PISCA_LENTO,
   LED_STATUS_PISCA_RAPIDO,
   LED_STATUS_ESTROBO_RAPIDO_025_ON_1_OFF,
   LED_STATUS_ESTROBO_LENTO_010_ON_050_OFF,
   LED_MAX
} modo_led_t;

static volatile uint16_t pisca_led_status_timeout = 0;
static volatile uint16_t tempo_estrobo_status = 0;
static volatile uint16_t troca_modo_led_status_timeout = 0;
static TaskHandle_t taskhandle_led_status_controller; // handle da tarefa de contrle dos leds.
static QueueHandle_t led_status_queue; 
static volatile int modo_led = LED_STATUS_DESLIGADO;

// configurar o LED como saida
static void led_bicolor_configura_pinos() {
   gpio_pad_select_gpio(LED_STATUS);
   gpio_set_direction(LED_STATUS, GPIO_MODE_OUTPUT);
   gpio_set_level(LED_STATUS, 0);
}

// Saidas LEDs ----------------------------------------------------------------
static void controle_led_status(modo_led_t modo_atual) {
   static bool s_led = 0;
   static modo_led_t led_status_anterior = 0;

   // Se mudar de status tem que atualizar na hora
   if (modo_atual != led_status_anterior) {
      pisca_led_status_timeout = 0;
      troca_modo_led_status_timeout = 0;
      led_status_anterior = modo_atual;
   }

   switch (modo_atual) {
      case LED_STATUS_DESLIGADO:
         s_led = 0;
         break;
      case LED_STATUS_LIGADO:
         s_led = 1;
         break;
      case LED_STATUS_PISCA_LENTO:
         if (!pisca_led_status_timeout) {
            pisca_led_status_timeout = (350 / LED_STATUS_BASE_TEMPO_MS);
            s_led = !s_led;
         }
         break;
      case LED_STATUS_PISCA_RAPIDO:
         if (!pisca_led_status_timeout) {
            pisca_led_status_timeout = (50 / LED_STATUS_BASE_TEMPO_MS);
            s_led = !s_led;
         }
         break;
      case LED_STATUS_ESTROBO_RAPIDO_025_ON_1_OFF:
         if (!troca_modo_led_status_timeout || (troca_modo_led_status_timeout >= (1000 / LED_STATUS_BASE_TEMPO_MS))) troca_modo_led_status_timeout = (1000 / LED_STATUS_BASE_TEMPO_MS);
         if (troca_modo_led_status_timeout <= (250 / LED_STATUS_BASE_TEMPO_MS)) {
            if (!pisca_led_status_timeout) {
               pisca_led_status_timeout = (50 / LED_STATUS_BASE_TEMPO_MS);
               s_led = !s_led;
            }
         }
         else {
            s_led = 0;
         }
         break;
      case LED_STATUS_ESTROBO_LENTO_010_ON_050_OFF:
         if (!troca_modo_led_status_timeout || (troca_modo_led_status_timeout >= (500 / LED_STATUS_BASE_TEMPO_MS))) troca_modo_led_status_timeout = (500 / LED_STATUS_BASE_TEMPO_MS);
         if (troca_modo_led_status_timeout <= (100 / LED_STATUS_BASE_TEMPO_MS)) {
            if (!pisca_led_status_timeout) {
               pisca_led_status_timeout = (100 / LED_STATUS_BASE_TEMPO_MS);
               s_led = !s_led;
            }
         }
         else {
            s_led = 0;

         }
         break;
      default:
         s_led = 0;
         break;
   }

   gpio_set_level(LED_STATUS, s_led);
}

static void task_led_status() {
   led_bicolor_configura_pinos();
   int modo_temp = 0;

   while(true) {
      if (xQueueReceive(led_status_queue, &modo_temp, 0) == pdTRUE) {
         modo_led = modo_temp;
         ESP_LOGI("LED","modo:%d", modo_led);
      }

      if (pisca_led_status_timeout) pisca_led_status_timeout--;
      if (tempo_estrobo_status) tempo_estrobo_status--;
      if (troca_modo_led_status_timeout) troca_modo_led_status_timeout--;
      if (tempo_estrobo_status > 0) controle_led_status(LED_STATUS_PISCA_RAPIDO);
      else controle_led_status(modo_led);
      
      vTaskDelay(10 / portTICK_PERIOD_MS);
   }
}

void modo_led_status(int modo) {
   xQueueSend(led_status_queue, &modo, portMAX_DELAY);
}

void estrobo_led_status_ms(uint16_t ms) {
   tempo_estrobo_status = (ms / LED_STATUS_BASE_TEMPO_MS);
}

void init_task_led_status_controller() {
   led_status_queue = xQueueCreate(1, sizeof(int));
   xTaskCreatePinnedToCore(task_led_status, "task_led_status", 2048, NULL, 10, &taskhandle_led_status_controller, 0);
}
#endif
#define __LED_BICOLOR__