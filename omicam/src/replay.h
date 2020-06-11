#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Begins recording a replay */
void replay_begin();
/** Flushes any active replays to disk */
void replay_end();
/** Disposes resources for any active replays */
void replay_dispose();
/** @returns true if a replay is currently being recorded */
bool replay_is_recording();
/** @returns the ID of the current recording, or 0 if one has not been started */
time_t replay_get_id();

#ifdef __cplusplus
};
#endif