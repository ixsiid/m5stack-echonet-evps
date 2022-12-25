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

#include <switchbot_client.hpp>
#include <button.hpp>
#include <wifiManager.hpp>
#include <udp-socket.hpp>
#include <evps-object.hpp>

#include "wifi_credential.h"
/**
#define SSID "your_ssid"
#define WIFI_PASSWORD "your_wifi_password"
 */

#include "switchbot_macaddr.h"
// #define SB_MAC "your_switchbot_mac_address" // "01:23:45:68:89:ab"

#define tag "SBC"

extern "C" {
void app_main();
}

void app_main(void) {
	/* Initialize NVS — it is used to store PHY calibration data */
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);

	static SwitchBotClient *sb = new SwitchBotClient(SB_MAC);

	/*
	while (true) {
		sb->press();
		vTaskDelay(30 * 1000 / portTICK_PERIOD_MS);
	}
	*/
	xTaskCreatePinnedToCore([](void *_sb) {
		// Nimble使うと39（Button A）が干渉するっぽい
		const uint8_t buttonPins[] = {37, 38};

		Button *button		= new Button(buttonPins, 2);
		SwitchBotClient *sb = (SwitchBotClient *)_sb;

		while (true) {
			// Main loop
			vTaskDelay(1);

			button->check(nullptr, [&](uint8_t pin) {
				switch (pin) {
					case 37:
						ESP_LOGI(tag, "released gpio 37");
						sb->press();
						break;
					case 38:
						ESP_LOGI(tag, "released gpio 38");
						sb->push();
						break;
					case 39:
						ESP_LOGI(tag, "released gpio 39");
						sb->pull();
						break;
				}
			});
		}
	}, "ButtonCheck", 4096, sb, 1, nullptr, 1);

	vTaskDelay(3 * 1000 / portTICK_PERIOD_MS);

	ret = WiFi::Connect(SSID, WIFI_PASSWORD);
	if (!ret) ESP_LOGI("SBC", "IP: %s", WiFi::get_address());

	UDPSocket *udp = new UDPSocket();

	esp_ip_addr_t _multi;
	_multi.type		   = ESP_IPADDR_TYPE_V4;
	_multi.u_addr.ip4.addr = ipaddr_addr(EL_MULTICAST_IP);

	if (udp->beginReceive(EL_PORT)) {
		ESP_LOGI(tag, "EL.udp.begin successful.");
	} else {
		ESP_LOGI(tag, "Reseiver udp.begin failed.");	 // localPort
	}

	if (udp->beginMulticastReceive(&_multi)) {
		ESP_LOGI(tag, "EL.udp.beginMulticast successful.");
	} else {
		ESP_LOGI(tag, "Reseiver EL.udp.beginMulticast failed.");  // localPort
	}

	int packetSize;

	Profile *profile = new Profile();
	EVPS *evps	  = new EVPS(1);

	evps->set_update_mode_cb([](EVPS_Mode current_mode, EVPS_Mode request_mode) {
		EVPS_Mode next_mode;
		switch (request_mode) {
			case EVPS_Mode::Charge:
				next_mode = EVPS_Mode::Charge;
				break;
			case EVPS_Mode::Stanby:
			case EVPS_Mode::Stop:
			case EVPS_Mode::Auto:
				next_mode = EVPS_Mode::Stanby;
				break;
			default:
				return EVPS_Mode::Unacceptable;
		}

		// モード繊維がないため、何もしない
		if (current_mode == next_mode) return current_mode;

		bool press_result = sb->press_async();
		ESP_LOGI(tag, "Press: %d", press_result);;
		return press_result ? next_mode : current_mode;
	});

	uint8_t rBuffer[EL_BUFFER_SIZE];
	elpacket_t *p = (elpacket_t *)rBuffer;
	uint8_t *epcs = rBuffer + sizeof(elpacket_t);

	esp_ip_addr_t remote_addr;
	// esp_ip_addr_t multi_addr;
	// multi_addr.u_addr.ip4.addr = esp_ip4addr_aton(EL_MULTICAST_IP);
	// ESP_LOGI("Multicast addr", "%x", multi_addr.u_addr.ip4.addr);

	while (true) {
		vTaskDelay(100 / portTICK_PERIOD_MS);

		packetSize = udp->read(rBuffer, EL_BUFFER_SIZE, &remote_addr);

		if (packetSize > 0) {
			if (epcs[0] == 0xda) {
			ESP_LOGI("EL Packet", "(%04x, %04x) %04x-%02x -> %04x-%02x: ESV %02x [%02x]",
				    p->_1081, p->packet_id,
				    p->src_device_class, p->src_device_id,
				    p->dst_device_class, p->dst_device_id,
				    p->esv, p->epc_count);
			ESP_LOG_BUFFER_HEXDUMP("EL Packet", epcs, packetSize - sizeof(elpacket_t), ESP_LOG_INFO);
			}

			uint8_t epc_res_count = 0;
			switch (p->dst_device_class) {
				case 0xf00e:
					epc_res_count = profile->process(p, epcs);
					if (epc_res_count > 0) {
						profile->send(udp, &remote_addr);
						continue;
					}
					break;
				case 0x7e02:
					epc_res_count = evps->process(p, epcs);
					if (epc_res_count > 0) {
						evps->send(udp, &remote_addr);
						continue;
					}
					break;
			}
		}
	}
}
