set(COMPONENT_ADD_INCLUDEDIRS .)
set(COMPONENT_PRIV_REQUIRES "main")

set(COMPONENT_SRCS "sh2_hal_esp.c"
                   "sh2_SensorValue.c"
                   "sh2_util.c"
                   "sh2.c"
                   "shtp.c"
                   "worldTare.c")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D LOG_LOCAL_LEVEL=ESP_LOG_DEBUG")
register_component()
