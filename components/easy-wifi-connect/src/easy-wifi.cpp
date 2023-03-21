#include <esp_log.h>
#include <esp_err.h>

#include "easy-wifi.hpp"

const char EasyWiFi::TAG[] = "EasyWiFi";

int EasyWiFi::s_retry_num = 0;
wifi_config_t EasyWiFi::s_dpp_wifi_config = {};
EventGroupHandle_t EasyWiFi::s_dpp_event_group = nullptr;

void EasyWiFi::event_handler(void *arg, esp_event_base_t event_base,
					    int32_t event_id, void *event_data) {
	if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
		ESP_ERROR_CHECK(esp_supp_dpp_start_listen());
		ESP_LOGI(TAG, "Started listening for DPP Authentication");
	} else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
		if (s_retry_num < 5) {
			esp_wifi_connect();
			s_retry_num++;
			ESP_LOGI(TAG, "retry to connect to the AP");
		} else {
			xEventGroupSetBits(s_dpp_event_group, CONNECT_FAIL);
		}
		ESP_LOGI(TAG, "connect to the AP fail");
	} else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
		ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
		ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
		s_retry_num = 0;
		xEventGroupSetBits(s_dpp_event_group, CONNECTED);
	}
};

void EasyWiFi::dpp_enrollee_event_cb(esp_supp_dpp_event_t event, void *data) {
	switch (event) {
		case ESP_SUPP_DPP_URI_READY:
			if (data != NULL) {
				// esp_qrcode_config_t cfg = ESP_QRCODE_CONFIG_DEFAULT();

				ESP_LOGI(TAG, "Scan below QR Code to configure the enrollee:\n");
				//esp_qrcode_generate(&cfg, (const char *)data);

				ESP_LOG_BUFFER_HEXDUMP(TAG, data, 32, esp_log_level_t::ESP_LOG_ERROR);
				const char * qr_text = static_cast<const char *>(data);
				ESP_LOGI(TAG, "%s", qr_text);
			}
			break;
		case ESP_SUPP_DPP_CFG_RECVD:
			memcpy(&s_dpp_wifi_config, data, sizeof(s_dpp_wifi_config));
			esp_wifi_set_config(static_cast<wifi_interface_t>(ESP_IF_WIFI_STA), &s_dpp_wifi_config);
			ESP_LOGI(TAG, "DPP Authentication successful, connecting to AP : %s",
				    s_dpp_wifi_config.sta.ssid);
			s_retry_num = 0;
			esp_wifi_connect();
			break;
		case ESP_SUPP_DPP_FAIL:
			if (s_retry_num < 5) {
				ESP_LOGI(TAG, "DPP Auth failed (Reason: %s), retry...", esp_err_to_name((int)data));
				ESP_ERROR_CHECK(esp_supp_dpp_start_listen());
				s_retry_num++;
			} else {
				xEventGroupSetBits(s_dpp_event_group, AUTH_FAIL);
			}
			break;
		default:
			break;
	}
};

#ifdef CONFIG_ESP_DPP_LISTEN_CHANNEL
#define EXAMPLE_DPP_LISTEN_CHANNEL_LIST CONFIG_ESP_DPP_LISTEN_CHANNEL_LIST
#else
#define EXAMPLE_DPP_LISTEN_CHANNEL_LIST "6"
#endif

#ifdef CONFIG_ESP_DPP_BOOTSTRAPPING_KEY
#define EXAMPLE_DPP_BOOTSTRAPPING_KEY CONFIG_ESP_DPP_BOOTSTRAPPING_KEY
#else
#define EXAMPLE_DPP_BOOTSTRAPPING_KEY 0
#endif

#ifdef CONFIG_ESP_DPP_DEVICE_INFO
#define EXAMPLE_DPP_DEVICE_INFO CONFIG_ESP_DPP_DEVICE_INFO
#else
#define EXAMPLE_DPP_DEVICE_INFO 0
#endif

void EasyWiFi::wait_connection() {
	s_dpp_event_group = xEventGroupCreate();

	ESP_ERROR_CHECK(esp_netif_init());

	ESP_ERROR_CHECK(esp_event_loop_create_default());
	esp_netif_create_default_wifi_sta();

	ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
	ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));

	ESP_ERROR_CHECK(esp_supp_dpp_init(dpp_enrollee_event_cb));
	/* Currently only supported method is QR Code */
	ESP_ERROR_CHECK(esp_supp_dpp_bootstrap_gen(EXAMPLE_DPP_LISTEN_CHANNEL_LIST, DPP_BOOTSTRAP_QR_CODE,
									   EXAMPLE_DPP_BOOTSTRAPPING_KEY, EXAMPLE_DPP_DEVICE_INFO));

	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_ERROR_CHECK(esp_wifi_start());

	/* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
	 * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
	EventBits_t bits = xEventGroupWaitBits(s_dpp_event_group,
								    CONNECTED | CONNECT_FAIL | AUTH_FAIL,
								    pdFALSE,
								    pdFALSE,
								    portMAX_DELAY);

	/* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
	 * happened. */
	switch (bits) {
		case CONNECTED:
			ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
				    s_dpp_wifi_config.sta.ssid, s_dpp_wifi_config.sta.password);
			break;
		case CONNECT_FAIL:
			ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
				    s_dpp_wifi_config.sta.ssid, s_dpp_wifi_config.sta.password);
			break;
		case AUTH_FAIL:
			ESP_LOGI(TAG, "DPP Authentication failed after %d retries", s_retry_num);
			break;
		default:
			ESP_LOGE(TAG, "UNEXPECTED EVENT");
			break;
	}
};
