# Important note: CLion ships with CMake 3.12 which supports parallel builds using the "j" option. Ubuntu only has
# CMake 3.10 as a package and given that I'm too lazy to build from source, on the Jetson for now we will have to cop
# slower builds. As soon as I'm not lazy or CMake 3.12 is released, use that instead.
cmake_minimum_required(VERSION 3.10)
project(omicam LANGUAGES C CXX)

# set variables
set(CMAKE_C_STANDARD 11)
set(CMAKE_CPP_STANDARD 17)
add_definitions(-D_GNU_SOURCE -D_FORTIFY_SOURCE=2 -DMATHC_USE_DOUBLE_FLOATING_POINT)

# enable ASan and stack protection
set (SAFETY_FLAGS "-fstack-protector-strong -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer -static-libsan")
# these flags tune Omicam builds specifically for the platform it was compiled on, and enable SSE3 instructions
# note: using -flto to enable link time optimisations breaks using "-Og" and "-Ofast", but should be used in future
set (PERFORMANCE_FLAGS "-march=native -mtune=native -msse -msse3")
# (we generate debug info in all modes since it adds no performance penalty and is good for debug)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wno-unused-parameter -g3 -pthread")
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-parameter -g3 -pthread")

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS} -O0 ${SAFETY_FLAGS}") # safety features and debug optimisation
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -O0 ${SAFETY_FLAGS}")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} -O3 ${PERFORMANCE_FLAGS}") # full optimisation and no safety features
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_C_FLAGS} -O3 ${PERFORMANCE_FLAGS}")

set(CUDA_NVCC_FLAGS "${CUDA_NVCC_FLAGS} -lineinfo -O3")

# find packages
find_package(OpenCV REQUIRED)

# set includes
include_directories(lib)
include_directories(${OpenCV_INCLUDE_DIRS})

# add source files
# probably not pragmatic CMake but screw you, this is a build script, it just builds my shit alright
add_executable(omicam src/main.c
        lib/log/log.c
        lib/log/log.h
        lib/iniparser/dictionary.c
        lib/iniparser/dictionary.h
        lib/iniparser/iniparser.c
        lib/iniparser/iniparser.h src/utils.h src/localisation.c src/localisation.h
        src/defines.h src/utils.c
        src/remote_debug.c src/remote_debug.h lib/DG_dynarr.h lib/rpa_queue.c lib/rpa_queue.h
        lib/nanopb/pb_common.c lib/nanopb/pb_decode.c lib/nanopb/pb_encode.c
        src/protobuf/RemoteDebug.pb.c src/protobuf/RemoteDebug.pb.h lib/map.c lib/map.h
        src/protobuf/FieldFile.pb.c src/protobuf/FieldFile.pb.h lib/stb_image_write.h
        src/protobuf/UART.pb.c src/protobuf/UART.pb.h src/movavg.c src/movavg.h lib/mathc.c lib/mathc.h
        lib/stb_image.h src/computer_vision.cpp src/computer_vision.hpp src/comms_uart.c src/comms_uart.h)

set_target_properties(omicam PROPERTIES POSITION_INDEPENDENT_CODE ON)
set_target_properties(omicam PROPERTIES CUDA_SEPARABLE_COMPILATION ON)

# link libraries
target_link_libraries(omicam pthread dl rt m z turbojpeg nlopt ${OpenCV_LIBS})
