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

#ifdef __cplusplus
extern "C" {
#endif
/**
 * Starts OpenCV frame grabbing and processing. This blocks the current thread until capture is stopped with
 * @ref vision_dispose
 */
void vision_init(void);

/**
 * Stops capture and frees allocated OpenCV resources. This will unblock the current thread.
 */
void vision_dispose(void);
#ifdef __cplusplus
}
#endif