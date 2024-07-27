#ifndef _WS2812B_HPP
#define _WS2812B_HPP

#include <cstdint>

enum struct LEDBrightness : uint8_t {
    Full    = 0u, // At least 8A power supply recommended (at 5V)
    Half    = 1u, // At least 4A power supply recommended (at 5V)
    Quarter = 2u, // At least 2A power supply recommended (at 5V)
    Eighth  = 3u  // At least 1A power supply recommended (at 5V)
};

class WS2812B {
public:
    WS2812B(uint32_t data_pin, LEDBrightness brightness);

    void set_pixel(uint32_t x, uint32_t y, uint8_t r, uint8_t g, uint8_t b);
    void fill(uint8_t r, uint8_t g, uint8_t b);

    void update_display();

private:
    uint8_t data[16u*16u*3u]{};
    uint8_t brightness_shift{};
};


#endif