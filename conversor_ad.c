#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
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
#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 64
#define RECT_SIZE 8

uint32_t last_time;
uint8_t tolerancia_ruido = 100; // Tolerância de ruído para os valores lidos do joystick

volatile bool pwm_leds = true;
volatile bool cor = true;

// Protótipo das Funções
void setup(void);
static void gpio_irq_handler(uint gpio, uint32_t events);
uint pwm_init_gpio(uint gpio, uint wrap);
int map(int valor, int center, int in_min, int in_max, int out_min, int out_max);

int main()
{
    setup(); // Configuração das portas digitais

    // Habilitando interrupção da gpio no botão A e no botão do Joystick.
    gpio_set_irq_enabled_with_callback(A_BUTTON, GPIO_IRQ_EDGE_FALL, 1, & gpio_irq_handler);
    gpio_set_irq_enabled(JOYSTICK_BUTTON, GPIO_IRQ_EDGE_FALL, true);

    uint16_t adc_value_x;
    uint16_t adc_value_y;

    uint pwm_wrap = 4096;  
    uint pwm_red_slice = pwm_init_gpio(LED_RED, pwm_wrap);
    uint pwm_blue_slice = pwm_init_gpio(LED_BLUE, pwm_wrap);

    ssd1306_t ssd; // Inicializa a estrutura do display
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT); // Inicializa o display
    ssd1306_config(&ssd); // Configura o display
    ssd1306_send_data(&ssd); // Envia os dados para o display

    // Inicia o display com todos os pixels apagados.
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);

    // Intervalos ADC
    // X: 19-20 .. 2160 .. 4085
    // Y: 19-20 .. 1940 .. 4085

    int new_center_x;
    int new_center_y;
    int rect_x;
    int rect_y;

    while(true) {    
        adc_select_input(1); // Seleciona o ADC para eixo X. O pino 26 como entrada analógica
        adc_value_x = adc_read();
        adc_select_input(0); // Seleciona o ADC para eixo Y. O pino 27 como entrada analógica
        adc_value_y = adc_read();

        // A cada acionamento do botão A, alterna entre ON/OFF os LED PWM
        if (pwm_leds)
        {   
            // Desliga o Led Vermelho quando o eixo x do joystick estiver centralizado
            if (adc_value_x >= (2160 - tolerancia_ruido) && adc_value_x <= (2160 + tolerancia_ruido)) {
                pwm_set_gpio_level(LED_RED, 0);
            } else {    // Varia intensidade do Led de acordo com a posição do joystick
                pwm_set_gpio_level(LED_RED, adc_value_x);
            }

            // Desliga o Led Azul quando o eixo y do joystick estiver centralizado
            if (adc_value_y >= (1940 - tolerancia_ruido) && adc_value_y <= (1940 + tolerancia_ruido)) {
                pwm_set_gpio_level(LED_BLUE, 0);
            } else {    // Varia intensidade do Led de acordo com a posição do joystick
                pwm_set_gpio_level(LED_BLUE, adc_value_y);
            }
        } else {
            pwm_set_gpio_level(LED_RED, 0);
            pwm_set_gpio_level(LED_BLUE, 0);
        }

        // Mapeia valores do ADC para o display considerando as posições centrais do joystick (2160,1940)
        new_center_x = map(adc_value_x, 2160, 20, 4085, 0, DISPLAY_WIDTH);
        new_center_y = map(adc_value_y, 1940, 20, 4085, 0, DISPLAY_HEIGHT);

        // Ajuste da posição para canto superior esquerdo do retângulo
        rect_x = new_center_x - (RECT_SIZE / 2);
        rect_y = new_center_y - (RECT_SIZE / 2);

        // Garantir que o retângulo não saia da tela
        if (rect_x < 0) rect_x = 0;
        if (rect_x > DISPLAY_WIDTH - RECT_SIZE) rect_x = DISPLAY_WIDTH - RECT_SIZE;
        if (rect_y < 0) rect_y = 0;
        if (rect_y > DISPLAY_HEIGHT - RECT_SIZE) rect_y = DISPLAY_HEIGHT - RECT_SIZE;

        // Atualiza a tela do display
        ssd1306_fill(&ssd, false); // Limpa a tela
        ssd1306_rect(&ssd, 3, 3, 124, 60, cor, !cor);
        ssd1306_rect(&ssd, rect_y, rect_x, RECT_SIZE, RECT_SIZE, 1, 0); // Desenha o retângulo
        ssd1306_send_data(&ssd); // Exibe na tela

        sleep_ms(50); // Pequeno delay para suavizar a movimentação
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

    // Inicialização do I2C em 400Khz.
    i2c_init(I2C_PORT, 400 * 1000);

    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C); // Configura o pino GPIO para I2C
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C); // Configura o pino GPIO para I2C
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

}

// Rotina da Interrupção
static void gpio_irq_handler(uint gpio, uint32_t events){

    uint32_t current_time = to_us_since_boot(get_absolute_time());
    if (current_time - last_time > 200000) // Delay de 200ms para debounce
    {
        last_time = current_time;

        switch (gpio) {
        case A_BUTTON:  // Botão A Liga/Desliga os Leds PWM
            pwm_leds = !pwm_leds;   
            break;

        case JOYSTICK_BUTTON:   // Botão do Joystick Ativa/Desativa Borda do Display e Liga/Desliga Led Verde
            cor = !cor;     
            gpio_put(LED_GREEN, !gpio_get(LED_GREEN));
            break;
        
        default:
            break;
        }
    }
}

uint pwm_init_gpio(uint gpio, uint wrap) {
    gpio_set_function(gpio, GPIO_FUNC_PWM);

    uint slice_num = pwm_gpio_to_slice_num(gpio);
    pwm_set_wrap(slice_num, wrap);
    
    pwm_set_enabled(slice_num, true);  
    return slice_num;  
}

// Função para converter proporcionalmente os valores do adc para o display
int map(int value, int center, int min, int max, int out_min, int out_max) {
    return ((value - center) * (out_max - out_min) / (max - min)) + (out_min + out_max) / 2;
}