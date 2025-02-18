#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/adc.h"
#include "inc/ssd1306.h"

// Definição de constantes
#define A_BUTTON 5
#define LED_RED 13
#define LED_BLUE 12
#define LED_GREEN 11
#define JOYSTICK_BUTTON 22
#define JOYSTICK_X 26
#define JOYSTICK_Y 27
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C

uint32_t last_time;

ssd1306_t ssd;      // Inicializa a estrutura do display
bool cor = true;

// Protótipo das Funções
void setup(void);
static void gpio_irq_handler(uint gpio, uint32_t events);

int main()
{
    setup(); // Configuração das portas digitais

    // Habilitando interrupção da gpio no botão A e no botão do Joystick.
    gpio_set_irq_enabled_with_callback(A_BUTTON, GPIO_IRQ_EDGE_FALL, 1, & gpio_irq_handler);
    gpio_set_irq_enabled(JOYSTICK_BUTTON, GPIO_IRQ_EDGE_FALL, true);

    uint16_t adc_value_x;
    uint16_t adc_value_y;

    // Intervalos ADC
    // X: 19-20 .. 2160 .. 4085
    // Y: 19-20 .. 1940 .. 4085

    while(true) {

        adc_select_input(1); // Seleciona o ADC para eixo X. O pino 26 como entrada analógica
        adc_value_x = adc_read();
        adc_select_input(0); // Seleciona o ADC para eixo Y. O pino 27 como entrada analógica
        adc_value_y = adc_read();

    }

    return 0;
}

// Inicialização e configuração das portas digitais
void setup(void)
{
    stdio_init_all();

    gpio_init(LED_RED);         // Inicializando LED vermelho.
    gpio_set_dir(LED_RED, GPIO_OUT);

    gpio_init(LED_BLUE);        // Inicializando LED azul.
    gpio_set_dir(LED_BLUE, GPIO_OUT);

    gpio_init(LED_GREEN);       // Inicializando LED verde.
    gpio_set_dir(LED_GREEN, GPIO_OUT);

    gpio_init(A_BUTTON);        // Inicializando Botão A.
    gpio_set_dir(A_BUTTON, GPIO_IN);
    gpio_pull_up(A_BUTTON);

    adc_init();                 // Inicializar o ADC
    adc_gpio_init(JOYSTICK_X);  // Configurar GPIO para eixo X
    adc_gpio_init(JOYSTICK_Y);  // Configurar GPIO para eixo Y

    gpio_init(JOYSTICK_BUTTON); // Inicializando o Botão do joystick
    gpio_set_dir(JOYSTICK_BUTTON, GPIO_IN);
    gpio_pull_up(JOYSTICK_BUTTON);

}

// Rotina da Interrupção
static void gpio_irq_handler(uint gpio, uint32_t events){

    uint32_t current_time = to_us_since_boot(get_absolute_time());
    if (current_time - last_time > 200000) // Delay de 200ms para debounce
    {
        last_time = current_time;

        switch (gpio) {
        case 5:

            break;

        case 22:

            break;
        
        default:
            break;
        }
    }
}