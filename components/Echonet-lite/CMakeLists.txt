set(COMPONENT_ADD_INCLUDEDIRS
    src
    )
file(GLOB SRCS
     src/*.cpp
	src/*.c
     )
set(COMPONENT_SRCS ${SRCS})
set(COMPONENT_REQUIRES esp_netif lwip)


register_component()
