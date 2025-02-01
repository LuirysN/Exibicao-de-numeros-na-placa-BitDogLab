#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include <stdint.h>

#include "ws2812b.pio.h"

#define LED_COUNT 25
#define LED_PIN 7
#define BOTAO_A 5
#define BOTAO_B 6
#define LED_R 12
#define BLINK_INTERVAL 100  // LED alterna estado a cada 100ms (5 piscadas por segundo)
#define DEBOUNCE_TIME 100   // Ajustado para 100ms para melhor debounce

uint32_t lastBlinkTime = 0;
bool ledState = false;
volatile uint32_t lastPressTimeA = 0;
volatile uint32_t lastPressTimeB = 0;

struct pixel_t {
    uint8_t G, R, B;
};
typedef struct pixel_t pixel_t;
typedef pixel_t npLED_t;

npLED_t leds[LED_COUNT];

PIO np_pio;
uint sm;
volatile int numero_atual = 0;

// Inicializa WS2812
void npInit(uint pin) {
    uint offset = pio_add_program(pio0, &ws2812_program);
    np_pio = pio0;
    sm = pio_claim_unused_sm(np_pio, true);
    ws2812_program_init(np_pio, sm, offset, pin, 800000.f, false);

    for (uint i = 0; i < LED_COUNT; ++i) {
        leds[i].R = 0;
        leds[i].G = 0;
        leds[i].B = 0;
    }
}

void npSetLED(const uint index, const uint8_t r, const uint8_t g, const uint8_t b) {
    leds[index].R = r;
    leds[index].G = g;
    leds[index].B = b;
}

void npClear() {
    for (uint i = 0; i < LED_COUNT; ++i) {
        npSetLED(i, 0, 0, 0);
    }
}

void npWrite() {
    for (uint i = 0; i < LED_COUNT; ++i) {
        pio_sm_put_blocking(np_pio, sm, (leds[i].G << 16) | (leds[i].R << 8) | leds[i].B);
    }
    sleep_us(100);
}

uint32_t numeros[10][LED_COUNT] = {
    {1, 1, 1, 1, 1,
     1, 0, 0, 0, 1,
     1, 0, 0, 0, 1,
     1, 0, 0, 0, 1,
     1, 1, 1, 1, 1}, // 0
    
    {0, 0, 1, 0, 0,
     0, 0, 1, 0, 0,
     0, 0, 1, 0, 0,
     0, 0, 1, 0, 0,
     0, 0, 1, 0, 0}, // 1
    
    {1, 1, 1, 1, 1,
     0, 0, 0, 0, 1,
     1, 1, 1, 1, 1,
     1, 0, 0, 0, 0,
     1, 1, 1, 1, 1}, // 2
    
    {1, 1, 1, 1, 1,
     0, 0, 0, 0, 1,
     0, 1, 1, 1, 1,
     0, 0, 0, 0, 1,
     1, 1, 1, 1, 1}, // 3
    
    {1, 0, 0, 0, 1,
     1, 0, 0, 0, 1,
     1, 1, 1, 1, 1,
     0, 0, 0, 0, 1,
     0, 0, 0, 0, 1}, // 4
    
    {1, 1, 1, 1, 1,
     1, 0, 0, 0, 0,
     1, 1, 1, 1, 1,
     0, 0, 0, 0, 1,
     1, 1, 1, 1, 1}, // 5
    
    {1, 1, 1, 1, 1,
     1, 0, 0, 0, 0,
     1, 1, 1, 1, 1,
     1, 0, 0, 0, 1,
     1, 1, 1, 1, 1}, // 6
    
    {1, 1, 1, 1, 1,
     0, 0, 0, 0, 1,
     0, 0, 0, 1, 0,
     0, 0, 1, 0, 0,
     0, 1, 0, 0, 0}, // 7
    
    {1, 1, 1, 1, 1,
     1, 0, 0, 0, 1,
     1, 1, 1, 1, 1,
     1, 0, 0, 0, 1,
     1, 1, 1, 1, 1}, // 8
    
    {1, 1, 1, 1, 1,
     1, 0, 0, 0, 1,
     1, 1, 1, 1, 1,
     0, 0, 0, 0, 1,
     1, 1, 1, 1, 1}  // 9
};


void mostrarNumero(int numero) {
    npClear();
    for (int y = 0; y < 5; y++) {
        for (int x = 0; x < 5; x++) {
            int index = (4 - y) * 5 + ((4 - y) % 2 == 0 ? (4 - x) : x);
            if (numeros[numero][y * 5 + x]) {
                npSetLED(index, 255, 0, 0);
            }
        }
    }
    npWrite();
}

void gpio_callback(uint gpio, uint32_t events) {
    uint32_t currentTime = to_ms_since_boot(get_absolute_time());

    if (gpio == BOTAO_A && (currentTime - lastPressTimeA > DEBOUNCE_TIME)) {
        lastPressTimeA = currentTime;
        if (numero_atual < 9) {
            numero_atual++;
            mostrarNumero(numero_atual);
        }
    }

    if (gpio == BOTAO_B && (currentTime - lastPressTimeB > DEBOUNCE_TIME)) {
        lastPressTimeB = currentTime;
        if (numero_atual > 0) {
            numero_atual--;
            mostrarNumero(numero_atual);
        }
    }
}

void piscarLED() {
    uint32_t currentTime = to_ms_since_boot(get_absolute_time());
    if (currentTime - lastBlinkTime >= BLINK_INTERVAL) {
        lastBlinkTime = currentTime;
        ledState = !ledState;
        gpio_put(LED_R, ledState);
    }
}

int main() {
    gpio_init(LED_R);
    gpio_set_dir(LED_R, GPIO_OUT);

    gpio_set_irq_enabled_with_callback(BOTAO_A, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);
    gpio_set_irq_enabled_with_callback(BOTAO_B, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);

    npInit(LED_PIN);
    npClear();

    while (true) {
        piscarLED();
    }
}
