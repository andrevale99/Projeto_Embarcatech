#include <stdio.h>
#include <memory.h>

#include "pico/stdlib.h"

#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"
#include "hardware/timer.h"

#include "inc/ssd1306.h"

#define I2C_SDA 14
#define I2C_SCL 15

#define ONBOARD_TEMP_SENSOR_GPIO 4

#define BUTTON_A 5

#define MAX_LEN_BUFFER 16

#define RETURN_HOME_SSD(_ssd) memset(_ssd, 0, ssd1306_buffer_length)
#define DELAY_MS(ms, call_back, flag)            \
    add_alarm_in_ms(ms, call_back, NULL, false); \
    while (!flag)                                \
        tight_loop_contents();                   \
    flag = false;

//======================================
//  VARS GLOBAIS
//======================================

// Preparar área de renderização para o display
// (ssd1306_width pixels por ssd1306_n_pages páginas)
struct render_area frame_area = {
    start_column : 0,
    end_column : ssd1306_width - 1,
    start_page : 0,
    end_page : ssd1306_n_pages - 1
};

uint8_t ssd[ssd1306_buffer_length];

uint16_t TempSensor = 0;

volatile bool flag_timer = false;

char buffer[MAX_LEN_BUFFER];

volatile void (*DisplayShow)(void) = NULL;
volatile uint8_t choosePage = 0;

//======================================
//  PROTOTIPOS
//======================================

//  @brief Pagina inicial do sistema
void hello_page(void);

//  @param Pagina do sensor de temperatura
void temp_page(void);

//  @brief Funcão de configuracao do ADC
static void setup_adc(void);

//  @brief Funcão de configuracao do I2C
static void setup_i2c(void);

//  @brief Funcão de configuracao do GPIO
static void setup_gpio(void);

//  @brief Funcão de configuracao do DMA
static void setup_dma(void);

//  @brief Funcão de configuracao do DISPLAY OLED
static void setup_oled(void);

/**
 *  @brief Funcao de call_back do estouro do TIMER
 *
 *  @param id ID do processo
 *  @param *user_data variaveis do sistema (sem utilidade)
 */
int64_t alarm_callback(alarm_id_t id, __unused void *user_data);

/*
 *  @brief Funcao de interrupcao para realizar a troca
 *  de rotina ao apertar o botao
 *
 *  @param gpio o botao que foi pressionado (mapeado
 *  durante o GPIO_setup())
 *
 *  @param events O motivo que foi realizado a interrupcao
 *  (borda de descida do sinal do GPIO)
 */
void gpio_button_callback(uint gpio, uint32_t events);

//======================================
//  MAIN
//======================================
int main()
{
    stdio_init_all();

    setup_i2c();
    setup_oled();
    setup_adc();
    setup_gpio();

    DisplayShow = hello_page;

    while (true)
    {
        // adc_select_input(ONBOARD_TEMP_SENSOR_GPIO);
        // TempSensor = adc_read();

        DisplayShow();

        // DELAY_MS(1000, alarm_callback, flag_timer);
    }
}
//======================================
//  FUNCS
//======================================

void hello_page(void)
{
    RETURN_HOME_SSD(ssd);
    sprintf(&buffer[0], "BEM VINDO");
    ssd1306_draw_string(ssd, 5, 2, buffer);

    sprintf(&buffer[0], "Press the Botao");
    ssd1306_draw_string(ssd, 5, 15, buffer);

    render_on_display(ssd, &frame_area);
}

void temp_page(void)
{
    RETURN_HOME_SSD(ssd);

    sprintf(&buffer[0], "TEMP PAGE");
    ssd1306_draw_string(ssd, 5, 10, buffer);
    render_on_display(ssd, &frame_area);
}

static void setup_adc(void)
{
    adc_init();

    adc_set_temp_sensor_enabled(true);
}

static void setup_i2c(void)
{
    // Inicialização do i2c
    i2c_init(i2c1, ssd1306_i2c_clock * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
}

static void setup_gpio(void)
{
    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_pull_up(BUTTON_A);

    // Configura a interrupção no GPIO do botão para borda de descida
    gpio_set_irq_enabled_with_callback(BUTTON_A, GPIO_IRQ_EDGE_FALL,
                                       true, gpio_button_callback);
}

static void setup_dma(void)
{
}

static void setup_oled(void)
{
    // Inicia o display oled
    ssd1306_init();
    calculate_render_area_buffer_length(&frame_area);

    // coloca no home do display
    memset(ssd, 0, ssd1306_buffer_length);
    render_on_display(ssd, &frame_area);
}

int64_t alarm_callback(alarm_id_t id, __unused void *user_data)
{
    flag_timer = true;
    // Can return a value here in us to fire in the future
    return 0;
}

void gpio_button_callback(uint gpio, uint32_t events)
{
    if (gpio == BUTTON_A)
    {
        if( choosePage % 2 == 0)
            DisplayShow = temp_page;
        else 
            DisplayShow = hello_page;
        
        choosePage++;
    }
}