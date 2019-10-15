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
        lib/iniparser.h src/utils.h)

# FIXME: there's gotta be a better way to import the VideoCore SDK than this horseshit, but nothinge lse works
# as long as this dumbassery works I'm not going to change it since I hate configuring shitty build tools
target_link_libraries(omicam pthread dl rt m)
target_link_libraries(omicam /opt/vc/lib/libbrcmGLESv2.so)
target_link_libraries(omicam /opt/vc/lib/libbrcmEGL.so)
target_link_libraries(omicam /opt/vc/lib/libopenmaxil.so)
target_link_libraries(omicam /opt/vc/lib/libvchiq_arm.so)
target_link_libraries(omicam /opt/vc/lib/libvcos.so)
target_link_libraries(omicam /opt/vc/lib/libvcsm.so)
target_link_libraries(omicam /opt/vc/lib/libbcm_host.so)
target_link_libraries(omicam ${CMAKE_CURRENT_SOURCE_DIR}/lib/libomxcam.so)