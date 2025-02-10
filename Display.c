//Bibliotecas do Pico
#include <stdlib.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/pio.h"
#include "hardware/timer.h"
#include "hardware/adc.h"
#include "hardware/clocks.h"

//Importando os arquivos das pastas
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

#define UART_ID uart0 // Seleciona a UART0
#define BAUD_RATE 115200 // Define a taxa de transmissão
#define UART_TX_PIN 0 // Pino GPIO usado para TX
#define UART_RX_PIN 1 // Pino GPIO usado para RX
#define LED_VERDE 11 // Pino GPIO usado para o LED
#define LED_AZUL 12 // Pino GPIO usado para o LED
#define MATRIZ_LED 7 //Pino da matriz de LEDs
#define botão_A 5 //Porta do botão A
#define botão_B 6 //Porta do botão B

// Prototipação das funções
static void gpio_irq_handler(uint gpio, uint32_t events);
void desenho_pio(double *desenho, uint32_t valor_led, PIO pio, uint sm, double r, double g, double b);

static float intensidade = 0.1;//Define a intensidade do brilho do LED da matriz
static volatile uint cont_cor = 0;//Contador que define a cor do número
static volatile uint32_t last_time = 0; // Armazena o tempo do último evento (em microssegundos)
static volatile bool estado_LED_verde = false;
static volatile bool estado_LED_azul = false;
static char buffer_LED1[50];
static char buffer_LED2[50];
bool cor = true;
char c;// Várivel que será lida pela USB
char buffer[1];// Buffer para armazenar a mensagem que será enviada

int main()
{
  //Inicializando o PIO
  static PIO pio = pio0; 
  uint16_t i;
  uint32_t valor_led;

  // Inicializa todos os códigos stdio padrão 
  stdio_init_all();

  //Configurações da PIO
  uint offset = pio_add_program(pio, &pio_matrix_program);
  uint sm = pio_claim_unused_sm(pio, true);
  pio_matrix_program_init(pio, sm, offset, MATRIZ_LED);

  // I2C Initialisation. Using it at 400Khz.
  i2c_init(I2C_PORT, 400 * 1000);
  uart_init(UART_ID, BAUD_RATE);

  // Configura os pinos GPIO para a UART
  gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART); // Configura o pino 0 para TX
  gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART); // Configura o pino 1 para RX
  gpio_set_function(I2C_SDA, GPIO_FUNC_I2C); // Set the GPIO pin function to I2C
  gpio_set_function(I2C_SCL, GPIO_FUNC_I2C); // Set the GPIO pin function to I2C
  gpio_pull_up(I2C_SDA); // Pull up the data line
  gpio_pull_up(I2C_SCL); // Pull up the clock line
  ssd1306_t ssd; // Inicializa a estrutura do display
  ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT); // Inicializa o display
  ssd1306_config(&ssd); // Configura o display
  ssd1306_send_data(&ssd); // Envia os dados para o display

  // Limpa o display. O display inicia com todos os pixels apagados.
  ssd1306_fill(&ssd, false);
  ssd1306_send_data(&ssd);

  //Inciailizando e configurando os pinos dos botões e LEDs
  gpio_init(botão_A);//Inicializa o pino do botão A
  gpio_set_dir(botão_A, GPIO_IN);// Configura o pino como entrada
  gpio_pull_up(botão_A);// Habilita o pull-up interno
  gpio_init(botão_B);//Inicializa o pino do botão B
  gpio_set_dir(botão_B, GPIO_IN);// Configura o pino como entrada
  gpio_pull_up(botão_B);// Habilita o pull-up interno
  gpio_init(LED_VERDE);// Inicializa o pino do LED VERDE
  gpio_set_dir(LED_VERDE, GPIO_OUT); // Configura o pino como saída  
  gpio_init(LED_AZUL);// Inicializa o pino do LED AZUL
  gpio_set_dir(LED_AZUL, GPIO_OUT); // Configura o pino como saída

  // Configuração da interrupção com callback
  gpio_set_irq_enabled_with_callback(botão_A, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
  gpio_set_irq_enabled(botão_B, GPIO_IRQ_EDGE_FALL,true);

  
  
  while (true)
  {
    if (uart_is_readable(UART_ID)) {
      c = uart_getc(UART_ID);// Lê o caractere recebido
      if (c != '\n') {
        buffer[0] = c;//Guarda o caractere em um buffer, sendo capaz de formar strings completas caso aumente o buffer
      }
    }
    if(buffer[0] >= '0' && c <= '9'){
      desenho_pio(numeros[buffer[0]-48], valor_led, pio, sm, intensidade * lista_de_cores[cont_cor][0],//O indice [buffer[0]-48] é para transformar o valor ASCII '0' em um inteiro 0
                                                    intensidade * lista_de_cores[cont_cor][1] ,//As cores estão na ordem RGB
                                                    intensidade * lista_de_cores[cont_cor][2]);//Além disso existe a váriavel intensidade que define o brilho

    }
    else{
      desenho_pio(leds_apagados, valor_led, pio, sm, intensidade * lista_de_cores[cont_cor][0],//A cor do número é definido por outro contador (lista_de_cores[cor])
                                                    intensidade * lista_de_cores[cont_cor][1] ,//As cores estão na ordem RGB
                                                    intensidade * lista_de_cores[cont_cor][2]);//Além disso existe a váriavel intensidade que define o brilho
    }
    
    // Atualiza o conteúdo do display com animações
    ssd1306_fill(&ssd, cor); // Limpa o display
    ssd1306_rect(&ssd, 3, 3, 122, 58, !cor, cor); // Desenha um retângulo
    ssd1306_draw_string(&ssd, "Digite algo:", 8, 10); // Desenha uma string   
    ssd1306_draw_string(&ssd,buffer,50, 20); // Desenha uma string
    ssd1306_draw_string(&ssd,buffer_LED1,15, 35);
    ssd1306_draw_string(&ssd,buffer_LED2,15, 48);        
    ssd1306_send_data(&ssd); // Atualiza o display

    sleep_ms(100);
  }
}
void gpio_irq_handler(uint gpio, uint32_t events){
  uint32_t current_time = to_us_since_boot(get_absolute_time());// Obtém o tempo atual em microssegundos
  if (current_time - last_time > 200000){ // Verifica se passou tempo suficiente desde o último evento com 200 ms de debouncing 
      last_time = current_time; // Atualiza o tempo do último evento
      cont_cor++;
      if(cont_cor==6){
        cont_cor =0;
      }
      if(gpio == botão_A){
        estado_LED_verde = !estado_LED_verde;
        gpio_put(LED_VERDE,estado_LED_verde);
        sprintf(buffer_LED1, "LED Verde %s", estado_LED_verde ? "ON" : "OFF");
        printf("%s\n",buffer_LED1);
        }
      else if(gpio == botão_B){
        estado_LED_azul = !estado_LED_azul; 
        gpio_put(LED_AZUL,estado_LED_azul);
        sprintf(buffer_LED2, "LED azul %s", estado_LED_azul ? "ON" : "OFF");
        printf("%s\n",buffer_LED2);
      }
  }
}