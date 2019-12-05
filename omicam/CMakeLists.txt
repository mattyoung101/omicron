cmake_minimum_required(VERSION 3.13)
project(omicam)

# set variables
set(CMAKE_C_STANDARD 11)
set(CMAKE_CPP_STANDARD 17)
add_definitions(-D_GNU_SOURCE -D_FORTIFY_SOURCE=2 -D__ARM_NEON)

# enable ASan and stack protection
set (SAFETY_FLAGS "-fstack-protector-strong -fsanitize=address -fno-omit-frame-pointer -static-libsan")
# enable NEON (fast math is required due to a hardware bug), the march stuff may not be required but doesn't really matter
# if anything related to NLopt or math in general breaks, use -mfpu=vfp and disable ffast-math
set (NEON_FLAGS "-ffast-math -march=native -mtune=native") # TODO add back when on Jetson: -mfpu=neon -mfloat-abi=hard
# (we generate debug info in all modes since it adds no performance penalty and is good for debug)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wno-unused-parameter -g3 -pthread")
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-parameter -g3 -pthread")

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS} -Og ${SAFETY_FLAGS} ${NEON_FLAGS}") # safety features and debug optimiastion
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -Og ${SAFETY_FLAGS} ${NEON_FLAGS}")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} -Ofast ${NEON_FLAGS}") # full optimisation and no safety features
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_C_FLAGS} -Ofast ${NEON_FLAGS}")

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
        lib/stb_image.h src/computer_vision.cpp src/computer_vision.h)

# link libraries
target_link_libraries(omicam pthread dl rt m SDL2 z turbojpeg ${OpenCV_LIBS})
