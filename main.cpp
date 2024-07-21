#include <pico/stdlib.h>
#include <pico/time.h>

#include "ws2812b.hpp"
#include "tcp_server.hpp"
#include "secrets.hpp" // WIFI_SSID "", WIFI_PASS ""

#define DATA_PIN 0u

static WS2812B* led_matrix{};

static void on_packet_ready(const Packet& packet) {
    //printf("Packet header:\n");
    //printf("\tdata_size: %u\n", (uint32_t)packet.get_data_size());
    //printf("\tdata_type: %u\n", (uint32_t)packet.data_type);

    uint32_t idx{};

    switch(packet.data_type) {
        case Packet::DATA_TYPE_FULL:
            for(uint32_t x{}; x < 16u; ++x) {
                for(uint32_t y{}; y < 16u; ++y) {
                    idx = y * 16u + x;

                    led_matrix->set_pixel(x, y, 
                        packet.data[idx * 3u + 0u],
                        packet.data[idx * 3u + 1u],
                        packet.data[idx * 3u + 2u]
                    );
                }
            }
            break;
        case Packet::DATA_TYPE_HALF:
            for(uint32_t x{}; x < 16u; ++x) {
                for(uint32_t y{}; y < 16u; ++y) {
                    idx = y * 16u + x;

                    uint8_t r = (idx % 2u == 0u) ? ((packet.data[idx * 3u / 2u + 0u] & 0xf0) >> 4) :  (packet.data[idx * 3u / 2u + 0u] & 0x0f);
                    uint8_t g = (idx % 2u == 0u) ?  (packet.data[idx * 3u / 2u + 0u] & 0x0f)       : ((packet.data[idx * 3u / 2u + 1u] & 0xf0) >> 4);
                    uint8_t b = (idx % 2u == 0u) ? ((packet.data[idx * 3u / 2u + 1u] & 0xf0) >> 4) :  (packet.data[idx * 3u / 2u + 1u] & 0x0f);

                    // Unpack from 4 bits to 8 bits
                    r *= 16u;
                    g *= 16u;
                    b *= 16u;

                    led_matrix->set_pixel(x, y, r, g, b);
                }
            }
            break;
        default:
            printf("Incorrect packet.data_type!\n");
            break;
    }

    led_matrix->update_display();
}

int main() {
    stdio_init_all();

    if (cyw43_arch_init()) {
        printf("Failed to initialise cyw43_arch!\n");
        return 1;
    }
    
    cyw43_arch_enable_sta_mode();

    WS2812B led_controller(DATA_PIN, LEDBrightness::Half);
    led_matrix = &led_controller;
    led_matrix->fill(64, 0, 0);
    led_matrix->update_display();

    printf("Connecting to Wi-Fi...\n");

    while(cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASS, CYW43_AUTH_WPA2_AES_PSK, 10000u)) {
        printf("Failed to connect to Wi-Fi. Retrying...\n");
    }

    printf("Connected to Wi-Fi.\n");

    led_matrix->fill(64, 64, 0);
    led_matrix->update_display();

    TCPServer server{};
    server.set_packet_callback(on_packet_ready);

    if(!server.start(4242)) {
        printf("Failed to start the server. Retrying..\n");

        led_matrix->fill(64, 0, 64);
        led_matrix->update_display();

        cyw43_arch_deinit();

        return 1;
    }

    printf("Server started successfully!\n");

    led_matrix->fill(0, 64, 0);
    led_matrix->update_display();

    while(server.is_running()) {  
        cyw43_arch_poll();
        cyw43_arch_wait_for_work_until(make_timeout_time_ms(100));
    }

    cyw43_arch_deinit();
}