/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include "esp_log.h"
#include "nvs_flash.h"

#include <M5GFX.h>

#include <switchbot_client.hpp>
#include <button.hpp>
#include <wifiManager.hpp>

#include <udp-socket.hpp>
#include <evps-object.hpp>

#include "NatureLogo.hpp"

#include "wifi_credential.h"
/**
#define SSID "your_ssid"
#define WIFI_PASSWORD "your_wifi_password"
 */

#include "switchbot_macaddr.h"
// #define SB_MAC "your_switchbot_mac_address" // "01:23:45:68:89:ab"

#define tag "SBC"

M5GFX display;
bool active = false;

extern "C" {
void app_main();
}

void app_main(void) {
	ESP_LOGI(tag, "Start");
	esp_err_t ret;

	/* Initialize NVS — it is used to store PHY calibration data */
	ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);

	display.init();
	display.startWrite();

	display.setBrightness(64);
	display.setColorDepth(lgfx::v1::color_depth_t::rgb565_2Byte);
	display.fillScreen(TFT_GREENYELLOW);

	static SwitchBotClient *sb = new SwitchBotClient(SB_MAC);

	// ATOMS3
	const uint8_t buttonPins[] = {41};
	static Button *button = new Button(buttonPins, sizeof(buttonPins));

	xTaskCreatePinnedToCore([](void *_) {
		while (true) {
			// Main loop
			vTaskDelay(100 / portTICK_RATE_MS);

			button->check(nullptr, [&](uint8_t pin) {
				switch (pin) {
					case 41:
						ESP_LOGI(tag, "released gpio 41");
						sb->press();
						break;
				}
			});
		}
	}, "ButtonCheck", 2048, nullptr, 1, nullptr, 1);

	vTaskDelay(3 * 1000 / portTICK_PERIOD_MS);

	ret = WiFi::Connect(SSID, WIFI_PASSWORD);
	if (!ret)
		ESP_LOGI("SBC", "IP: %s", WiFi::get_address());
	else
		display.fillScreen(TFT_RED);

	UDPSocket *udp = new UDPSocket();

	esp_ip_addr_t _multi;
	_multi.type		   = ESP_IPADDR_TYPE_V4;
	_multi.u_addr.ip4.addr = ipaddr_addr(ELConstant::EL_MULTICAST_IP);

	if (udp->beginReceive(ELConstant::EL_PORT)) {
		ESP_LOGI(tag, "EL.udp.begin successful.");
	} else {
		ESP_LOGI(tag, "Reseiver udp.begin failed.");	 // localPort
	}

	if (udp->beginMulticastReceive(&_multi)) {
		ESP_LOGI(tag, "EL.udp.beginMulticast successful.");
	} else {
		ESP_LOGI(tag, "Reseiver EL.udp.beginMulticast failed.");  // localPort
	}

	static NatureLogo *logo;

	logo = new NatureLogo(display.width(), display.height());
	logo->draw(display, active);
	

	Profile *profile = new Profile(1, 13);
	EVPS *evps	  = new EVPS(1);

	profile->add(evps);

	evps->update_remain_battery_ratio(0x32);
	evps->set_cb = [](ELObject * obj, uint8_t epc, uint8_t length, uint8_t* current_buffer, uint8_t* request_buffer) {
		ESP_LOGI(tag, "Request: %hx -> %hx", current_buffer[0], request_buffer[0]);

		EVPS::Mode current = static_cast<EVPS::Mode>(current_buffer[0]);
		EVPS::Mode request = static_cast<EVPS::Mode>(request_buffer[0]);

		// 充電と待機以外は無視
		if (request != EVPS::Mode::Charge && request != EVPS::Mode::Stanby) return ELObject::SetRequestResult::Reject;

		if (current == request) return ELObject::SetRequestResult::Accept;

		bool press_result = sb->press_async();
		ESP_LOGI(tag, "Press: %d", press_result);
		if (!press_result) return ELObject::SetRequestResult::Reject;

		active = (request == EVPS::Mode::Charge);
		logo->draw(display, active);
		current_buffer[0] = request_buffer[0];
		return ELObject::SetRequestResult::Accept;
	};

	Profile::el_packet_buffer_t buffer;
	while (true) {
		vTaskDelay(100 / portTICK_PERIOD_MS);
		profile->process_all_instance(udp, &buffer);
	}
}
