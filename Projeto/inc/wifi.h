#ifndef WIFI_H
#define WIFI_H

#include "string.h"

#include "pico/stdlib.h"
#include "lwip/tcp.h"
#include "pico/cyw43_arch.h"

#include "Personal_defs.h"

#define WIFI_SSID "brisa-2532295" // Substitua pelo nome da sua rede Wi-Fi
#define WIFI_PASS "zlgy1ssc"      // Substitua pela senha da sua rede Wi-Fi
#define THINGSPEAK_API_KEY_WRITE "BI3ZE0R7YBBD1M7H"
#define THINGSPEAK_HOST "api.thingspeak.com"
#define THINGSPEAK_PORT 80

#define CYW43_ARCH_INIT_ERROR 1
#define CONECTION_TIMEOUT 2
#define LWIP_IP_ERROR 3
#define TCP_CLOSE 1

#define MAX_TCP_BYTES_SEND UINT16_MAX

char buffer_response_http[MAX_TCP_BYTES_SEND];
static uint8_t field = 0;

// Callback quando recebe resposta do ThingSpeak
static err_t http_recv_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
    if (p == NULL)
    {
        tcp_close(tpcb);
        return TCP_CLOSE;
    }
    printf("Resposta do ThingSpeak: %.*s\n", p->len, (char *)p->payload);
    pbuf_free(p);
    return ERR_OK;
}

// Callback quando a conexão TCP é estabelecida
static err_t http_connected_callback(void *arg, struct tcp_pcb *tpcb, err_t err)
{
    if (err != ERR_OK)
    {
        printf("Erro na conexão TCP\n");
        return err;
    }

    printf("Conectado ao ThingSpeak!\n");

    if ((field % 2 + 1) == 1)
    {
        snprintf(buffer_response_http, MAX_TCP_BYTES_SEND,
                 "GET https://api.thingspeak.com/update?api_key=%s&field1=%d HTTP/1.1\r\n"
                 "Host: %s\r\n"
                 "Connection: close\r\n"
                 "\r\n",
                 THINGSPEAK_API_KEY_WRITE, data.TempSensor, THINGSPEAK_HOST);
    }
    else
    {
        snprintf(buffer_response_http, MAX_TCP_BYTES_SEND,
                 "GET https://api.thingspeak.com/update?api_key=%s&field2=%d HTTP/1.1\r\n"
                 "Host: %s\r\n"
                 "Connection: close\r\n"
                 "\r\n",
                 THINGSPEAK_API_KEY_WRITE, data.UmidadeSolo, THINGSPEAK_HOST);
    }

    tcp_write(tpcb, buffer_response_http, strlen(buffer_response_http), TCP_WRITE_FLAG_COPY);
    tcp_output(tpcb);
    tcp_recv(tpcb, http_recv_callback);

    field++;

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
        uint8_t *ip_address = (uint8_t *)&(cyw43_state.netif[0].ip_addr.addr);

        snprintf(buffer_ip, len, "%d.%d.%d.%d\n", ip_address[0], ip_address[1], ip_address[2], ip_address[3]);
    }

    return ERR_OK;
}

// Resolver DNS e conectar ao servidor
err_t dns_callback(const char *name, const ip_addr_t *ipaddr, void *callback_arg, struct tcp_pcb *tcp_client_pcb)
{
    if (ipaddr)
    {
        tcp_client_pcb = tcp_new();
        tcp_connect(tcp_client_pcb, ipaddr, THINGSPEAK_PORT, http_connected_callback);
    }
    else
        return LWIP_IP_ERROR;
}

#endif