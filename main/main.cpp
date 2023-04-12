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

// #include <switchbot_client.hpp>
#include <button.hpp>
#include <wifiManager.hpp>

#include <udp-socket.hpp>
#include <ECHONETlite-object.hpp>
#include <evps-object.hpp>
#include <battery-object.hpp>
#include <power-distribution-object.hpp>
#include <pv-object.hpp>
#include <water-heater-object.hpp>

#include "NatureLogo.hpp"

// #include "wifi_credential.h"
/**
#define SSID "your_ssid"
#define WIFI_PASSWORD "your_wifi_password"
 */

// #include "switchbot_macaddr.h"
// #define SB_MAC "your_switchbot_mac_address" // "01:23:45:68:89:ab"

#include <qrcodegen.hpp>
using namespace qrcodegen;

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

	display.setBrightness(1);
	display.setColorDepth(lgfx::v1::color_depth_t::rgb565_2Byte);
	display.fillScreen(TFT_WHITE);

	static const int size = QrCode::size;
	static const int size2 = 2 * size;
	static uint16_t buffer[QrCode::size * QrCode::size * 2 * 2] = {};
	WiFi::wait_connection([](const char *pairing_text) {
		ESP_LOGI(tag, "%s", pairing_text);
		QrCode qr = QrCode::encodeText(pairing_text);

		for (int y = 0; y < size; y++) {
			for (int x = 0; x < size; x++) {
				uint16_t c = qr.getModule(x, y) ? 0x0000 : 0xffff;

				buffer[(y * 2 + 0) * size2 + (x * 2 + 0)] = c;
				buffer[(y * 2 + 0) * size2 + (x * 2 + 1)] = c;
				buffer[(y * 2 + 1) * size2 + (x * 2 + 0)] = c;
				buffer[(y * 2 + 1) * size2 + (x * 2 + 1)] = c;
			}
		}

		display.setBrightness(48);
		display.pushImage(23, 23, size2, size2, buffer);
	});


	display.setBrightness(16);
	static NatureLogo *logo;

	logo = new NatureLogo(display.width(), display.height());
	logo->draw(display, active);

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

	Profile *profile	  = new Profile(1, 13);
	EVPS *evps		  = new EVPS(0);
	Battery *battery	  = new Battery(1);
	PowerDistribution *pd = new PowerDistribution(2);
	PV *pv			  = new PV(3);
	WaterHeater *wh	  = new WaterHeater(4);

	profile
	    ->add(evps)
	    ->add(battery)
	    ->add(pd)
	    ->add(pv)
	    ->add(wh);

	Profile::el_packet_buffer_t el_packet;
	while (true) {
		vTaskDelay(200 / portTICK_PERIOD_MS);
		profile->process_all_instance(udp, &el_packet);
	}
}
