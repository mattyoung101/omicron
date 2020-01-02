# Important note: CLion ships with CMake 3.12 which supports parallel builds using the "j" option. Ubuntu only has
# CMake 3.10 as a package and given that I'm too lazy to build from source, on the Jetson for now we will have to cop
# slower builds. As soon as I'm not lazy or CMake 3.12 is released, use that instead.
cmake_minimum_required(VERSION 3.10)
project(omicam LANGUAGES C CXX CUDA)

# set variables
set(CMAKE_C_STANDARD 11)
set(CMAKE_CPP_STANDARD 17)
add_definitions(-D_GNU_SOURCE -D_FORTIFY_SOURCE=2)

# enable ASan and stack protection
set (SAFETY_FLAGS "-fstack-protector-strong -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer -static-libsan")
# these flags are used to enable the FPU and tune to the architecture of the jetson nano (this will break PC builds!)
# if anything related to NLopt or math in general breaks, use -mfpu=vfp and disable ffast-math
# source of gcc arguments for jetson: https://devtalk.nvidia.com/default/topic/1055706/jetson-nano/gcc-options-for-nano/
# note: using -flto to enable link time optimisations breaks using "-Og" and "-Ofast"
set (PERFORMANCE_FLAGS "-ffast-math -ffast-math -march=armv8-a+crypto -mcpu=cortex-a57+crypto")
# (we generate debug info in all modes since it adds no performance penalty and is good for debug)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wno-unused-parameter -g3 -pthread")
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-parameter -g3 -pthread")

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS} -Og ${SAFETY_FLAGS} ${PERFORMANCE_FLAGS}") # safety features and debug optimisation
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -Og ${SAFETY_FLAGS} ${PERFORMANCE_FLAGS}")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} -Ofast ${PERFORMANCE_FLAGS}") # full optimisation and no safety features
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_C_FLAGS} -Ofast ${PERFORMANCE_FLAGS}")

# find packages
find_package(OpenMP)
if (OPENMP_FOUND)
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
endif()
find_package(OpenCV REQUIRED)

# set includes
include_directories(lib)
include_directories(/usr/local/cuda/include)
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
        src/protobuf/RemoteDebug.pb.c src/protobuf/RemoteDebug.pb.h src/alloc_pool.c src/alloc_pool.h lib/map.c lib/map.h
        src/protobuf/FieldFile.pb.c src/protobuf/FieldFile.pb.h
        src/protobuf/UART.pb.c src/protobuf/UART.pb.h lib/kdtree.c lib/kdtree.h
        src/cuda/in_range.cu src/cuda/in_range.cuh
        lib/stb_image.h src/computer_vision.cpp src/computer_vision.hpp src/comms_uart.c src/comms_uart.h)

set_target_properties(omicam PROPERTIES POSITION_INDEPENDENT_CODE ON)
set_target_properties(omicam PROPERTIES CUDA_SEPARABLE_COMPILATION ON)

# link libraries
target_link_libraries(omicam pthread dl rt m SDL2 z turbojpeg nlopt ${OpenCV_LIBS})
