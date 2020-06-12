#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include "defines.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Begins recording a replay */
void replay_record(void);
/** Flushes any active replays to disk */
void replay_close(void);
/** @returns the current status of the replay system (eg whether recording, playing back, or nothing) */
replay_status_t replay_get_status(void);
/** @returns the ID of the current recording, or 0 if one has not been started */
time_t replay_get_id(void);

// TODO add replay loading stuff

#ifdef __cplusplus
};
#endif