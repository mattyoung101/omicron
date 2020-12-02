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

fsm_state_t stateGeneralNothing = {&state_nothing_enter, &state_nothing_exit, &state_nothing_update, "GeneralNothing"};
fsm_state_t stateGeneralShoot = {&state_general_shoot_enter, &state_nothing_exit, &state_general_shoot_update, "GeneralShoot"};
fsm_state_t stateGeneralThrow = {&state_general_throw_enter, &state_nothing_exit, &state_general_throw_update, "GeneralThrow"};
fsm_state_t stateGeneralFindball = {&state_nothing_enter, &state_nothing_exit, &state_general_findball_update, "GeneralFindball"};
static om_timer_t shootTimer = {NULL, false};

// shortcut cos i hate typing
#define rs robotState

/** start a timer if its not already started and has been instantiated */
void om_timer_start(om_timer_t *timer){
    if (timer->timer != NULL && !timer->running){
        xTimerReset(timer->timer, pdMS_TO_TICKS(10));
        xTimerStart(timer->timer, pdMS_TO_TICKS(10));
        timer->running = true;
    }
}

/** stops a timer if it has been instantiated */
void om_timer_stop(om_timer_t *timer){
    if (timer->timer != NULL){
        xTimerStop(timer->timer, pdMS_TO_TICKS(10));
        timer->running = false;
    }
}

void om_timer_check_create(om_timer_t *timer, char *timerName, int32_t timeout, void *const parameter, 
                            TimerCallbackFunction_t callback){
    if (timer->timer == NULL && !timer->running){
        ESP_LOGI("CreateTimer", "Creating timer: %s", timerName);
        timer->timer = xTimerCreate(timerName, pdMS_TO_TICKS(timeout), false, parameter, callback);
    }
}

static void shoot_timer_callback(TimerHandle_t timer){
    static const char *TAG = "ShootTimerCallback";
    ESP_LOGW(TAG, "Shoot timer gone off, enabling shooting again");
    
    canShoot = true;
    om_timer_stop(&shootTimer);
}

// Shoot
void state_general_shoot_enter(state_machine_t *fsm){
    static const char *TAG = "ShootState";
    if (!canShoot){
        ESP_LOGE(TAG, "Illegal state change: shoot not permitted at this time. Fix your code!");
        return;
    }
    
    canShoot = false;
    om_timer_check_create(&shootTimer, "ShootTimer", SHOOT_TIMEOUT, NULL, shoot_timer_callback);
    om_timer_start(&shootTimer);
    
    ESP_LOGW(TAG, "Activating kicker");
    gpio_set_level(KICKER_PIN1, 1);
    vTaskDelay(pdMS_TO_TICKS(KICKER_DELAY));
    gpio_set_level(KICKER_PIN1, 0);
}

void state_general_shoot_update(state_machine_t *fsm){
    // we revert here as reverting in enter seems to cause problems
    FSM_REVERT;
}

void state_general_throw_enter(state_machine_t *fsm){
    static const char *TAG = "ThrowState";
    if (!canShoot){
        ESP_LOGE(TAG, "Kicker not ready, shoot not permitted, fix your code");
        return;
    }

    canShoot = false;
    om_timer_check_create(&shootTimer, "ShootTimer", SHOOT_TIMEOUT, NULL, shoot_timer_callback);
    om_timer_start(&shootTimer);
    
    // TODO, implement rotation for throwing mechanism
    ESP_LOGW(TAG, "Activating kicker");
    gpio_set_level(KICKER_PIN2, 1);
    vTaskDelay(pdMS_TO_TICKS(KICKER_DELAY));
    gpio_set_level(KICKER_PIN2, 0);
}

void state_general_throw_update(state_machine_t *fsm){
    FSM_REVERT;
}

void state_general_findball_update(state_machine_t *fsm){
    static const char *TAG = "FindballState";

    // Check criteria
    if (robotState.inBallVisible){
        LOG_ONCE(TAG, "Ball is visible, reverting");
        FSM_REVERT;
    }

    // TODO: Implement movement code
}

// Nothing. Empty states for when no state function is declared.
void state_nothing_enter(state_machine_t *fsm){
    // do nothing
}
void state_nothing_exit(state_machine_t *fsm){
    // do nothing
}
void state_nothing_update(state_machine_t *fsm){
    // do nothing
    ESP_LOGW("StateNothing", "Illegal state change: should never be in StateNothing. Fix your code!");
    FSM_MOTOR_BRAKE;
}