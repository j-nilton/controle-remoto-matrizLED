#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "ws2818b.pio.h"
#include "pico/cyw43_arch.h"
#include "lwip/tcp.h"
#include <string.h>

#define LED_COUNT 25
#define LED_PIN 7
#define WIFI_SSID "Nome da rede"  // Substitua pelo nome da sua rede Wi-Fi
#define WIFI_PASS "senha da rede" // Substitua pela senha da sua rede Wi-Fi

struct pixel_t
{
    uint8_t G, R, B;
};
typedef struct pixel_t pixel_t;
typedef pixel_t npLED_t;

npLED_t leds[LED_COUNT];
PIO np_pio;
uint sm;

void npInit(uint pin)
{
    uint offset = pio_add_program(pio0, &ws2818b_program);
    np_pio = pio0;
    sm = pio_claim_unused_sm(np_pio, false);
    if (sm < 0)
    {
        np_pio = pio1;
        sm = pio_claim_unused_sm(np_pio, true);
    }
    ws2818b_program_init(np_pio, sm, offset, pin, 800000.f);
    for (uint i = 0; i < LED_COUNT; ++i)
    {
        leds[i].R = leds[i].G = leds[i].B = 0;
    }
}

void npSetLED(const uint index, const uint8_t r, const uint8_t g, const uint8_t b)
{
    leds[index].R = r;
    leds[index].G = g;
    leds[index].B = b;
}

void npClear()
{
    for (uint i = 0; i < LED_COUNT; ++i)
        npSetLED(i, 0, 0, 0);
}

void npWrite()
{
    for (uint i = 0; i < LED_COUNT; ++i)
    {
        pio_sm_put_blocking(np_pio, sm, leds[i].G);
        pio_sm_put_blocking(np_pio, sm, leds[i].R);
        pio_sm_put_blocking(np_pio, sm, leds[i].B);
    }
    sleep_us(100);
}

int getIndex(int x, int y)
{
    return (y % 2 == 0) ? 24 - (y * 5 + x) : 24 - (y * 5 + (4 - x));
}

// Buffer para resposta HTTP
char http_response[4096];

// Função para criar a resposta HTTP
void create_http_response()
{
    snprintf(http_response, sizeof(http_response),
             "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=UTF-8\r\n\r\n"
             "<!DOCTYPE html>"
             "<html>"
             "<head>"
             "  <meta charset=\"UTF-8\">"
             "  <title>Controle da Matriz de LEDs</title>"
             "</head>"
             "<body>"
             "  <h1>Controle da Matriz de LEDs 5x5</h1>"
             "  <table>"
             "    <tr>"
             "      <td><a href=\"/led/0/0/on\"><img src=\"img/led_on.png\" alt=\"LED (0,0)\" width=\"20\" height=\"20\"></a></td>"
             "      <td><a href=\"/led/0/1/on\"><img src=\"img/led_on.png\" alt=\"LED (0,1)\" width=\"20\" height=\"20\"></a></td>"
             "      <td><a href=\"/led/0/2/on\"><img src=\"img/led_on.png\" alt=\"LED (0,2)\" width=\"20\" height=\"20\"></a></td>"
             "      <td><a href=\"/led/0/3/on\"><img src=\"img/led_on.png\" alt=\"LED (0,3)\" width=\"20\" height=\"20\"></a></td>"
             "      <td><a href=\"/led/0/4/on\"><img src=\"img/led_on.png\" alt=\"LED (0,4)\" width=\"20\" height=\"20\"></a></td>"
             "    </tr>"
             "    <tr>"
             "      <td><a href=\"/led/1/0/on\"><img src=\"img/led_on.png\" alt=\"LED (1,0)\" width=\"20\" height=\"20\"></a></td>"
             "      <td><a href=\"/led/1/1/on\"><img src=\"img/led_on.png\" alt=\"LED (1,1)\" width=\"20\" height=\"20\"></a></td>"
             "      <td><a href=\"/led/1/2/on\"><img src=\"img/led_on.png\" alt=\"LED (1,2)\" width=\"20\" height=\"20\"></a></td>"
             "      <td><a href=\"/led/1/3/on\"><img src=\"img/led_on.png\" alt=\"LED (1,3)\" width=\"20\" height=\"20\"></a></td>"
             "      <td><a href=\"/led/1/4/on\"><img src=\"img/led_on.png\" alt=\"LED (1,4)\" width=\"20\" height=\"20\"></a></td>"
             "    </tr>"
             "    <tr>"
             "      <td><a href=\"/led/2/0/on\"><img src=\"img/led_on.png\" alt=\"LED (2,0)\" width=\"20\" height=\"20\"></a></td>"
             "      <td><a href=\"/led/2/1/on\"><img src=\"img/led_on.png\" alt=\"LED (2,1)\" width=\"20\" height=\"20\"></a></td>"
             "      <td><a href=\"/led/2/2/on\"><img src=\"img/led_on.png\" alt=\"LED (2,2)\" width=\"20\" height=\"20\"></a></td>"
             "      <td><a href=\"/led/2/3/on\"><img src=\"img/led_on.png\" alt=\"LED (2,3)\" width=\"20\" height=\"20\"></a></td>"
             "      <td><a href=\"/led/2/4/on\"><img src=\"img/led_on.png\" alt=\"LED (2,4)\" width=\"20\" height=\"20\"></a></td>"
             "    </tr>"
             "    <tr>"
             "      <td><a href=\"/led/3/0/on\"><img src=\"img/led_on.png\" alt=\"LED (3,0)\" width=\"20\" height=\"20\"></a></td>"
             "      <td><a href=\"/led/3/1/on\"><img src=\"img/led_on.png\" alt=\"LED (3,1)\" width=\"20\" height=\"20\"></a></td>"
             "      <td><a href=\"/led/3/2/on\"><img src=\"img/led_on.png\" alt=\"LED (3,2)\" width=\"20\" height=\"20\"></a></td>"
             "      <td><a href=\"/led/3/3/on\"><img src=\"img/led_on.png\" alt=\"LED (3,3)\" width=\"20\" height=\"20\"></a></td>"
             "      <td><a href=\"/led/3/4/on\"><img src=\"img/led_on.png\" alt=\"LED (3,4)\" width=\"20\" height=\"20\"></a></td>"
             "    </tr>"
             "    <tr>"
             "      <td><a href=\"/led/4/0/on\"><img src=\"img/led_on.png\" alt=\"LED (4,0)\" width=\"20\" height=\"20\"></a></td>"
             "      <td><a href=\"/led/4/1/on\"><img src=\"img/led_on.png\" alt=\"LED (4,1)\" width=\"20\" height=\"20\"></a></td>"
             "      <td><a href=\"/led/4/2/on\"><img src=\"img/led_on.png\" alt=\"LED (4,2)\" width=\"20\" height=\"20\"></a></td>"
             "      <td><a href=\"/led/4/3/on\"><img src=\"img/led_on.png\" alt=\"LED (4,3)\" width=\"20\" height=\"20\"></a></td>"
             "      <td><a href=\"/led/4/4/on\"><img src=\"img/led_on.png\" alt=\"LED (4,4)\" width=\"20\" height=\"20\"></a></td>"
             "    </tr>"
             "  </table>"
             "  <p><a href=\"/clear\">Desligar os LEDs</a></p>"
             "</body>"
             "</html>\r\n");
}

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

    if (strstr(request, "GET /led/"))
    {
        int x, y;
        sscanf(request, "GET /led/%d/%d/on", &x, &y);
        int index = getIndex(x, y);
        npSetLED(index, 255, 255, 255); // Liga o LED na posição (x, y)
        npWrite();
    }
    else if (strstr(request, "GET /clear"))
    {
        npClear(); // Desliga todos os LEDs
        npWrite();
    }

    // Atualiza o conteúdo da página
    create_http_response();

    // Envia a resposta HTTP
    tcp_write(tpcb, http_response, strlen(http_response), TCP_WRITE_FLAG_COPY);

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

// Função de setup do servidor TCP
static void start_http_server(void)
{
    struct tcp_pcb *pcb = tcp_new();
    if (!pcb)
    {
        printf("Erro ao criar PCB\n");
        return;
    }

    // Liga o servidor na porta 80
    if (tcp_bind(pcb, IP_ADDR_ANY, 80) != ERR_OK)
    {
        printf("Erro ao o servidor na porta 80\n");
        return;
    }

    pcb = tcp_listen(pcb);                // Coloca o PCB em modo de escuta
    tcp_accept(pcb, connection_callback); // Associa o callback de conexão

    printf("Servidor HTTP rodando na porta 80...\n");
}

int main()
{
    stdio_init_all(); // Inicializa a saída padrão
    sleep_ms(10000);
    printf("Iniciando servidor HTTP\n");

    // Inicializa a matriz de LEDs
    npInit(LED_PIN);
    npClear();
    npWrite();

    // Inicializa o Wi-Fi
    if (cyw43_arch_init())
    {
        printf("Erro ao inicializar o Wi-Fi\n");
        return 1;
    }

    cyw43_arch_enable_sta_mode();
    printf("Conectando ao Wi-Fi...\n");

    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASS, CYW43_AUTH_WPA2_AES_PSK, 10000))
    {
        printf("Falha ao conectar ao Wi-Fi\n");
        return 1;
    }
    else
    {
        printf("Connected.\n");
        // Read the ip address in a human readable way
        uint8_t *ip_address = (uint8_t *)&(cyw43_state.netif[0].ip_addr.addr);
        printf("Endereço IP %d.%d.%d.%d\n", ip_address[0], ip_address[1], ip_address[2], ip_address[3]);
    }

    printf("Wi-Fi conectado!\n");

    // Inicia o servidor HTTP
    start_http_server();

    // Loop principal
    while (true)
    {
        cyw43_arch_poll(); // Necessário para manter o Wi-Fi ativo
        sleep_ms(100);     // Reduz o uso da CPU
    }

    cyw43_arch_deinit(); // Desliga o Wi-Fi (não será chamado, pois o loop é infinito)
    return 0;
}