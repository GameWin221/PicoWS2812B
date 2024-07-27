#include <pico/stdlib.h>
#include <pico/time.h>
#include <hardware/flash.h>
#include <hardware/sync.h>
#include <hardware/irq.h>

#include <string.h>

#include "packet.hpp"
#include "ws2812b.hpp"
#include "tcp_server.hpp"
#include "secrets.hpp" // WIFI_SSID "", WIFI_PASS ""

constexpr uint32_t DATA_PIN = 0u;
constexpr uint32_t SEVER_TIMEOUT_S = 8u;
constexpr uint16_t SEVER_PORT = 4242;

// 768KB offset
constexpr uint32_t FLASH_TARGET_OFFSET = (768u * 1024u);
static const uint8_t *flash_read_contents = (const uint8_t*)(XIP_BASE); // Adding the offset here makes read/write more inconsistent.

static repeating_timer_t flash_player_timer{};
static bool flash_player_running = false;

static uint16_t begin_frame_idx_inclusive{}; 
static uint16_t end_frame_idx_inclusive{}; 
static uint16_t time_interval_ms{};
static uint16_t current_frame_id{};

// Allows for better flash data packing and better flash wear. 
// Instead of using a whole sector per one frame, use one quarter of a sector per frame.
// Since one frame is 16*16*3 (768) bytes we only waste 25% of memory this way instead of 81.25%.
// It can be improved but it is good enough. We can store up to 1280 full 8bpp frames starting from 768KB offset.
static void write_flash_sector_quarter(uint32_t dst_sector_id, uint32_t dst_quarter_id, const uint8_t *data, uint32_t data_size) {
    if(data_size > FLASH_SECTOR_SIZE / 4u) {
        printf("write_flash_sector_quarter(uint32_t dst_sector_id, uint32_t dst_quarter_id, const uint8_t *data, uint32_t data_size) data_size cannot be larger than FLASH_SECTOR_SIZE / 4u\n");
        return;
    }

    if(dst_quarter_id > 3u) {
        printf("write_flash_sector_quarter(uint32_t dst_sector_id, uint32_t dst_quarter_id, const uint8_t *data, uint32_t data_size) dst_quarter_id msut be either 0, 1, 2 or 3\n");
        return;
    }

    const uint32_t quarter_size = FLASH_SECTOR_SIZE / 4;

    uint32_t dst_sector_offset = dst_sector_id * FLASH_SECTOR_SIZE;
    uint32_t dst_quarter_offset = dst_quarter_id * quarter_size;

    bool quarter_erased[4] = { true, true, true, true };

    for(uint32_t i{}; i < 4u; ++i) {
        for(uint32_t j{}; j < quarter_size; ++j) {
            if (flash_read_contents[FLASH_TARGET_OFFSET + dst_sector_offset + (i * quarter_size) + j] != 0xFF) {
                quarter_erased[i] = false;
                break;
            }
        }
    }

    uint32_t interrupts = save_and_disable_interrupts();

    if(quarter_erased[dst_quarter_id]) {
        flash_range_program(FLASH_TARGET_OFFSET + dst_sector_offset + dst_quarter_offset, data, data_size);
    } else {
        static uint8_t sector_copy[FLASH_SECTOR_SIZE];
        
        memcpy(sector_copy, &flash_read_contents[FLASH_TARGET_OFFSET + dst_sector_offset], FLASH_SECTOR_SIZE);
        
        flash_range_erase(FLASH_TARGET_OFFSET + dst_sector_offset, FLASH_SECTOR_SIZE);

        for(uint32_t i{}; i < 4u; ++i) {
            uint32_t src_quarter_offset = i * quarter_size;

            if(i != dst_quarter_id && !quarter_erased[i]) {
                // Restore old 3 quarters. No need to restore erased quarters.
                flash_range_program(FLASH_TARGET_OFFSET + dst_sector_offset + src_quarter_offset, &sector_copy[src_quarter_offset], quarter_size);
            } else {
                // Write new quarter data
                flash_range_program(FLASH_TARGET_OFFSET + dst_sector_offset + src_quarter_offset, data, data_size);
            }
        }
    }

    restore_interrupts(interrupts);
}

static bool flash_player_timer_callback(repeating_timer_t *rt) {
    WS2812B *led_matrix = (WS2812B*)rt->user_data;

    uint32_t current_frame_offset = (uint32_t)current_frame_id * FLASH_SECTOR_SIZE / 4u;

    for(uint32_t x{}; x < 16u; ++x) {
        for(uint32_t y{}; y < 16u; ++y) {
            uint32_t idx = y * 16u + x;

            led_matrix->set_pixel(x, y, 
                flash_read_contents[FLASH_TARGET_OFFSET + idx * 3u + 0u + current_frame_offset],
                flash_read_contents[FLASH_TARGET_OFFSET + idx * 3u + 1u + current_frame_offset],
                flash_read_contents[FLASH_TARGET_OFFSET + idx * 3u + 2u + current_frame_offset]
            );
        }
    }

    led_matrix->update_display();

    // Wrap back to the beginning
    if((++current_frame_id) > end_frame_idx_inclusive) {
        current_frame_id = begin_frame_idx_inclusive;
    }

    return true;
}

static void on_buffer_ready(const uint8_t *buf, WS2812B &led_matrix) {
    // Any incoming data will interrupt the currently playing flash player
    if(flash_player_running) {
        cancel_repeating_timer(&flash_player_timer);
        flash_player_running = false;
    }

    uint8_t buf_data_type = buf[0];

    switch(buf_data_type) {
        case DATA_TYPE_FULL: {
            const PacketFull *p = (const PacketFull*)buf;

            for(uint32_t x{}; x < 16u; ++x) {
                for(uint32_t y{}; y < 16u; ++y) {
                    uint32_t idx = y * 16u + x;

                    led_matrix.set_pixel(x, y, 
                        p->data[idx * 3u + 0u],
                        p->data[idx * 3u + 1u],
                        p->data[idx * 3u + 2u]
                    );
                }
            }

            led_matrix.update_display();
        }   break;

        case DATA_TYPE_HALF: {
            const PacketHalf *p = (const PacketHalf*)buf;

            for(uint32_t x{}; x < 16u; ++x) {
                for(uint32_t y{}; y < 16u; ++y) {
                    uint32_t idx = y * 16u + x;

                    uint8_t r = (idx % 2u == 0u) ? ((p->data[idx * 3u / 2u + 0u] & 0xf0) >> 4) :  (p->data[idx * 3u / 2u + 0u] & 0x0f);
                    uint8_t g = (idx % 2u == 0u) ?  (p->data[idx * 3u / 2u + 0u] & 0x0f)       : ((p->data[idx * 3u / 2u + 1u] & 0xf0) >> 4);
                    uint8_t b = (idx % 2u == 0u) ? ((p->data[idx * 3u / 2u + 1u] & 0xf0) >> 4) :  (p->data[idx * 3u / 2u + 1u] & 0x0f);

                    // Unpack from 4 bits to 8 bits
                    led_matrix.set_pixel(x, y, r << 4, g << 4, b << 4);
                }
            }

            led_matrix.update_display();
        }   break;

        case DATA_TYPE_WRITE_FLASH: {
            const PacketWriteFlash *p = (const PacketWriteFlash*)buf;

            uint32_t dst_sector = (uint32_t)p->frame_idx / 4u;
            uint32_t dst_quarter = (uint32_t)p->frame_idx % 4u;

            write_flash_sector_quarter(dst_sector, dst_quarter, p->data, 16u * 16u * 3u);
        } break;

        case DATA_TYPE_PLAY_FLASH: {
            const PacketPlayFlash *p = (const PacketPlayFlash*)buf;

            begin_frame_idx_inclusive = p->begin_frame_idx_inclusive;
            end_frame_idx_inclusive = p->end_frame_idx_inclusive;
            time_interval_ms = p->time_interval_ms;

            if(begin_frame_idx_inclusive > end_frame_idx_inclusive) {
                printf("main(): begin_frame_idx_inclusive cannot be greater than end_frame_idx_inclusive!\n");
                begin_frame_idx_inclusive = 0u;
                end_frame_idx_inclusive = 0u;
                
                break;
            }

            if(!add_repeating_timer_ms(-((int32_t)time_interval_ms), flash_player_timer_callback, &led_matrix, &flash_player_timer)) { 
                printf("main(): Failed to create flash_player_timer!\n");

                break;
            } else {
                flash_player_running = true;
            }
        }   break;

        default:
            printf("main(): Incorrect buf_data_type!\n");
            break;
    }
}

int main() {
    stdio_init_all();

    if (cyw43_arch_init()) {
        printf("main(): Failed to initialise cyw43_arch!\n");
        return 1;
    }
    
    cyw43_arch_enable_sta_mode();

    WS2812B led_matrix(DATA_PIN, LEDBrightness::Half);
    led_matrix.fill(64, 0, 0);
    led_matrix.update_display();

    printf("Connecting to Wi-Fi...\n");

    while(cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASS, CYW43_AUTH_WPA2_AES_PSK, 10000u)) {
        printf("Failed to connect to Wi-Fi. Retrying...\n");
    }

    printf("Connected to Wi-Fi.\n");

    led_matrix.fill(64, 64, 0);
    led_matrix.update_display();

    TCPServer server{};
    if(!server.start(SEVER_PORT, SEVER_TIMEOUT_S)) {
        printf("main(): Failed to start the server. Retrying..\n");

        led_matrix.fill(0, 64, 64);
        led_matrix.update_display();

        cyw43_arch_deinit();

        return 1;
    }
    
    printf("Server started successfully!\n");
    
    led_matrix.fill(0, 64, 0);
    led_matrix.update_display();

    while(server.is_running()) {  
        cyw43_arch_poll();
        
        // Handle packets from the main thread to avoid problems (e.g. writing to flash)
        const uint8_t *buf = server.get_ready_buffer();
        if(buf != nullptr) {
            on_buffer_ready(buf, led_matrix);
        }

        cyw43_arch_wait_for_work_until(make_timeout_time_ms(100));
    }

    cyw43_arch_deinit();
}