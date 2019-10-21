#pragma once

// Misc macros

/** if the key exists in the INI file, puts it into the omxcam settings struct as an integer **/
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

/** gets the timestamp in milliseconds **/
double utils_get_millis();
