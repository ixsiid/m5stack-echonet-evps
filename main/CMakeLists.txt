idf_component_register(SRCS "main.cpp" "button.cpp"
                    PRIV_REQUIRES nvs_flash switchbot-nimble-client WiFiManager Echonet-lite
                    INCLUDE_DIRS ".")
