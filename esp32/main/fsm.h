/*
 * This file is part of the ESP32 firmware project.
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
#include <string.h>
#include <stdbool.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "DG_dynarr.h"

// State machine with stack history, based on/inspired by:
// https://github.com/libgdx/gdx-ai/blob/master/gdx-ai/src/com/badlogic/gdx/ai/fsm/StackStateMachine.java

typedef struct fsm_state_t fsm_state_t;
typedef struct state_machine_t state_machine_t;

// void stateFunction(state_machine_t *fsm);
typedef void (*fsm_func)(state_machine_t*);

struct fsm_state_t {
    fsm_func enter;
    fsm_func exit;
    fsm_func update;
    char *name;
};

// this is for the DG_dynarr library to setup it's type definitions
DA_TYPEDEF(fsm_state_t*, fsm_state_history_t);

struct state_machine_t {
    fsm_state_t *currentState;
    fsm_state_history_t stateHistory;
    SemaphoreHandle_t semaphore;
    SemaphoreHandle_t updateInProgress;
};

extern state_machine_t *stateMachine;

/** Instantiates a new state machine, starting out in the given state */
state_machine_t* fsm_new(fsm_state_t *startState);
/** Updates the instance of the Finite State Machine */
void fsm_update(state_machine_t *fsm);
/** Changes from one state to another */
void fsm_change_state(state_machine_t *fsm, fsm_state_t *newState);
/** Reverts to the previous state in the stack */
void fsm_revert_state(state_machine_t *fsm);
/** Returns true if the given state machine is in the state provided by the string "name" */
bool fsm_in_state(state_machine_t *fsm, char *name);
/** 
 * Thread safe function to get the current state name, synchronised with semaphores.
 * IMPORTANT NOTE: the returned string MUST be freed with free(), as strdup is used. 
 */
char *fsm_get_current_state_name(state_machine_t *fsm);
/** Resets the FSM to its initial state */
void fsm_reset(state_machine_t *fsm);
/** Just clears the state history, but does not change states */
void fsm_partial_reset(state_machine_t *fsm);
/** Dumps the FSM to the console */
void fsm_dump(state_machine_t *fsm);
/** Frees memory allocated by the FSM */
void fsm_free(state_machine_t *fsm);