#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "ws2818b.pio.h"
#include "pico/cyw43_arch.h"
#include "lwip/tcp.h"
#include <string.h>

// Definição de constantes
#define LED_COUNT 25            // Número total de LEDs na matriz
#define LED_PIN 7               // Pino GPIO onde os LEDs estão conectados
#define NUM_ROWS 5              // Número de linhas da matriz de LEDs
#define NUM_COLS 5              // Número de colunas da matriz de LEDs
#define WIFI_SSID "Digite aqui" // Nome da rede Wi-Fi
#define WIFI_PASS "Digite aqui" // Senha da rede Wi-Fi

// Estrutura para armazenar a cor de um LED (formato GRB)
struct pixel_t
{
    uint8_t G, R, B;
};
typedef struct pixel_t pixel_t;
typedef pixel_t npLED_t;

// Array que representa o estado dos LEDs
npLED_t leds[LED_COUNT];
PIO np_pio;
uint sm;

// Inicializa o controle dos LEDs
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

// Define a cor de um LED específico
void npSetLED(const uint index, const uint8_t r, const uint8_t g, const uint8_t b)
{
    leds[index].R = r;
    leds[index].G = g;
    leds[index].B = b;
}

// Desliga todos os LEDs
void npClear()
{
    for (uint i = 0; i < LED_COUNT; ++i)
        npSetLED(i, 0, 0, 0);
}

// Atualiza o estado dos LEDs na matriz
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

// Converte coordenadas (x, y) para o índice do LED na matriz
int getIndex(int x, int y)
{
    return (y % 2 == 0) ? 24 - (y * 5 + x) : 24 - (y * 5 + (4 - x));
}

// Buffer para resposta HTTP
char http_response[2096];
int led_status[5][5] = {0}; // 0 = desligado, 1 = ligado

// Função para criar a resposta HTTP
void create_http_response()
{
    snprintf(http_response, sizeof(http_response),
             "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=UTF-8\r\n\r\n"
             "<!DOCTYPE html>"
             "<html>"
             "<head>"
             "<meta charset=\"UTF-8\">"
             "<title>Controle da Matriz de LEDs</title>"
             "<style>"
             "body {"
             "    font-family: Arial, sans-serif;"
             "    background-color: #f4f4f4;"
             "    margin: 0;"
             "    padding: 20px;"
             "    display: flex;"
             "    flex-direction: column;"
             "    align-items: center;"
             "    justify-content: flex-start;"
             "}"
             "h1 {"
             "    color: #333;"
             "    margin: 10px 0;"
             "    font-size: 24px;"
             "}"
             "table {"
             "    border-collapse: collapse;"
             "    margin-bottom: 20px;"
             "    box-shadow: 0 4px 8px rgba(0, 0, 0, 0.1);"
             "    border-radius: 10px;"
             "    overflow: hidden;"
             "}"
             "td {"
             "    width: 50px;"
             "    height: 50px;"
             "    border: 2px solid #ddd;"
             "    text-align: center;"
             "    background-color: #fff;"
             "    transition: background-color 0.3s ease;"
             "}"
             "td.led-on {"
             "    background-color: #4CAF50;"
             "}"
             "td a {"
             "    display: block;"
             "    width: 100%;"
             "    height: 100%;"
             "    text-decoration: none;"
             "}"
             "button.clear-button {"
             "    background-color: #f44336;"
             "    color: white;"
             "    border: none;"
             "    padding: 10px 20px;"
             "    font-size: 16px;"
             "    border-radius: 5px;"
             "    cursor: pointer;"
             "    transition: background-color 0.3s ease;"
             "}"
             "button.clear-button:hover {"
             "    background-color: #d32f2f;"
             "}"
             "</style>"
             "</head>"
             "<body>"
             "<h1>Controle da Matriz de LEDs 5x5</h1>"
             "<table>");

    for (int y = 0; y < 5; y++)
    {
        strcat(http_response, "<tr>");
        for (int x = 0; x < 5; x++)
        {
            char cell[100];
            snprintf(cell, sizeof(cell), "<td class='%s'><a href='/led/%d/%d/on'>&nbsp;</a></td>",
                     led_status[x][y] ? "led-on" : "", x, y);
            strcat(http_response, cell);
        }
        strcat(http_response, "</tr>");
    }

    strcat(http_response, "</table>");
    strcat(http_response, "<button class='clear-button' onclick=\"window.location.href='/clear'\">Desligar os LEDs</button>");
    strcat(http_response, "</body></html>\r\n");
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

        if (led_status[x][y] == 0)
        {
            npSetLED(index, 255, 255, 255); // Liga o LED
            led_status[x][y] = 1;           // Marca como ligado
        }
        else
        {
            npSetLED(index, 0, 0, 0); // Desliga o LED
            led_status[x][y] = 0;     // Marca como desligado
        }

        npWrite();
        create_http_response(); // Atualiza a resposta HTML
        tcp_write(tpcb, http_response, strlen(http_response), TCP_WRITE_FLAG_COPY);
    }

    if (strstr(request, "GET /clear"))
    {
        // Percorre toda a matriz e desliga os LEDs
        for (int x = 0; x < NUM_ROWS; x++)
        {
            for (int y = 0; y < NUM_COLS; y++)
            {
                int index = getIndex(x, y);
                npSetLED(index, 0, 0, 0); // Desliga o LED
                led_status[x][y] = 0;     // Marca como desligado
            }
        }

        npWrite();              // Atualiza os LEDs
        create_http_response(); // Atualiza a resposta HTML
        tcp_write(tpcb, http_response, strlen(http_response), TCP_WRITE_FLAG_COPY);
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

// Função principal
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
        printf("Conectado.\n");
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
