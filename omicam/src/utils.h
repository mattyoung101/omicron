#pragma once

// Misc macros

/** if the key exists in the INI file, puts it into the omxcam settings struct as an integer **/
#include <GLES2/gl2.h>

#define INI_LOAD_INT(key) if (iniparser_find_entry(config, "VideoSettings:" #key)) { \
    log_trace("Have int key: " #key); \
    settings.camera.key = iniparser_getint(config, "VideoSettings:" #key, -1); \
} else { \
    log_trace("Using default value for int key: " # key); \
}

/** if the key exists in the INI file, puts it into the omxcam settings struct as a boolean **/
#define INI_LOAD_BOOL(key) if (iniparser_find_entry(config, "VideoSettings:" #key)) { \
    log_trace("Have bool key: " #key); \
    settings.camera.key = iniparser_getboolean(config, "VideoSettings:" #key, false); \
} else { \
    log_trace("Using default value for bool key: " # key); \
}

/** locks a pthread semaphore, then runs the provided code and unlocks it again **/
#define PTHREAD_SEM_RUN(sem, code) if (pthread_mutex_trylock(sem)){ \
    code; \
    pthread_mutex_unlock(sem); \
}

#define UPDATE_UNIFORM(location, dataName) if (location != -1){ \
    log_trace("Updating uniform at %d to the following: [%.2f, %.2f, %.2f]", location, dataName[0], dataName[1], dataName[2]); \
    glUniform3fv(location, 1, dataName); /* count is 1 because it's one vec3 */ \
    check_gl_error(); \
}  else { \
    log_warn("Failed to update uniform at location: " #location); \
}

#define GET_UNIFORM_LOCATION(uniform, name) do { uniform = glGetUniformLocation(shaderProgram, name); \
    check_gl_error(); \
    if (uniform == -1){ \
        log_error("Failed to get uniform location of " name); \
    } } while (0);

#define GCC_UNUSED __attribute__((unused))

//#define DYAD_CHECK_UPDATE if (dyad_getStreamCount() > 0) { dyad_update(); }

/** first 8 bits of unsigned 16 bit int **/
#define HIGH_BYTE_16(num) ((uint8_t) ((num >> 8) & 0xFF))
/** second 8 bits of unsigned 16 bit int **/
#define LOW_BYTE_16(num)  ((uint8_t) ((num & 0xFF)))
/** unpack two 8 bit integers into a 16 bit integer **/
#define UNPACK_16(a, b) ((uint16_t) ((a << 8) | b))

/** gets the timestamp in milliseconds **/
double utils_get_millis();
/** get the last EGL error as a descriptive string **/
const char *eglGetErrorStr();
/** GL enum to error **/
char *glErrorStr(GLenum error);
