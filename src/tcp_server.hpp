#ifndef _TCP_SERVER_HPP
#define _TCP_SERVER_HPP

#include <pico/stdlib.h>
#include <pico/cyw43_arch.h>

#include <lwip/pbuf.h>
#include <lwip/tcp.h>

class TCPServer {
public:
    bool start(uint16_t port, uint8_t _timeout_time_s);
    void stop();

    inline bool is_connected() const { return client_pcb != nullptr; };
    inline bool is_running() const { return server_pcb != nullptr; };

    const uint8_t *get_ready_buffer();

private:
    uint8_t timeout_time_s{};

    tcp_pcb *server_pcb{}, *client_pcb{};

    uint16_t in_received_count{};
    uint8_t in_buffer[2048];

    bool is_buffer_ready{};

    static err_t server_send_data(void *arg, tcp_pcb *tpcb, const void *data, uint16_t data_size);
    static err_t server_recv(void *arg, tcp_pcb *tpcb, pbuf *p, err_t err);
    static err_t server_poll(void *arg, tcp_pcb *tpcb);

    static void server_close(void *arg);
    static void server_err(void *arg, err_t err);

    static err_t server_connect_client(void *arg, tcp_pcb *client_pcb, err_t err);
    static void server_disconnect_client(void *arg);
};

#endif