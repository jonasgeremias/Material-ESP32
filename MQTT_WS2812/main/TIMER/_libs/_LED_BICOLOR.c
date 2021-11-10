#ifndef __LED_BICOLOR__
   #ifndef LED_BICOLOR_BASE_TEMPO_MS
      #error "defina a base de tempo do timer para LED_BICOLOR_BASE_TEMPO_MS."
   #endif
   #ifndef LED_VD
      #error "defina o pino do LED_VD."
   #endif
   #ifndef LED_VM
      #error "defina o pino do LED_VM."
   #endif

uint16_t pisca_led_status_timeout = 0;
uint16_t led_status_anterior = 0xffff;
uint16_t tempo_estrobo_status = 0;
uint16_t tempo_estrobo_erro = 0;
uint16_t troca_modo_led_status_timeout = 0;

static void IRAM_ATTR led_bicolor_timer() {
   if (pisca_led_status_timeout) pisca_led_status_timeout--;
   if (tempo_estrobo_status) tempo_estrobo_status--;
   if (troca_modo_led_status_timeout) troca_modo_led_status_timeout--;
   if (tempo_estrobo_erro) tempo_estrobo_erro--;
}

static void led_bicolor_configura_pinos() {
   gpio_pad_select_gpio(LED_VD);
   gpio_set_direction(LED_VD, GPIO_MODE_OUTPUT);
   gpio_pad_select_gpio(LED_VM);
   gpio_set_direction(LED_VM, GPIO_MODE_OUTPUT);
   gpio_set_level(LED_VD, 0);
   gpio_set_level(LED_VM, 0);
}

typedef enum modo_led_t {
   LED_DESLIGADO,
   LED_VD_LIGADO,
   LED_VM_LIGADO,
   LED_VD_PISCA_LENTO,
   LED_VM_PISCA_LENTO,
   LED_VD_PISCA_RAPIDO,
   LED_VM_PISCA_RAPIDO,
   LED_VD_ESTROBO_RAPIDO_025_ON_1_OFF,
   LED_VM_ESTROBO_RAPIDO_025_ON_1_OFF,
   LED_VD_ESTROBO_LENTO_010_ON_050_OFF,
   LED_VM_ESTROBO_LENTO_010_ON_050_OFF,
   LED_MAX
} modo_led_t;

// Saidas LEDs ----------------------------------------------------------------
static void controle_led_status(modo_led_t modo) {
   static bool s_led_vd = 0, s_led_vm = 0;
   static modo_led_t led_status_anterior = 0;

   // Se mudar de status tem que atualizar na hora
   if (modo != led_status_anterior) {
      pisca_led_status_timeout = 0;
      troca_modo_led_status_timeout = 0;
      led_status_anterior = modo;
   }

   switch (modo) {
      case LED_DESLIGADO:
         s_led_vd = 0;
         s_led_vm = 0;
         break;
      case LED_VD_LIGADO:
         s_led_vd = 1;
         s_led_vm = 0;
         break;
      case LED_VM_LIGADO:
         s_led_vd = 0;
         s_led_vm = 1;
         break;
      case LED_VD_PISCA_LENTO:
         if (!pisca_led_status_timeout) {
            pisca_led_status_timeout = (350 / LED_BICOLOR_BASE_TEMPO_MS);
            s_led_vd = !s_led_vd;
            s_led_vm = 0;
         }
         break;
      case LED_VM_PISCA_LENTO:
         if (!pisca_led_status_timeout) {
            pisca_led_status_timeout = (350 / LED_BICOLOR_BASE_TEMPO_MS);
            s_led_vm = !s_led_vm;
            s_led_vd = 0;
         }
         break;
      case LED_VD_PISCA_RAPIDO:
         if (!pisca_led_status_timeout) {
            pisca_led_status_timeout = (50 / LED_BICOLOR_BASE_TEMPO_MS);
            s_led_vd = !s_led_vd;
            s_led_vm = 0;
         }
         break;
      case LED_VM_PISCA_RAPIDO:
         if (!pisca_led_status_timeout) {
            pisca_led_status_timeout = (50 / LED_BICOLOR_BASE_TEMPO_MS);
            s_led_vm = !s_led_vm;
            s_led_vd = 0;
         }
         break;
      case LED_VD_ESTROBO_RAPIDO_025_ON_1_OFF:
         if (!troca_modo_led_status_timeout || (troca_modo_led_status_timeout >= (1000 / LED_BICOLOR_BASE_TEMPO_MS))) troca_modo_led_status_timeout = (1000 / LED_BICOLOR_BASE_TEMPO_MS);
         if (troca_modo_led_status_timeout <= (250 / LED_BICOLOR_BASE_TEMPO_MS)) {
            if (!pisca_led_status_timeout) {
               pisca_led_status_timeout = (50 / LED_BICOLOR_BASE_TEMPO_MS);
               s_led_vd = !s_led_vd;
               s_led_vm = 0;
            }
         }
         else {
            s_led_vd = 0;
            s_led_vm = 0;
         }
         break;
      case LED_VM_ESTROBO_RAPIDO_025_ON_1_OFF:
         if (!troca_modo_led_status_timeout || (troca_modo_led_status_timeout >= (1000 / LED_BICOLOR_BASE_TEMPO_MS))) troca_modo_led_status_timeout = (1000 / LED_BICOLOR_BASE_TEMPO_MS);
         if (troca_modo_led_status_timeout <= (250 / LED_BICOLOR_BASE_TEMPO_MS)) {
            if (!pisca_led_status_timeout) {
               pisca_led_status_timeout = (50 / LED_BICOLOR_BASE_TEMPO_MS);
               s_led_vm = !s_led_vm;
               s_led_vd = 0;
            }
         }
         else {
            s_led_vd = 0;
            s_led_vm = 0;
         }
         break;
      case LED_VD_ESTROBO_LENTO_010_ON_050_OFF:
         if (!troca_modo_led_status_timeout || (troca_modo_led_status_timeout >= (500 / LED_BICOLOR_BASE_TEMPO_MS))) troca_modo_led_status_timeout = (500 / LED_BICOLOR_BASE_TEMPO_MS);
         if (troca_modo_led_status_timeout <= (100 / LED_BICOLOR_BASE_TEMPO_MS)) {
            if (!pisca_led_status_timeout) {
               pisca_led_status_timeout = (100 / LED_BICOLOR_BASE_TEMPO_MS);
               s_led_vd = !s_led_vd;
               s_led_vm = 0;
            }
         }
         else {
            s_led_vd = 0;
            s_led_vm = 0;
         }
         break;
      case LED_VM_ESTROBO_LENTO_010_ON_050_OFF:
         if (!troca_modo_led_status_timeout || (troca_modo_led_status_timeout >= (500 / LED_BICOLOR_BASE_TEMPO_MS))) troca_modo_led_status_timeout = (500 / LED_BICOLOR_BASE_TEMPO_MS);
         if (troca_modo_led_status_timeout <= (100 / LED_BICOLOR_BASE_TEMPO_MS)) {
            if (!pisca_led_status_timeout) {
               pisca_led_status_timeout = (100 / LED_BICOLOR_BASE_TEMPO_MS);
               s_led_vm = !s_led_vm;
               s_led_vd = 0;
            }
         }
         else {
            s_led_vd = 0;
            s_led_vm = 0;
         }
         break;
      default:
         s_led_vd = 0;
         s_led_vm = 0;
         break;
   }

   gpio_set_level(LED_VD, s_led_vd);
   gpio_set_level(LED_VM, s_led_vm);
}
#endif
#define __LED_BICOLOR__