idf_component_register(SRCS "main.cpp" "button.cpp" "NatureLogo.cpp"
                    PRIV_REQUIRES nvs_flash M5GFX switchbot-nimble-client WiFiManager Echonet-lite
                    INCLUDE_DIRS ".")
