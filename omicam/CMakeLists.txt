cmake_minimum_required(VERSION 3.13)
project(omicam C)

# set variables
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS " ${CMAKE_C_FLAGS} -Wall -Wextra -pthread")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS} -g -Og")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} -g -Ofast -mfpu=vfp -mfloat-abi=hard -march=armv6zk -mtune=arm1176jzf-s")

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
        lib/omxcam/omxcam.h
        lib/omxcam/omxcam_version.h
        lib/iniparser/dictionary.c
        lib/iniparser/dictionary.h
        lib/iniparser/iniparser.c
        lib/iniparser/iniparser.h src/utils.h src/gpu_manager.c src/gpu_manager.h src/localisation.c src/localisation.h
        src/defines.h src/blob_detection.c src/blob_detection.h src/utils.c)

# link GPU libraries
# FIXME: there's gotta be a better way to import the VideoCore SDK than this horseshit, but nothinge lse works
# as long as this dumbassery works I'm not going to change it since I hate configuring shitty build tools
# try to find a way to static link these maybe
target_link_libraries(omicam pthread dl rt m
        /opt/vc/lib/libbrcmGLESv2.so
        /opt/vc/lib/libbrcmEGL.so
        /opt/vc/lib/libopenmaxil.so
        /opt/vc/lib/libvchiq_arm.so
        /opt/vc/lib/libvcos.so
        /opt/vc/lib/libvcsm.so
        /opt/vc/lib/libbcm_host.so
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/libomxcam.so)