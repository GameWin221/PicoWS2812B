#include "ws2812b.hpp"
#include "ws2812b.pio.h"

#include <hardware/pio.h>

// PIO interface based on the example: https://github.com/raspberrypi/pico-examples/blob/master/pio/ws2812/ws2812.c

WS2812B::WS2812B(uint32_t data_pin, LEDBrightness brightness) : brightness_shift((uint8_t)brightness) {
    uint32_t offset = pio_add_program(pio0, &ws2812_program);

    ws2812_program_init(pio0, pio_claim_unused_sm(pio0, true), offset, data_pin, 800000.0f, false);
}

void WS2812B::set_pixel(uint32_t x, uint32_t y, uint8_t r, uint8_t g, uint8_t b) {
    // assume (x:0, y:0) is the lower-left corner
    uint32_t i = x * 16u + ((x % 2u == 1u) ? (y) : (15u - y)); 

    data[i * 3u + 0u] = g;
    data[i * 3u + 1u] = r;
    data[i * 3u + 2u] = b;
}
void WS2812B::fill(uint8_t r, uint8_t g, uint8_t b) {
    for(uint32_t i{}; i < 16u*16u; ++i) {
        data[i * 3u + 0u] = g;
        data[i * 3u + 1u] = r;
        data[i * 3u + 2u] = b;
    }
}

void WS2812B::update_display() {
    uint32_t color{};

    for(uint32_t i{}; i < 16u*16u; ++i) {
        color = 
            (((uint32_t)(data[i * 3u + 0u] >> brightness_shift)) << 24) |
            (((uint32_t)(data[i * 3u + 1u] >> brightness_shift)) << 16) |
            (((uint32_t)(data[i * 3u + 2u] >> brightness_shift)) << 8);

        pio_sm_put_blocking(pio0, 0u, color);
    }

    sleep_us(50);
}