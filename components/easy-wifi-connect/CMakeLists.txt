set(COMPONENT_ADD_INCLUDEDIRS
    src
    )
file(GLOB SRCS
     src/*.cpp
	src/*.c
     )
set(COMPONENT_SRCS ${SRCS})
set(COMPONENT_REQUIRES freertos esp_netif esp_wifi wpa_supplicant)

register_component()
