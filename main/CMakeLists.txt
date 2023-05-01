idf_component_register(SRCS "main.cpp" "button.cpp"
                    PRIV_REQUIRES nvs_flash switchbot-nimble-client WiFiManager esp-ECHONET-Lite simple_ws2812_rmt_esp-idf
#                   PRIV_REQUIRES M5GFX
                    INCLUDE_DIRS ".")
