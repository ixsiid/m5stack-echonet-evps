#pragma once

#include <esp_netif_ip_addr.h>

#include <lwip/err.h>
#include <lwip/sockets.h>
#include <lwip/sys.h>

class UDPSocket {
    private:
	int sock;

    public:
	UDPSocket();

	int write(const esp_ip_addr_t *addr, uint16_t port, const uint8_t *data, size_t length);

	bool beginReceive(uint16_t port);
	bool beginMulticastReceive(const esp_ip_addr_t *addr);
	int read(uint8_t *buffer, size_t buffer_length, esp_ip_addr_t *remote_addr);
};
