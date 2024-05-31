#ifndef TCP_SERVER_HPP
#define TCP_SERVER_HPP

#include <functional>

#include <pico/stdlib.h>
#include <pico/cyw43_arch.h>

#include <lwip/pbuf.h>
#include <lwip/tcp.h>

struct Packet {
    uint8_t data_type{};
    uint8_t data[1023]{};

    uint16_t get_data_size() const {
        if(data_type == DATA_TYPE_FULL) {
            return 16u*16u*3u;
        } else if(data_type == DATA_TYPE_HALF) {
            return 16u*16u*3u/2u;
        } else {
            printf("invalid packet data_type!");
            return 0u;
        }
    } 

    const static uint8_t DATA_TYPE_FULL = 0x01;
    const static uint8_t DATA_TYPE_HALF = 0x02;
};

class TCPServer {
public:
    bool start(uint16_t port/*, uint8_t _timeout_time_s = 5u*/);
    void stop();

    inline bool is_connected() const { return client_pcb != nullptr; };
    inline bool is_running() const { return server_pcb != nullptr; };

    void set_packet_callback(const std::function<void(const Packet&)>& fn);

private:
    /*uint8_t timeout_time_s{};*/

    tcp_pcb *server_pcb{}, *client_pcb{};

    uint16_t in_received_count{};

    Packet in_packet{};

    std::function<void(const Packet&)> on_packet_ready{};

    static err_t server_sent(void *arg, tcp_pcb *tpcb, u16_t len);
    static err_t server_send_data(void *arg, tcp_pcb *tpcb, const void *data, uint16_t data_size);
    static err_t server_recv(void *arg, tcp_pcb *tpcb, pbuf *p, err_t err);
    /*static err_t server_poll(void *arg, tcp_pcb *tpcb);*/

    static void server_close(void *arg);
    static void server_err(void *arg, err_t err);

    static err_t server_connect_client(void *arg, tcp_pcb *client_pcb, err_t err);
    static void server_disconnect_client(void *arg);
};

#endif