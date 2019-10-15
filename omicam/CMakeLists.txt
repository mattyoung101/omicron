cmake_minimum_required(VERSION 3.15)
project(omicam C)

set(CMAKE_C_STANDARD 11)

include_directories(lib)
include_directories(/opt/vc/include)
SET(CMAKE_CXX_FLAGS -pthread)

add_executable(omicam src/main.c
        lib/log.c
        lib/log.h
        lib/omxcam.h
        lib/omxcam_version.h
        lib/dictionary.c
        lib/dictionary.h
        lib/iniparser.c
        lib/iniparser.h)

target_link_libraries(omicam pthread)
target_link_libraries(omicam /opt/vc/lib/libopenmaxil.so)
target_link_libraries(omicam /opt/vc/lib/libvchiq_arm.so)
target_link_libraries(omicam /opt/vc/lib/libvcos.so)
target_link_libraries(omicam /opt/vc/lib/libvcsm.so)
target_link_libraries(omicam /opt/vc/lib/libbcm_host.so)
target_link_libraries(omicam ${CMAKE_CURRENT_SOURCE_DIR}/lib/libomxcam.so)