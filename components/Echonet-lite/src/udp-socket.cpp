#include "udp-socket.hpp"

#include "lwip/sockets.h"
#include "lwip/priv/sockets_priv.h"
#include "lwip/api.h"
#include "lwip/sys.h"
#include "lwip/tcp.h"
#include "lwip/raw.h"
#include "lwip/udp.h"

#include <esp_log.h>

#define TAG "UDP_SOCKET"

UDPSocket::UDPSocket() {
	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
	if (sock < 0) {
		ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
	}
}

int UDPSocket::write(const esp_ip_addr_t *addr, uint16_t port, const uint8_t *data, size_t length) {
	struct sockaddr_in dest;
	dest.sin_addr.s_addr = addr->u_addr.ip4.addr;

	dest.sin_port	 = htons(port);
	dest.sin_family = AF_INET;

	int len = sendto(sock, data, length, 0, (struct sockaddr *)&dest, sizeof(dest));
	// ESP_LOGI(TAG, "sock %d, data length %d, %8x:%d", sock, length, dest.sin_addr.s_addr, dest.sin_port);
	// ESP_LOG_BUFFER_HEXDUMP(TAG, data, length, ESP_LOG_INFO);
	if (len < 0) {
		ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
	} else {
		// ESP_LOGI(TAG, "Message sent");
	}

	return len;
}

bool UDPSocket::beginMulticastReceive(const esp_ip_addr_t *addr) {
	struct ip_mreq mreq		 = {};
	mreq.imr_interface.s_addr = INADDR_ANY;
	mreq.imr_multiaddr.s_addr = addr->u_addr.ip4.addr;

	int opted = setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void *)&mreq, sizeof(mreq));

	return opted ? false : true;
}

// 受信用メソッド
bool UDPSocket::beginReceive(uint16_t port) {
	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
	if (sock < 0) {
		ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
	}

	// Set timeout
	struct timeval timeout;
	timeout.tv_sec	 = 10;
	timeout.tv_usec = 0;
	// timeout.tv_nsec = 0;
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

	struct sockaddr_in addr;
	addr.sin_family	 = AF_INET;
	addr.sin_port		 = htons(port);
	addr.sin_addr.s_addr = INADDR_ANY;

	int binded = bind(sock, (struct sockaddr *)&addr, sizeof(addr));

	return binded ? false : true;
}

int UDPSocket::read(uint8_t *buffer, size_t buffer_length, esp_ip_addr_t *remote_addr) {
	struct sockaddr_storage source_addr;  // Large enough for both IPv4 or IPv6
	socklen_t socklen = sizeof(source_addr);

	int len = recvfrom(sock, buffer, buffer_length, 0, (struct sockaddr *)&source_addr, &socklen);

	remote_addr->u_addr.ip4.addr = ((struct sockaddr_in *)&source_addr)->sin_addr.s_addr;

	// Error occurred during receiving
	if (len < 0) {
		ESP_LOGE(TAG, "recvfrom failed: errno %d", errno);
	} else {
		// Data received
		buffer[len] = 0;  // Null-terminate whatever we received and treat like a string
		// ESP_LOGI(TAG, "Received %d bytes from host", len);
		// ESP_LOGI(TAG, "%s", rx_buffer);
	}

	return len > 0 ? len : 0;
}
