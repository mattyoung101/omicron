#include "states.h"
#include "utils.h"

robot_state_t robotState = {0};
SemaphoreHandle_t robotStateSem = NULL;
#ifdef HAS_KICKER
bool canShoot = true;
#else
bool canShoot = false;
#endif

fsm_state_t stateAttackPursue = {&state_attack_pursue_enter, &state_nothing_exit, &state_attack_pursue_update, "AttackPursue"};
fsm_state_t stateAttackFrontOrbit = {&state_nothing_enter, &state_nothing_exit, &state_attack_frontorbit_update, "AttackFrontOrbit"};
fsm_state_t stateAttackReverseOrbit = {&state_nothing_enter, &state_nothing_exit, &state_attack_reverseorbit_update, "AttackReverseOrbit"};
fsm_state_t stateAttackYeet = {&state_attack_yeet_enter, &state_nothing_exit, &state_attack_yeet_update, "AttackYeet"};
fsm_state_t stateAttackTurtle = {&state_nothing_enter, &state_nothing_exit, &state_attack_turtle_update, "AttackTurtle"};
fsm_state_t stateAttackLinerun = {&state_nothing_enter, &state_nothing_update, &state_attack_linerun_update, "AttackLinerun"};

static om_timer_t noBallTimer = {NULL, false};
static om_timer_t yeetTimer = {NULL, false};
static bool canExitYeet = false; // true if allowed to exit dribble (dribble timer has gone off)
static float accelProgress = 0.0f;
static float accelBegin = 0.0f;

// shortcut lol
#define rs robotState

/** callback that goes off after idle timeout **/
static void noball_timer_callback(TimerHandle_t timer){
    static const char *TAG = "NoBallTimerCallback";
    ESP_LOGI(TAG, "No ball timer has gone off, switching to idle state");

    // world-class intellectual hack: we need to get access to the state machine instance from this callback.
    // as it turns out, the timer ID is passed as a void pointer (meaning it can be any type, though in this context
    // it should probably be an integer) - so we pass the state_machine_t as the timer's ID
    state_machine_t *fsm = (state_machine_t*) pvTimerGetTimerID(timer);
    om_timer_stop(&noBallTimer);
    FSM_CHANGE_STATE_GENERAL(Findball); // TODO: make this change into ball finder
}

static void yeet_timer_callback(TimerHandle_t timer){
    static const char *TAG = "YeetTimerCallback";
    ESP_LOGI(TAG, "Yeet timer has gone off, free to switch out of dribble if willing");
    canExitYeet = true;
    om_timer_stop(&yeetTimer);
}

static void create_timers_if_needed(state_machine_t *fsm){
    om_timer_check_create(&noBallTimer, "IdleTimer", IDLE_TIMEOUT, (void*) fsm, noball_timer_callback);
    om_timer_check_create(&yeetTimer, "DribbleTimer", DRIBBLE_TIMEOUT, NULL, yeet_timer_callback);
}

static void timer_check(){
    // if the ball is visible, stop the idle timer
    if (orangeBall.exists){
        om_timer_stop(&noBallTimer);
    }
}


////////// BEGIN STATE MACHINE CODE //////////
// Pursue
void state_attack_pursue_enter(state_machine_t *fsm){
    create_timers_if_needed(fsm);
}

void state_attack_pursue_update(state_machine_t *fsm){
    static const char *TAG = "PursueState";
    
    accelProgress = 0;
    RS_SEM_LOCK
    rs.outIsAttack = true;
    rs.outSwitchOk = true;
    RS_SEM_UNLOCK
    imu_correction(&robotState);
    timer_check();

    // Check criteria:
    // Ball not visible (brake) and ball too close (switch to orbit)
    if (!orangeBall.exists){
        LOG_ONCE(TAG, "Ball is not visible, braking");
        om_timer_start(&noBallTimer);
        FSM_MOTOR_BRAKE;
    } else if (rs.inBallDistance >= ORBIT_DIST){
        if (is_angle_between(rs.inBallAngle, FORWARD_ORBIT_MIN_ANGLE, FORWARD_ORBIT_MAX_ANGLE)){
            LOG_ONCE(TAG, "Ball close enough and ball is in forward range, switching to front orbit, strength: %f, orbit dist thresh: %d, angle: %f", 
            rs.inBallDistance, ORBIT_DIST, rs.inBallAngle);
            FSM_CHANGE_STATE(FrontOrbit);
        } else {
            LOG_ONCE(TAG, "Ball close enough and ball is in back range, switching to reverse orbit, strength: %f, orbit dist thresh: %d, angle: %f", 
            rs.inBallDistance, ORBIT_DIST, rs.inBallAngle);
            FSM_CHANGE_STATE(ReverseOrbit);
        }
    }

    // LOG_ONCE(TAG, "Ball is visible, pursuing");
    // Quickly approach the ball
    robotState.outSpeed = 100; // TODO: using mm/s speed now :D
    robotState.outDirection = robotState.inBallAngle;
}

// Orbit
void state_attack_frontorbit_update(state_machine_t *fsm){
    static const char *TAG = "FrontOrbitState";

    accelProgress = 0; // reset acceleration progress
    RS_SEM_LOCK
    rs.outIsAttack = true;
    rs.outSwitchOk = true;
    RS_SEM_UNLOCK

    imu_correction(&robotState);
    timer_check();

    if (rs.inBallDistance <= DRIBBLE_BALL_TOO_FAR && is_angle_between(rs.inBallAngle, IN_FRONT_MIN_ANGLE, IN_FRONT_MAX_ANGLE)){
        LOG_ONCE(TAG, "Ball and angle in correct spot, changing intro dribble, strength: %f, angle: %f, orbit dist thresh: %d"
                " angle range: %d-%d", robotState.inBallDistance, robotState.inBallAngle, ORBIT_DIST, IN_FRONT_MIN_ANGLE, 
                IN_FRONT_MAX_ANGLE);
        accelBegin = rs.outSpeed;
        FSM_CHANGE_STATE(Yeet);
    }

    // Check criteria:
    // Ball too far away, Ball too close and angle good (go to yeet), Ball too far (revert)
    if (!orangeBall.exists){
        LOG_ONCE(TAG, "Ball not visible, starting idle timer, strength: %f", robotState.inBallDistance);
        om_timer_start(&noBallTimer);
        FSM_MOTOR_BRAKE;
    } else if (rs.inBallDistance < ORBIT_DIST){
        LOG_ONCE(TAG, "Ball too far away, reverting, strength: %f, orbit dist thresh: %d", robotState.inBallDistance,
                 ORBIT_DIST);
        FSM_REVERT;
    } else if (!is_angle_between(rs.inBallAngle, FORWARD_ORBIT_MIN_ANGLE, FORWARD_ORBIT_MAX_ANGLE)){
        LOG_ONCE(TAG, "Ball is behind, reverting");
        FSM_REVERT;
    }

    orbit(&robotState); // TODO: replace with new orbit
}

void state_attack_reverseorbit_update(state_machine_t *fsm){
    static const char *TAG = "ReverseOrbitState";

    accelProgress = 0; // reset acceleration progress
    RS_SEM_LOCK
    rs.outIsAttack = true;
    rs.outSwitchOk = true;
    RS_SEM_UNLOCK

    imu_correction(&robotState);
    timer_check();

    // Check criteria:
    if (!orangeBall.exists){
        LOG_ONCE(TAG, "Ball not visible, starting idle timer, strength: %f", robotState.inBallDistance);
        om_timer_start(&noBallTimer);
        FSM_MOTOR_BRAKE;
    } else if (rs.inBallDistance < ORBIT_DIST){
        LOG_ONCE(TAG, "Ball too far away, reverting, strength: %f, orbit dist thresh: %d", robotState.inBallDistance,
                 ORBIT_DIST);
        FSM_REVERT;
    } else if (is_angle_between(rs.inBallAngle, FORWARD_ORBIT_MIN_ANGLE, FORWARD_ORBIT_MAX_ANGLE)){
        LOG_ONCE(TAG, "Ball is in front, reverting");
        FSM_REVERT;
    } else if (rs.inBackGate){
        LOG_ONCE(TAG, "Ball is in back capture zone, activating strat");
        // TODO: add state switch to strat, and the magic black box
    }

    orbit(&robotState); // TODO: replace with reversed orbit
}

// Yeet
void state_attack_yeet_enter(state_machine_t *fsm){
    create_timers_if_needed(fsm);
    canExitYeet = false;
}

void state_attack_yeet_update(state_machine_t *fsm){
    static const char *TAG = "YeetState";
    
    RS_SEM_LOCK
    rs.outIsAttack = true;
    rs.outSwitchOk = false; // we're trying to score so piss off
    RS_SEM_UNLOCK
    goal_correction(&robotState);
    timer_check();

    // Check criteria:
    // Ball not visible, ball not in front, ball too far away, not facing goal, should we kick?
    if (rs.inFrontGate && is_angle_between(rs.inGoalAngle, IN_FRONT_MIN_ANGLE + IN_FRONT_ANGLE_BUFFER, 
        IN_FRONT_MAX_ANGLE - IN_FRONT_ANGLE_BUFFER) && robotState.inGoalLength <= GOAL_SHOOT_DIST && canShoot){
        LOG_ONCE(TAG, "Ball in capture zone and facing goal and shoot permitted, shooting, angle: %d, range: %d", rs.inGoalAngle, rs.inGoalLength);
        FSM_CHANGE_STATE_GENERAL(Shoot); // TODO: use real world units
    } else if (!orangeBall.exists && !rs.inFrontGate){
        LOG_ONCE(TAG, "Ball not visible, braking, strength: %f", robotState.inBallDistance);
        om_timer_start(&noBallTimer);
        FSM_MOTOR_BRAKE;
    } else if (rs.inBallAngle > IN_FRONT_MIN_ANGLE + IN_FRONT_ANGLE_BUFFER && rs.inBallAngle < IN_FRONT_MAX_ANGLE - IN_FRONT_ANGLE_BUFFER){
        LOG_ONCE(TAG, "Ball not in front, WILL REVERT, angle: %f, range: %d-%d", robotState.inBallAngle,
                IN_FRONT_MIN_ANGLE + IN_FRONT_ANGLE_BUFFER, IN_FRONT_MAX_ANGLE - IN_FRONT_ANGLE_BUFFER);       
        om_timer_start(&yeetTimer);

        if (canExitYeet){
            LOG_ONCE(TAG, "Allowed to exit dribble. Reverting now!");
            FSM_REVERT;
        }
    } else {
        // all checks passed, stop switch out of dribble timer and say we're locked in dribble again
        om_timer_stop(&yeetTimer);
        canExitYeet = false;
    }

    // Linear acceleration to give robot time to goal correct and so it doesn't slip
    robotState.outSpeed = lerp(accelBegin, DRIBBLE_SPEED, constrain(accelProgress, 0.0f, 1.0f)); 
    // Yeet towards te goal if visible
    // robotState.outDirection = robotState.inGoalVisible ? robotState.inGoalAngle : robotState.inBallAngle;
    robotState.outDirection = robotState.inBallAngle;

    // Update progress for linear interpolation
    accelProgress += ACCEL_PROG;
}

void state_attack_turtle_update(state_machine_t *fsm){
    static const char *TAG = "TurtleState";

    RS_SEM_LOCK
    rs.outIsAttack = true;
    rs.outSwitchOk = false; // we're trying to score so piss off
    RS_SEM_UNLOCK
    goal_correction(&robotState);
    timer_check();

    // Check criteria:
    if (!rs.inBackGate){
        LOG_ONCE(TAG, "Ball not in capture zone, reverting");
        FSM_REVERT;
    } else if (rs.inGoalLength <= GOAL_SHOOT_DIST && canShoot){
        LOG_ONCE(TAG, "Goal in range, shooting");
        FSM_CHANGE_STATE_GENERAL(Shoot); // Yet to implement throwing
    }

    // TODO: implement actual movement
}

void state_attack_linerun_update(state_machine_t *fsm){
    static const char *TAG = "LinerunState";

    RS_SEM_LOCK
    rs.outIsAttack = true;
    rs.outSwitchOk = false; // we're trying to score so piss off
    RS_SEM_UNLOCK
    goal_correction(&robotState);
    timer_check();

    // Check criteria:
    if (!rs.inBackGate){
        LOG_ONCE(TAG, "Ball not in capture zone, reverting");
        FSM_REVERT;
    } else if (rs.inGoalLength <= GOAL_SHOOT_DIST && canShoot){
        LOG_ONCE(TAG, "Goal in range, shooting");
        FSM_CHANGE_STATE_GENERAL(Shoot); // Yet to implement throwing
    }

    // TODO: implement actual movement
}

void state_attack_zigzag_update(state_machine_t *fsm)
{
    static const char *TAG = "LinerunState";

    RS_SEM_LOCK
    rs.outIsAttack = true;
    rs.outSwitchOk = false; // we're trying to score so piss off
    RS_SEM_UNLOCK
    goal_correction(&robotState);
    timer_check();

    // Check criteria:
    if (!rs.inBackGate)
    {
        LOG_ONCE(TAG, "Ball not in capture zone, reverting");
        FSM_REVERT;
    }
    else if (rs.inGoalLength <= GOAL_SHOOT_DIST && canShoot)
    {
        LOG_ONCE(TAG, "Goal in range, shooting");
        FSM_CHANGE_STATE_GENERAL(Shoot); // Yet to implement throwing
    }

    // TODO: implement actual movement
}

// done with this macro
#undef rs