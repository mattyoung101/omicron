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
#include "states.h"
#include "utils.h"

// static pid_config_t lrfPID = {LRF_KP, LRF_KI, LRF_KD, LRF_MAX};
static pid_config_t interceptPID = {INTERCEPT_KP, INTERCEPT_KI, INTERCEPT_KD, INTERCEPT_MAX, 0};

fsm_state_t stateDefenceIdle = {&state_nothing_enter, &state_nothing_exit, &state_defence_idle_update, "DefenceIdle"};
fsm_state_t stateDefenceDefend = {&state_nothing_enter, &state_nothing_exit, &state_defence_defend_update, "DefenceDefend"};
fsm_state_t stateDefenceSurge = {&state_nothing_enter, &state_defence_surge_exit, &state_defence_surge_update, "DefenceSurge"};

float accelProgress = 0.0f;

// shortcut lol
#define rs robotState

// Idle
void state_defence_idle_update(state_machine_t *fsm){
    static const char *TAG = "DefendIdleState";

    accelProgress = 0;

    rs.outIsAttack = false;
    imu_correction(&robotState);

    if (robotState.inBallVisible){
        LOG_ONCE(TAG, "Ball visible, switching to defend");
        FSM_CHANGE_STATE_DEFENCE(Defend);
    }

    // rs.outSpeed = 0;
    position(&robotState, 65.0f, 0.0f, rs.inGoal.arg, rs.inGoal.mag, true); // TODO: 
}

static om_timer_t surgeKickTimer = {NULL, false};

static void can_kick_callback(TimerHandle_t timer){
    ESP_LOGI("CanKick", "In surge for long enough, kicking now");
    om_timer_stop(&surgeKickTimer);
    state_machine_t *fsm = (state_machine_t*) pvTimerGetTimerID(timer);
    FSM_CHANGE_STATE_GENERAL(Shoot);
}

// Defend
 void state_defence_defend_update(state_machine_t *fsm){
    static const char *TAG = "DefendDefend";

    accelProgress = 0;
    rs.outIsAttack = false;

    goal_correction(&robotState);

    // Check criteria: TODO: FIX
    if (!robotState.inBallVisible){
        LOG_ONCE(TAG, "Ball not visible, switching to Idle");
        FSM_CHANGE_STATE_DEFENCE(Idle);
    } else if (is_angle_between(rs.inBallPos.arg, SURGEON_ANGLE_MIN, SURGEON_ANGLE_MAX) 
                && rs.inBallPos.mag <= SURGE_STRENGTH && rs.inGoal.mag < SURGE_DISTANCE){
        LOG_ONCE(TAG, "Switching to surge, angle: %.2f, distance: %f, goal length: %.2f",
                robotState.inBallPos.arg, robotState.inBallPos.mag, rs.inGoal.mag);
        FSM_CHANGE_STATE_DEFENCE(Surge);
    }

    // TODO: ADD NEW DEFENCE CODE
}

// Surge
void state_defence_surge_update(state_machine_t *fsm){
    static const char *TAG = "DefendSurgeState";
    imu_correction(&robotState);
    om_timer_check_create(&surgeKickTimer, "SurgeCanKick", SURGE_CAN_KICK_TIMEOUT, (void*) fsm, can_kick_callback);

    RS_SEM_LOCK
    rs.outSwitchOk = true;
    rs.outIsAttack = false;
    RS_SEM_UNLOCK

    if (!rs.inBTConnection){
        om_timer_start(&surgeKickTimer);
    } else {
        om_timer_stop(&surgeKickTimer);
    }

    // Check criteria:
    // too far from goal, ball not in capture zone, should we kick?
    if (rs.inGoal.mag >= SURGE_DISTANCE || !rs.inGoalVisible){
        LOG_ONCE(TAG, "Too far from goal, switching to defend, goal dist: %.2f", rs.inGoal.mag);
        FSM_CHANGE_STATE_DEFENCE(Defend);
    } else if (!rs.inFrontGate){
        LOG_ONCE(TAG, "Ball is not in capture zone, switching to defend");
        FSM_CHANGE_STATE_DEFENCE(Defend);
    }

    // EPIC YEET MODE
    // Linear acceleration to give robot time to goal correct and so it doesn't slip
    robotState.outMotion = vect_2d(SURGE_SPEED, robotState.inBallPos.arg, true);
    // Just yeet towards the ball (which is forwards)
    // robotState.outDirection = robotState.inGoalVisible ? robotState.inGoalAngle : robotState.inBallPos.arg * 1.05;
}

void state_defence_surge_exit(state_machine_t *fsm){
    om_timer_stop(&surgeKickTimer);
}