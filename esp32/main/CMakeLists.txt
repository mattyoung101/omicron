# Edit following two lines to set component requirements (see docs)
set(COMPONENT_REQUIRES )
set(COMPONENT_PRIV_REQUIRES "nanopb" "libraries")

set(COMPONENT_SRCS "main.c"
                   "defines.c" 
                   "utils.c"
                   "comms_i2c.c"
                   "pid.c"
                   "fsm.c"
                   "states_attack.c"
                   "states_defence.c"
                   "states_general.c"
                   "movavg.c"
                   "comms_bluetooth.c"
                   "bluetooth_manager.c"
                   "comms_uart.c"
                   "path_following.c"
                   "lsavoid.c"
                   "buzzer.c"
                   "avoid.c"
                   "motor.c"
                   )
set(COMPONENT_ADD_INCLUDEDIRS .)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D LOG_LOCAL_LEVEL=ESP_LOG_DEBUG")

register_component()
