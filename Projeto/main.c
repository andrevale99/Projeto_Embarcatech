#include <stdio.h>
#include <memory.h>

#include "pico/stdlib.h"

#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"
#include "hardware/timer.h"

#include "inc/ssd1306.h"
#include "inc/personal.h"
#include "wifi.h"

#define I2C_SDA 14
#define I2C_SCL 15

#define TEMP_SENSOR_ADC_CHANNEL 4 // Canal ADC do sensor de Temperatura onboard

#define BUTTON_A 5 // Botao A do bitdoglab
#define BUTTON_B 6 // Botao B do bitdoglab

#define MAX_LEN_BUFFER UINT8_MAX // Tamanho maximo do buffer

// GPIOS e Canais ADC's do joystick (debug)
#define JOY_X_AXIS 26            // Pino de leitura do eixo X do joystick (conectado ao ADC)
#define JOY_Y_AXIS 27            // Pino de leitura do eixo Y do joystick (conectado ao ADC)
#define JOY_ADC_CHANNEL_X_AXIS 0 // Canal ADC para o eixo X do joystick
#define JOY_ADC_CHANNEL_Y_AXIS 1 // Canal ADC para o eixo Y do joystick
#define JOY_BUTTON 22            // Pino de leitura do botão do joystick

// Macro para colocar a escrita no inicoio do OLED
#define RETURN_HOME_SSD(_ssd) memset(_ssd, 0, ssd1306_buffer_length)

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

struct DataHouse_t
{
    uint16_t TempSensor;
    uint16_t UmidadeSolo;

    uint16_t Portao;
} data;

volatile bool flag_timer = false;

char buffer[MAX_LEN_BUFFER];

volatile void (*DisplayShow)(void) = NULL;
volatile uint8_t choosePage = 0;

char ip_pico[50];
struct netif *netif_pico;

extern char buffer_response_http[MAX_TCP_BYTES_SEND];

//======================================
//  PROTOTIPOS
//======================================

//  @brief Pagina inicial do sistema
void hello_page(void);

//  @brief Pagina do sensor de temperatura
void temp_page(void);

//  @brief Pagina do jardim
void jardim_page(void);

//  @brief Pagina do servidor HTPP (situacao)
void http_page(void);

//  @brief Funcão de configuracao do ADC
static void setup_adc(void);

//  @brief Funcão de configuracao do I2C
static void setup_i2c(void);

//  @brief Funcão de configuracao do GPIO
static void setup_gpio(void);

//  @brief Funcão de configuracao do DISPLAY OLED
static void setup_oled(void);

/**
 *  @brief Funcao de call_back (interrupcao) do estouro do TIMER
 *
 *  @param id ID do processo
 *  @param *user_data variaveis do sistema (sem utilidade)
 *
 *  @note Usado como delay
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

// Função para criar a resposta HTTP
void create_http_response(char *buffer, size_t len);

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

    wifi_start_station_mode(&ip_pico[0], 50);

    if (start_http_server() == ERR_OK)
    {
        RETURN_HOME_SSD(ssd);

        sprintf(&buffer[0], "SERVER HTTP");
        ssd1306_draw_string(ssd, 5, 2, buffer);

        sprintf(&buffer[0], "ON");
        ssd1306_draw_string(ssd, 5, 15, buffer);

        render_on_display(ssd, &frame_area);

        DELAY_MS(2000, alarm_callback, flag_timer);
    }

    DisplayShow = hello_page;

    while (true)
    {
        DisplayShow();

        create_http_response(buffer_response_http, MAX_TCP_BYTES_SEND);

        cyw43_arch_poll(); // Necessário para manter o Wi-Fi ativo
    }

    cyw43_arch_deinit();

    return 0;
}
//======================================
//  FUNCS
//======================================

void hello_page(void)
{
    RETURN_HOME_SSD(ssd);
    sprintf(&buffer[0], "BEM VINDO");
    ssd1306_draw_string(ssd, 5, 10, buffer);

    sprintf(&buffer[0], "Press the Botao");
    ssd1306_draw_string(ssd, 5, 20, buffer);

    render_on_display(ssd, &frame_area);
}

void temp_page(void)
{

    // Leitura do valor do eixo X do joystick
    adc_select_input(TEMP_SENSOR_ADC_CHANNEL); // Seleciona o canal ADC para o eixo X
    DELAY_MS(2, alarm_callback, flag_timer);   // Pequeno delay para estabilidade
    data.TempSensor = adc_read();              // Lê o valor do eixo X (0-4095)

    RETURN_HOME_SSD(ssd);

    sprintf(&buffer[0], "TEMP PAGE");
    ssd1306_draw_string(ssd, 5, 5, buffer);

    sprintf(&buffer[0], "Temp: %d", data.TempSensor);
    ssd1306_draw_string(ssd, 5, 18, buffer);

    render_on_display(ssd, &frame_area);
}

void jardim_page(void)
{
    RETURN_HOME_SSD(ssd);

    sprintf(&buffer[0], "Jardim");
    ssd1306_draw_string(ssd, 5, 2, buffer);

    adc_select_input(TEMP_SENSOR_ADC_CHANNEL);
    data.TempSensor = adc_read();
    sprintf(&buffer[0], "Temperatura %d", data.TempSensor);
    ssd1306_draw_string(ssd, 5, 18, buffer);

    adc_select_input(JOY_ADC_CHANNEL_X_AXIS);
    data.UmidadeSolo = adc_read();
    sprintf(&buffer[0], "Solo: %d", data.UmidadeSolo);
    ssd1306_draw_string(ssd, 5, 28, buffer);

    render_on_display(ssd, &frame_area);
}

void http_page(void)
{
    RETURN_HOME_SSD(ssd);

    sprintf(buffer, "IP:");
    ssd1306_draw_string(ssd, 5, 5, buffer);

    sprintf(buffer, "%s", ip_pico);
    ssd1306_draw_string(ssd, 5, 10, buffer);

    render_on_display(ssd, &frame_area);
}

static void setup_adc(void)
{
    adc_init();

    adc_gpio_init(JOY_ADC_CHANNEL_X_AXIS); // Configura o pino VRX (eixo X) para entrada ADC
    adc_gpio_init(JOY_ADC_CHANNEL_Y_AXIS); // Configura o pino VRY (eixo Y) para entrada ADC

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
    gpio_init_mask((1 << BUTTON_A) | (1 << BUTTON_B));
    gpio_set_dir_in_masked((1 << BUTTON_A) | (1 << BUTTON_B));

    gpio_pull_up(BUTTON_A);
    gpio_pull_up(BUTTON_B);

    // Configura a interrupção no GPIO do botão para borda de descida
    gpio_set_irq_enabled_with_callback(BUTTON_A, GPIO_IRQ_EDGE_FALL,
                                       true, gpio_button_callback);

    // Configura a interrupção no GPIO do botão para borda de descida
    gpio_set_irq_enabled_with_callback(BUTTON_B, GPIO_IRQ_EDGE_FALL,
                                       true, gpio_button_callback);
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
    switch (gpio)
    {
    case BUTTON_A:
        choosePage++;
        break;
    case BUTTON_B:
        choosePage--;
        break;

    default:
        break;
    }

    switch (choosePage % 5)
    {
    case 0:
        DisplayShow = hello_page;
        break;

    case 1:
        DisplayShow = temp_page;
        break;

    case 3:
        DisplayShow = jardim_page;
        break;

    case 4:
        DisplayShow = http_page;
        break;

    default:
        break;
    }
}

void create_http_response(char *buffer, size_t len)
{
    snprintf(buffer, len,
             "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=UTF-8\r\n\r\n"
             "<!DOCTYPE html>"
             "<html>"
             "<head>"
             "  <meta charset=\"UTF-8\">"
             "  <title>Controle do LED e Botões</title>"
             "</head>"
             "<body>"
             "  <h1> Dados dos ambientes </h1>"
             "<p> Temperatura Jardim: %d</p>"
             "<p> Solo: %d</p>"
             "</body>"
             "</html>\r\n",
             data.TempSensor, data.UmidadeSolo);
}