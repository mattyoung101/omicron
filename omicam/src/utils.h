#pragma once

/** if the key exists in the INI file, puts it into the omxcam settings struct **/
#define INI_LOAD_INT(key) if (iniparser_find_entry(config, "VideoSettings:" #key)) { \
    settings.camera.key = iniparser_getint(config, "VideoSettings:" #key, -1); \
}
