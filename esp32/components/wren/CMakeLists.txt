set(COMPONENT_ADD_INCLUDEDIRS .)
set(COMPONENT_PRIV_REQUIRES "main")

set(COMPONENT_SRCS "wren_compiler.c"
                   "wren_core.c"
                   "wren_debug.c"
                   "wren_primitive.c"
                   "wren_utils.c"
                   "wren_value.c"
                   "wren_vm.c"
                   "wren_opt_meta.c"
                   "wren_opt_random.c")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D LOG_LOCAL_LEVEL=ESP_LOG_DEBUG")
register_component()