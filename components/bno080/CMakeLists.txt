set(COMPONENT_ADD_INCLUDEDIRS .)
set(COMPONENT_PRIV_REQUIRES "main")

set(COMPONENT_SRCS "sh2_hal_esp.c"
                   "sh2_SensorValue.c"
                   "sh2_util.c"
                   "sh2.c"
                   "shtp.c"
                   "worldTare.c")

register_component()
