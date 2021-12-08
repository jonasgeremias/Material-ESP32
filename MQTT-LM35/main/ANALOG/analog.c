/******************************************************************************
 *  Definição de analogica 
 * ***************************************************************************/
#include "driver/adc.h"
#include "esp_adc_cal.h"

#define ADC_DEFAULT_VREF 1100
#define ADC_NO_OF_SAMPLES 20
#define ADC_SAMPLES_TIMER_MS 10 // = 20 * 10 ms = 200 ms.

#ifndef PIN_ANALOG
#define PIN_ANALOG ADC1_CHANNEL_4
#endif

const char *LOG_ANALOG = "log_analog";
static esp_adc_cal_characteristics_t *adc_chars;
static const adc_channel_t adc_channel = PIN_ANALOG; // GPIO34 if ADC1
static const adc_bits_width_t adc_width = ADC_WIDTH_BIT_12;
static const adc_atten_t adc_atten = ADC_ATTEN_DB_0;
static const adc_unit_t adc_unit = ADC_UNIT_1;
static volatile float temperature = 0;

static void check_efuse(void)
{
    // Check if TP is burned into eFuse
    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_TP) == ESP_OK)
    {
        ESP_LOGI(LOG_ANALOG, "eFuse Two Point: Supported\n");
    }
    else
    {
        ESP_LOGI(LOG_ANALOG, "eFuse Two Point: NOT supported\n");
    }
    // Check Vref is burned into eFuse
    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_VREF) == ESP_OK)
    {
        ESP_LOGI(LOG_ANALOG, "eFuse Vref: Supported\n");
    }
    else
    {
        ESP_LOGI(LOG_ANALOG, "eFuse Vref: NOT supported\n");
    }
}

static void analog_loop()
{
    static uint32_t adc_reading = 0;
    static int samples_count = 0;
    uint64_t timer = 0;
    static bool adc_init = 0;
    static uint64_t timer_timeout = 0;

    // Para periodicidade do realtime.
    timer = esp_timer_get_time();
    if (timer >= timer_timeout)
    {
        timer_timeout = timer + (ADC_SAMPLES_TIMER_MS * 1000);
        do
        {
            // Multisampling
            if (adc_unit == ADC_UNIT_1)
            {
                adc_reading += adc1_get_raw((adc1_channel_t)adc_channel);
            }
            else
            {
                int raw;
                adc2_get_raw((adc2_channel_t)adc_channel, adc_width, &raw);
                adc_reading += raw;
            }

            if (++samples_count >= ADC_NO_OF_SAMPLES)
            {
                adc_reading /= ADC_NO_OF_SAMPLES;
                // Convert adc_reading to voltage in mV
                uint32_t voltage = esp_adc_cal_raw_to_voltage(adc_reading, adc_chars);
                temperature = (float)voltage / 10; // 0.01;
                adc_reading = 0;
                samples_count = 0;
                adc_init = 1;
            }
        } while (!adc_init);
    }
}

float read_temperature()
{
    return temperature;
}

static void analog_init(void)
{
    check_efuse();

    gpio_set_pull_mode(PIN_ANALOG_HARDWARE, GPIO_PULLDOWN_ONLY);

    // Configure ADC
    adc1_config_width(adc_width);
    adc1_config_channel_atten(adc_channel, adc_atten);

    // Characterize ADC
    adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));

    /*esp_adc_cal_value_t val_type = */
    esp_adc_cal_characterize(adc_unit, adc_atten, adc_width, ADC_DEFAULT_VREF, adc_chars);
}
