set(COMPONENT_ADD_INCLUDEDIRS .)

set(COMPONENT_SRCS "pb_common.c"
                   "pb_decode.c"
                   "pb_encode.c"
                   "wirecomms.pb.c"
                   "bluetooth.pb.c"
                   "UART.pb.c")

register_component()
