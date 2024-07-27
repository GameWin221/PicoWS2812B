#ifndef _PACKET_HPP
#define _PACKET_HPP

#include <cstdint>
#include <cstdio>

constexpr uint8_t DATA_TYPE_FULL = 0x01;
constexpr uint8_t DATA_TYPE_HALF = 0x02;
constexpr uint8_t DATA_TYPE_WRITE_FLASH = 0x03;
constexpr uint8_t DATA_TYPE_PLAY_FLASH = 0x04;

// Immediately show a single frame of 8bpp (Full) RGB data.
struct PacketFull {
    uint8_t data_type = DATA_TYPE_FULL;
    uint8_t data[16*16*3];
};

// Immediately show a single frame of 4bpp (Half) RGB data.
struct PacketHalf {
    uint8_t data_type = DATA_TYPE_HALF;
    uint8_t data[16*16*3/2];
};

// Writes a single frame of 8bpp (Full) RGB data.
struct PacketWriteFlash {
    uint8_t data_type = DATA_TYPE_WRITE_FLASH;
    uint16_t frame_idx;
    uint8_t data[16*16*3];
};

// Plays the stored frames in flash.
// The display will keep playing the frames until it receives a new packet.
// The TCP connection can be safely closed and it will keep playing the frames by itself.
struct PacketPlayFlash {
    uint8_t data_type = DATA_TYPE_PLAY_FLASH;
    uint16_t begin_frame_idx_inclusive;
    uint16_t end_frame_idx_inclusive;
    uint16_t time_interval_ms;  // Time measured in milliseconds between the starts of continous frames.
};

static uint16_t get_data_type_size(uint8_t data_type) {
    switch(data_type) {
        case DATA_TYPE_FULL:        return sizeof(PacketFull);
        case DATA_TYPE_HALF:        return sizeof(PacketHalf);
        case DATA_TYPE_WRITE_FLASH: return sizeof(PacketWriteFlash); 
        case DATA_TYPE_PLAY_FLASH:  return sizeof(PacketPlayFlash);      
        default:
            printf("get_data_type_size(uint8_t data_type): Invalid packet data_type!");
            return 0u;
    }
}

#endif