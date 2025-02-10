// Bibliotecas do Pico
#include <stdlib.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/pio.h"
#include "hardware/timer.h"
#include "hardware/adc.h"
#include "hardware/clocks.h"

// Importando os arquivos das pastas
#include "Funções/font.h"
#include "Funções/ssd1306.h"
#include "Funções/mudar_LED.c"
#include "Funções/numeros.h"
#include "Funções/cores.h"
#include "LED.pio.h"

#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C

#define UART_ID uart0
#define BAUD_RATE 115200
#define UART_TX_PIN 0
#define UART_RX_PIN 1
#define LED_VERDE 11
#define LED_AZUL 12
#define MATRIZ_LED 7
#define botão_A 5
#define botão_B 6

// Prototipação das funções
static void gpio_irq_handler(uint gpio, uint32_t events);
void desenho_pio(double *desenho, uint32_t valor_led, PIO pio, uint sm, double r, double g, double b);

// Variáveis globais
static float intensidade = 0.1;
static volatile uint cont_cor = 0;
static volatile uint32_t last_time = 0;
static volatile bool estado_LED_verde = false;
static volatile bool estado_LED_azul = false;
static char buffer_LED1[50];
static char buffer_LED2[50];
bool cor = true;
char c;
char buffer[1];
ssd1306_t ssd;  // Torna a variável ssd global

int main() {
  static PIO pio = pio0; 
  uint32_t valor_led;

  stdio_init_all();

  uint offset = pio_add_program(pio, &pio_matrix_program);
  uint sm = pio_claim_unused_sm(pio, true);
  pio_matrix_program_init(pio, sm, offset, MATRIZ_LED);

  i2c_init(I2C_PORT, 400 * 1000);
  uart_init(UART_ID, BAUD_RATE);
  gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
  gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
  gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
  gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
  gpio_pull_up(I2C_SDA);
  gpio_pull_up(I2C_SCL);
  
  ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT);
  ssd1306_config(&ssd);
  ssd1306_fill(&ssd, false);
  ssd1306_send_data(&ssd);

  gpio_init(botão_A);
  gpio_set_dir(botão_A, GPIO_IN);
  gpio_pull_up(botão_A);
  gpio_init(botão_B);
  gpio_set_dir(botão_B, GPIO_IN);
  gpio_pull_up(botão_B);
  gpio_init(LED_VERDE);
  gpio_set_dir(LED_VERDE, GPIO_OUT);
  gpio_init(LED_AZUL);
  gpio_set_dir(LED_AZUL, GPIO_OUT);

  gpio_set_irq_enabled_with_callback(botão_A, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
  gpio_set_irq_enabled_with_callback(botão_B, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

  while (true) {
      c = getchar();
      if (c != '\n') {
        buffer[0] = c;
      }
    
    if (buffer[0] >= '0' && buffer[0] <= '9') {
      desenho_pio(numeros[buffer[0] - 48], valor_led, pio, sm, 
                  intensidade * lista_de_cores[cont_cor][0], 
                  intensidade * lista_de_cores[cont_cor][1], 
                  intensidade * lista_de_cores[cont_cor][2]);
    } else {
      desenho_pio(leds_apagados, valor_led, pio, sm, 
                  intensidade * lista_de_cores[cont_cor][0], 
                  intensidade * lista_de_cores[cont_cor][1], 
                  intensidade * lista_de_cores[cont_cor][2]);
    }
    
    ssd1306_fill(&ssd, cor);
    ssd1306_rect(&ssd, 3, 3, 122, 58, !cor, cor);
    ssd1306_draw_string(&ssd, "Digite algo:", 8, 10);
    ssd1306_draw_string(&ssd, buffer, 50, 20);
    ssd1306_draw_string(&ssd, buffer_LED1, 15, 35);
    ssd1306_draw_string(&ssd, buffer_LED2, 15, 48);
    ssd1306_send_data(&ssd);
    
    sleep_ms(100);
  }
}

void gpio_irq_handler(uint gpio, uint32_t events) {
  uint32_t current_time = to_us_since_boot(get_absolute_time());
  if (current_time - last_time > 200000) {
    last_time = current_time;
    cont_cor++;
    if (cont_cor == 6) {
      cont_cor = 0;
    }
    if (gpio == botão_A) {
      estado_LED_verde = !estado_LED_verde;
      gpio_put(LED_VERDE, estado_LED_verde);
      sprintf(buffer_LED1, "LED Verde %s", estado_LED_verde ? "ON" : "OFF");
      printf("%s\n", buffer_LED1);
      ssd1306_draw_string(&ssd, buffer_LED1, 15, 35);
      ssd1306_send_data(&ssd);
    } else if (gpio == botão_B) {
      estado_LED_azul = !estado_LED_azul;
      gpio_put(LED_AZUL, estado_LED_azul);
      sprintf(buffer_LED2, "LED Azul %s", estado_LED_azul ? "ON" : "OFF");
      printf("%s\n", buffer_LED2);
      ssd1306_draw_string(&ssd, buffer_LED2, 15, 48);
      ssd1306_send_data(&ssd);
    }
  }
}
