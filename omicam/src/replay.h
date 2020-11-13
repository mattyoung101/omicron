/*
 * This file is part of the Omicam project.
 * Copyright (c) 2019-2020 Team Omicron. All rights reserved.
 *
 * Team Omicron members: Lachlan Ellis, Tynan Jones, Ethan Lo,
 * James Talkington, Matt Young.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include "defines.h"
#include "protobuf/Replay.pb.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Begins recording a replay, replay status will be set to REPLAY_RECORDING. */
void replay_record(void);
/** If a replay is being recorded, flushes it to disk. Then, disposes all resources that have been allocated. */
void replay_close(void);
/** @returns the current status of the replay system (eg whether recording, playing back, or nothing) */
replay_status_t replay_get_status(void);
/** @returns the ID of the current recording, or 0 if one has not been started */
time_t replay_get_id(void);
/**
 * Posts a ReplayFrame to the replay manager. It will be added to the write queue and eventually written to disk.
 *
 * Caution: this function may block for slightly longer than normal every 5 seconds or so, when the replay file is being
 * written to disk. The longer the replay becomes, the longer it will block for.
 */
void replay_post_frame(ReplayFrame frame);

#ifdef __cplusplus
};
#endif