#include "tcp_server.hpp"
#include "packet.hpp"

// Based on the official picow_tcp_server.c example:
// https://github.com/raspberrypi/pico-examples/blob/master/pico_w/wifi/tcp_server/picow_tcp_server.c

bool TCPServer::start(uint16_t port, uint8_t _timeout_time_s) {
    printf("Starting server at %s:%u\n", ip4addr_ntoa(netif_ip4_addr(netif_list)), port);

    tcp_pcb *pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
    if (!pcb) {
        printf("TCPServer::start(uint16_t port): Failed to create the server pcb\n");
        return false;
    }

    if (tcp_bind(pcb, nullptr, port)) {
        printf("TCPServer::start(uint16_t port): Failed to bind to port: %u\n", port);
        return false;
    }
    
    server_pcb = tcp_listen_with_backlog(pcb, 1);
    if (!server_pcb) {
        printf("TCPServer::start(uint16_t port): Failed to listen for incoming clients.\n");

        tcp_close(pcb);

        return false;
    }

    timeout_time_s = _timeout_time_s;

    tcp_arg(server_pcb, this);
    tcp_accept(server_pcb, TCPServer::server_connect_client);

    return true;
}
void TCPServer::stop() {
    return server_close(this);
}

const uint8_t *TCPServer::get_ready_buffer() {
    if(is_buffer_ready) {
        is_buffer_ready = false;
        
        return in_buffer;
    } else {
        return nullptr;
    }
}

err_t TCPServer::server_send_data(void *arg, tcp_pcb *tpcb, const void *data, uint16_t data_size) {
    TCPServer *state = (TCPServer*)arg;

    cyw43_arch_lwip_check();

    if (tcp_write(tpcb, data, data_size, TCP_WRITE_FLAG_COPY) != ERR_OK) {
        printf("TCPServer::server_send_data(void *arg, tcp_pcb *tpcb, const void *data, uint16_t data_size): Server failed to send bytes!\n");
        return ERR_MEM;
    }

    return ERR_OK;
}

err_t TCPServer::server_recv(void *arg, tcp_pcb *tpcb, pbuf *p, err_t err) {
    TCPServer *state = (TCPServer*)arg;
    if (!p) {
        server_disconnect_client(arg);
        return ERR_OK; // Safely handled disconnect
    }

    cyw43_arch_lwip_check();

    if (p->tot_len > 0u) {
        uint16_t bytes_read = pbuf_copy_partial(p, (void*)(state->in_buffer + state->in_received_count), p->tot_len, 0);
        pbuf_free(p);

        if(bytes_read == 0u) {
            printf("TCPServer::server_recv(void *arg, tcp_pcb *tpcb, pbuf *p, err_t err): Server failed to read bytes!\n");

            return ERR_MEM;
        }
        
        //printf("Server read: %u bytes, err: %d\n", (uint32_t)bytes_read, err);

        state->in_received_count += bytes_read;

        tcp_recved(tpcb, bytes_read);

        // Whole incoming packet received
        if(state->in_received_count == get_data_type_size(state->in_buffer[0])) {
            state->in_received_count = 0u;
            state->is_buffer_ready = true;

            // Respond with "ACK" message
            const char* ack_message = "ACK";
            return server_send_data(arg, state->client_pcb, ack_message, strlen(ack_message) + 1u);
        }
    } else {
        pbuf_free(p);
    }

    return ERR_OK;
}

err_t TCPServer::server_poll(void *arg, tcp_pcb *tpcb) {
    TCPServer *state = (TCPServer*)arg;

    printf("TCPServer::server_poll(void *arg, tcp_pcb *tpcb): No communication with client for more than %u seconds!\n", state->timeout_time_s);

    server_disconnect_client(arg);

    return ERR_OK;
}

void TCPServer::server_close(void *arg) {
    TCPServer *state = (TCPServer*)arg;

    server_disconnect_client(arg);

    if (state->server_pcb) {
        tcp_arg(state->server_pcb, nullptr);
        tcp_close(state->server_pcb);

        state->server_pcb = nullptr;
    }
}

void TCPServer::server_err(void *arg, err_t err) {
    // Failsafe
    if(err == ERR_CLSD || err == ERR_ABRT || err == ERR_RST || err == ERR_TIMEOUT || err == ERR_RTE) {
        server_disconnect_client(arg);  // Safely handled disconnect
        return; 
    }
    
    printf("TCPServer::server_err(void *arg, err_t err): code:%d\n", (int)err);
    server_close(arg);
}

err_t TCPServer::server_connect_client(void *arg, tcp_pcb *client_pcb, err_t err) {
    TCPServer *state = (TCPServer*)arg;
    if (err != ERR_OK || client_pcb == nullptr) {
        printf("TCPServer::server_connect_client(void *arg, tcp_pcb *client_pcb, err_t err): Failed to connect with an incoming client!\n");
        return ERR_VAL;
    }

    state->client_pcb = client_pcb;
    
    tcp_arg(state->client_pcb, state);
    tcp_recv(state->client_pcb, TCPServer::server_recv);
    tcp_poll(state->client_pcb, TCPServer::server_poll, state->timeout_time_s * 2u);
    tcp_err(state->client_pcb, TCPServer::server_err);

    printf("Client connected\n");

    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);

    return ERR_OK;
}
void TCPServer::server_disconnect_client(void *arg) {
    TCPServer *state = (TCPServer*)arg;
    if(state->client_pcb == nullptr) {
        return;
    }
    
    tcp_arg(state->client_pcb, nullptr);
    tcp_poll(state->client_pcb, nullptr, 0);
    tcp_recv(state->client_pcb, nullptr);
    tcp_err(state->client_pcb, nullptr);

    if (tcp_close(state->client_pcb) != ERR_OK) {
        printf("TCPServer::server_disconnect_client(void *arg): tcp_close failed, calling tcp_abort\n");
        tcp_abort(state->client_pcb);
    }

    state->client_pcb = nullptr;

    printf("Client disconnected\n");

    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
}