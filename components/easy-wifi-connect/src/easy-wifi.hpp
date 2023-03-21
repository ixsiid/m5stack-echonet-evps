#pragma once

#include <cstring>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>

#include <esp_netif.h>
#include <esp_wifi.h>
#include <esp_dpp.h>

class EasyWiFi {
    private:
	static const char TAG[9];
	static void event_handler(void *arg, esp_event_base_t event_base,
						 int32_t event_id, void *event_data);
	static void dpp_enrollee_event_cb(esp_supp_dpp_event_t event, void *data);

	static int s_retry_num;
	static wifi_config_t s_dpp_wifi_config;
	static EventGroupHandle_t s_dpp_event_group;

	static const EventBits_t	CONNECTED	   = 0b001;
	static const EventBits_t CONNECT_FAIL = 0b010;
	static const EventBits_t AUTH_FAIL	   = 0b100;

	EasyWiFi();
    public:
	static void wait_connection();
};
