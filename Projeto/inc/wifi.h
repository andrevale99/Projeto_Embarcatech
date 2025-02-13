#ifndef WIFI_H
#define WIFI_H

#include "string.h"

#include "pico/stdlib.h"
#include "lwip/tcp.h"
#include "pico/cyw43_arch.h"

#define WIFI_SSID "brisa-2532295" // Substitua pelo nome da sua rede Wi-Fi
#define WIFI_PASS "zlgy1ssc"      // Substitua pela senha da sua rede Wi-Fi
#define THINGSPEAK_API_KEY_WRITE "96B827LC3QSA2348"

#define CYW43_ARCH_INIT_ERROR 1
#define CONECTION_TIMEOUT 2

#define PCB_ERROR_START 1
#define BIND_ERROR 2

#define MAX_TCP_BYTES_SEND UINT16_MAX

char buffer_response_http[MAX_TCP_BYTES_SEND];

// Função de callback para processar requisições HTTP
static err_t http_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
    if (p == NULL)
    {
        // Cliente fechou a conexão
        tcp_close(tpcb);
        return ERR_OK;
    }

    // Processa a requisição HTTP
    char *request = (char *)p->payload;

    // Envia a resposta HTTP
    tcp_write(tpcb, buffer_response_http, UINT8_MAX, TCP_WRITE_FLAG_COPY);

    // Libera o buffer recebido
    pbuf_free(p);

    return ERR_OK;
}

// Callback de conexão: associa o http_callback à conexão
static err_t connection_callback(void *arg, struct tcp_pcb *newpcb, err_t err)
{
    tcp_recv(newpcb, http_callback); // Associa o callback HTTP
    return ERR_OK;
}

uint8_t wifi_start_station_mode(char *buffer_ip, uint8_t len)
{
    // Inicializa o Wi-Fi
    if (cyw43_arch_init())
        return CYW43_ARCH_INIT_ERROR;

    cyw43_arch_enable_sta_mode();

    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASS, CYW43_AUTH_WPA2_AES_PSK, 10000))
        return CONECTION_TIMEOUT;
    else
    {
        uint8_t *ip_address = (uint8_t*)&(cyw43_state.netif[0].ip_addr.addr);
        
        snprintf(buffer_ip, len, "%d.%d.%d.%d\n", ip_address[0], ip_address[1], ip_address[2], ip_address[3]);
    }
    return ERR_OK;
}

// Função de setup do servidor TCP
uint8_t start_http_server(void)
{
    struct tcp_pcb *pcb = tcp_new();
    if (!pcb)
        return PCB_ERROR_START;

    // Liga o servidor na porta 80
    if (tcp_bind(pcb, IP_ADDR_ANY, 80) != ERR_OK)
        return BIND_ERROR;

    pcb = tcp_listen(pcb);                // Coloca o PCB em modo de escuta
    tcp_accept(pcb, connection_callback); // Associa o callback de conexão

    return ERR_OK;
}

#endif