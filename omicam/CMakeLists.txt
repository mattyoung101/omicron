cmake_minimum_required(VERSION 3.13)
project(omicam C)

# set variables
set(CMAKE_C_STANDARD 11)
add_definitions(-D_GNU_SOURCE -D_FORTIFY_SOURCE=2 -D__ARM_NEON)

# -ffast-math -mfpu=neon -mfloat-abi=hard -march=armv7l

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -pthread")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS} -g -Og -fstack-protector-strong -fsanitize=address -fno-omit-frame-pointer -static-libsan -ffast-math -mfpu=neon -mfloat-abi=hard -march=armv7")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} -g -Ofast -mfpu=neon -mfloat-abi=hard -march=armv7 -mtune=arm1176jzf-s")

# the way we've set this up, debug is compiled to be OCD in its memory protection, with almost all sanitizers enabled
# and the stack protector enabled as well. this makes it very slow and resource intensive but easy to spot bugs.
# ideally, the program will be tested extensively in debug until its known to be secure, then run in release mode.
# relesae optimises purely for performance, with literally zero safety considerations (even going so far as to completely
# trust the FPU for all float calcs). ok nkw rjis, djis id eoif

# find packages
find_package(OpenMP)
if (OPENMP_FOUND)
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
endif()

# set includes
include_directories(lib)
include_directories(/opt/vc/include)

# add source files
add_executable(omicam src/main.c
        lib/log/log.c
        lib/log/log.h
        lib/iniparser/dictionary.c
        lib/iniparser/dictionary.h
        lib/iniparser/iniparser.c
        lib/iniparser/iniparser.h src/utils.h src/gpu_manager.c src/gpu_manager.h src/localisation.c src/localisation.h
        src/defines.h src/blob_detection.c src/blob_detection.h src/utils.c lib/raspicam/RaspiCamControl.c lib/raspicam/RaspiCamControl.h
        lib/raspicam/RaspiHelpers.c lib/raspicam/RaspiHelpers.h src/camera_manager.c src/camera_manager.h lib/raspicam/RaspiCommonSettings.c
        lib/raspicam/RaspiCommonSettings.h src/remote_debug.c src/remote_debug.h lib/DG_dynarr.h lib/rpa_queue.c lib/rpa_queue.h
        lib/nanopb/pb_common.c lib/nanopb/pb_decode.c lib/nanopb/pb_encode.c
        src/protobuf/RemoteDebug.pb.c src/protobuf/RemoteDebug.pb.h)

# link GPU libraries
file(GLOB GPU_LIBS "/opt/vc/lib/lib*.so")
target_link_libraries(omicam pthread dl rt m SDL2
        ${GPU_LIBS}
        /usr/lib/arm-linux-gnueabihf/libturbojpeg.so)
