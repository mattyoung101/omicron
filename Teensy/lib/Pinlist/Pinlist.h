/*
 * This file is part of the Teensy 4 firmware project.
 * Copyright (c) 2019-2020 Team Omicron. All rights reserved.
 *
 * Team Omicron members: Lachlan Ellis, Tynan Jones, Ethan Lo,
 * James Talkington, Matt Young.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
//Header file for defining pins used
#ifndef PINLIST_H
#define PINLIST_H

#include <Arduino.h>
#include <Config.h>

// --- Light Sensors --- //

// #define MUX_EN 43 // TODO: FIX
#define MUX_A0 6
#define MUX_A1 5
#define MUX_A2 4
#define MUX_A3 3
#define MUX_A4 2
// #define MUX_WR 44
#define MUX_OUT 22

// --- Serial --- //

#define ESPSERIAL Serial1

// --- Debug --- //

#define BUTTON1 0 // TODO: FIX

// --- Lightgate --- //

#define FRONTGATE A0 // TODO: FIX
#define BACKGATE A1

#endif // PINLIST_H
