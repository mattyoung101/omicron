set(COMPONENT_ADD_INCLUDEDIRS .)
set(COMPONENT_PRIV_REQUIRES "main")

set(COMPONENT_SRCS  "astar.c"
					"goap.c"
					"main.c"
					"goap_omicron.c")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D LOG_LOCAL_LEVEL=ESP_LOG_DEBUG")
register_component()