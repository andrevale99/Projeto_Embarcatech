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

#define RETURN_HOME_SSD(_ssd) memset(_ssd, 0, ssd1306_buffer_length)
#define DELAY_MS(ms, call_back, flag)            \
    add_alarm_in_ms(ms, call_back, NULL, false); \
    while (!timer_fired)                         \
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

volatile bool timer_fired = false;

//======================================
//  PROTOTIPOS
//======================================

//  @brief Funcão de configuracao do ADC
static void setup_adc(void);

//  @brief Funcão de configuracao do I2C
static void setup_i2c(void);

//  @brief Funcão de configuracao do TIMER
static void setup_timer(void);

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

//======================================
//  MAIN
//======================================
int main()
{
    stdio_init_all();

    setup_i2c();
    setup_oled();

    char buffer[16];
    unsigned int cnt = 0;
    while (true)
    {
        sprintf(buffer, "Teste: %d", cnt);
        ssd1306_draw_string(ssd, 5, 0, buffer);
        render_on_display(ssd, &frame_area);
        cnt++;
        RETURN_HOME_SSD(ssd);

        DELAY_MS(1000, alarm_callback, timer_fired);
    }
}
//======================================
//  FUNCS
//======================================

static void setup_adc(void)
{
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

static void setup_timer(void)
{
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
    printf("Timer %d fired!\n", (int)id);
    timer_fired = true;
    // Can return a value here in us to fire in the future
    return 0;
}