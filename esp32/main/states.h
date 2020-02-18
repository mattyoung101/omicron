#pragma once
#include "fsm.h"
#include <math.h>
#include <avoid.h>
#include <path_following.h>
#include "pid.h"
#include "defines.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"

// Struct which holds highly processed info about the robot's state. Shared resource, should be synced with a mutex.
typedef struct {
    // Goal Inputs
    // Main goal in question (home goal for defender, enemy goal for attacker)
    bool inGoalVisible;
    int16_t inGoalAngle;
    int16_t inGoalLength;
    int16_t inGoalDistance;
    // Other goal (home goal for attacker, enemy goal for defender)
    bool inOtherGoalVisible;
    int16_t inOtherGoalAngle;
    int16_t inOtherGoalLength;
    int16_t inOtherGoalDistance;
    // Coordinate System
    float inX;
    float inY;
    // IMU Input
    float inHeading;
    // Ball Inputs
    float inBallAngle;
    float inBallDistance;
    int16_t inBallX;
    int16_t inBallY;
    // Line Inputs
    float inLineAngle;
    float inLineSize;
    bool inIsOver;
    bool inOnLine; // Gotta call some update function for this one probs
    // Lightgate Inputs
    bool inFrontGate;
    bool inBackGate;
    // Other stuff
    float inBatteryVoltage;
    uint8_t inRobotId;
    bool inBTConnection;

    // Outputs
    int16_t outSpeed;
    int16_t outDirection;
    int16_t outOrientation;
    bool outShouldBrake;
    bool outIsAttack;
    bool outSwitchOk;
    bool outLineBallWaiting;
    int16_t outFRMotor;
    int16_t outBRMotor;
    int16_t outBLMotor;
    int16_t outFLMotor;
    bool debugLEDs[6];
} robot_state_t;

typedef struct {
    TimerHandle_t timer;
    bool running;
} om_timer_t;

extern bool canShoot;

/** locks the robot state semaphore */
#define RS_SEM_LOCK if (xSemaphoreTake(robotStateSem, pdMS_TO_TICKS(SEMAPHORE_UNLOCK_TIMEOUT))) {
/** unlocks the robot state semaphore */
#define RS_SEM_UNLOCK xSemaphoreGive(robotStateSem); } else { ESP_LOGW(TAG, "Failed to unlock robot state semaphore!"); }

/** start a timer if its not already started and has been instantiated */
void om_timer_start(om_timer_t *timer);
/** stops a timer if it has been instantiated */
void om_timer_stop(om_timer_t *timer);
/** 
 * checks to see if a timer needs to be created.
 * @param timeout timeout, in ms, automatically converted to ticks inside this function using pdMS_TO_TICKS
 */
void om_timer_check_create(om_timer_t *timer, char *timerName, int32_t timeout, void *const parameter, 
                            TimerCallbackFunction_t callback);

extern SemaphoreHandle_t robotStateSem;
extern robot_state_t robotState;

// Generic do nothing states (used for example if no "enter" method is needed on a state)
void state_nothing_enter(state_machine_t *fsm);
void state_nothing_exit(state_machine_t *fsm);
void state_nothing_update(state_machine_t *fsm);

/////////// ATTACK FSM /////////
// Pursue ball state: moves directly towards the ball
void state_attack_pursue_enter(state_machine_t *fsm);
void state_attack_pursue_update(state_machine_t *fsm);
extern fsm_state_t stateAttackPursue;

// Front orbit state: orbits behind the ball
void state_attack_frontorbit_enter(state_machine_t *fsm);
void state_attack_frontorbit_update(state_machine_t *fsm);
extern fsm_state_t stateAttackFrontOrbit;

// Reverse orbit state: orbits in front of the ball
void state_attack_reverseorbit_enter(state_machine_t *fsm);
void state_attack_reverseorbit_update(state_machine_t *fsm);
extern fsm_state_t stateAttackReverseOrbit;

// Yeet state: yeets forward at the ball to score
void state_attack_yeet_enter(state_machine_t *fsm);
void state_attack_yeet_update(state_machine_t *fsm);
extern fsm_state_t stateAttackYeet;

// Turtle state: approaches the goal with the ball behind it
void state_attack_turtle_enter(state_machine_t *fsm);
void state_attack_turtle_update(state_machine_t *fsm);
extern fsm_state_t stateAttackTurtle;

// Linerun state: runs up the side line with the ball behind it
// NOTE: Add functionality for duo linerun
void state_attack_linerun_enter(state_machine_t *fsm);
void state_attack_linerun_update(state_machine_t *fsm);
extern fsm_state_t stateAttackLinerun;

// Zigzag state: move in zigzags down the field so the opposition finds it hard to counter you reliably
void state_attack_zigzag_enter(state_machine_t *fsm);
void state_attack_zigzag_update(state_machine_t *fsm);
extern fsm_state_t stateAttackZigzag;

/////////// DEFENCE FSM /////////
// Idle state: centres on own goal
void state_defence_idle_update(state_machine_t *fsm);
extern fsm_state_t stateDefenceIdle;

// Defend state: positions between ball and centre of goal
void state_defence_defend_enter(state_machine_t *fsm);
void state_defence_defend_update(state_machine_t *fsm);
extern fsm_state_t stateDefenceDefend;

// Surge state: push ball away from goal
void state_defence_surge_update(state_machine_t *fsm);
void state_defence_surge_exit(state_machine_t *fsm);
extern fsm_state_t stateDefenceSurge;

/////////// GENERAL FSM ///////////
extern fsm_state_t stateGeneralNothing;

// Find ball state: yeets around the field to find the ball
void state_general_findball_enter(state_machine_t *fsm);
void state_general_findball_update(state_machine_t *fsm);
extern fsm_state_t stateGeneralFindball;

// Shoot state: kicks the ball, then reverts to previous state
void state_general_shoot_enter(state_machine_t *fsm);
void state_general_shoot_update(state_machine_t *fsm);
extern fsm_state_t stateGeneralShoot;

// Throw state: spins quickly to throw the ball out from behind the robot
void state_general_throw_enter(state_machine_t *fsm);
void state_general_throw_update(state_machine_t *fsm);
extern fsm_state_t stateGeneralThrow;