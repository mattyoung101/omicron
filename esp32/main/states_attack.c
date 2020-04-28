#include "states.h"
#include "utils.h"

robot_state_t robotState = {0};
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
    if (robotState.inBallVisible){
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
    
    // accelProgress = 0;
    RS_SEM_LOCK
    rs.outIsAttack = true;
    rs.outSwitchOk = true;
    RS_SEM_UNLOCK
    imu_correction(&robotState);
    timer_check();

    // Check criteria:
    // Ball not visible (brake) and ball too close (switch to orbit)
    if (!rs.inBallVisible){
        LOG_ONCE(TAG, "Ball is not visible, braking");
        om_timer_start(&noBallTimer);
        FSM_MOTOR_BRAKE;
    } else if (rs.inBallPos.mag >= ORBIT_DIST){
        // if (is_angle_between(rs.inBallPos.arg, FORWARD_ORBIT_MIN_ANGLE, FORWARD_ORBIT_MAX_ANGLE)){
            LOG_ONCE(TAG, "Ball close enough and ball is in forward range, switching to front orbit, distance: %f, orbit dist thresh: %d, angle: %f", 
            rs.inBallPos.mag, ORBIT_DIST, rs.inBallPos.arg);
            FSM_CHANGE_STATE(FrontOrbit);
        // }
    }
    //     } else {
    //         LOG_ONCE(TAG, "Ball close enough and ball is in back range, switching to reverse orbit, distance: %f, orbit dist thresh: %d, angle: %f", 
    //         rs.inBallPos.mag, ORBIT_DIST, rs.inBallPos.arg);
    //         FSM_CHANGE_STATE(ReverseOrbit);
    //     }
    // }

    // LOG_ONCE(TAG, "Ball is visible, pursuing");
    // Quickly approach the ball
    robotState.outMotion.mag = 30; // TODO: using mm/s speed now :D
    robotState.outMotion.arg = robotState.inBallPos.arg;
}

// Orbit
void state_attack_frontorbit_update(state_machine_t *fsm){
    static const char *TAG = "FrontOrbitState";
    
    // accelProgress = 0;
    RS_SEM_LOCK
    rs.outIsAttack = true;
    rs.outSwitchOk = true;
    RS_SEM_UNLOCK
    imu_correction(&robotState);
    timer_check();

    if (!rs.inBallVisible){
        LOG_ONCE(TAG, "Ball is not visible, braking");
        om_timer_start(&noBallTimer);
        FSM_MOTOR_BRAKE;
    }

    vect_2d_t tempVect = subtract_vect_2d(rs.inRobotPos, rs.inBallPos);
    if (sign(tempVect.y) == 1) {
        rs.outMotion = avoidMethod(rs.inBallPos, ORBIT_RADIUS, ORBIT_RADIUS, vect_2d(rs.inBallPos.x, rs.inBallPos.y - ORBIT_RADIUS, false), rs.inRobotPos);
    } else {
        if(sign(tempVect.x) == 1) {
            rs.outMotion = avoidMethod(vect_2d(rs.inBallPos.x + ORBIT_RADIUS/2, rs.inBallPos.y, false), ORBIT_RADIUS/2, ORBIT_RADIUS, rs.inBallPos, rs.inRobotPos);
        } else {
            rs.outMotion = avoidMethod(vect_2d(rs.inBallPos.x - ORBIT_RADIUS/2, rs.inBallPos.y, false), ORBIT_RADIUS/2, ORBIT_RADIUS, rs.inBallPos, rs.inRobotPos);
        }
    }
}

void state_attack_reverseorbit_update(state_machine_t *fsm){
    vect_2d_t tempVect = subtract_vect_2d(rs.inRobotPos, rs.inBallPos); 
     if (sign(tempVect.y) == -1) {
        rs.outMotion = avoidMethod(rs.inBallPos, ORBIT_RADIUS, ORBIT_RADIUS, vect_2d(rs.inBallPos.x, rs.inBallPos.y + ORBIT_RADIUS, false), rs.inRobotPos);
        return;
    } else {
        if(sign(tempVect.x) == 1) {
            rs.outMotion = avoidMethod(vect_2d(rs.inBallPos.x + ORBIT_RADIUS/2, rs.inBallPos.y, false), ORBIT_RADIUS/2, ORBIT_RADIUS, rs.inBallPos, rs.inRobotPos);
            return;
        } else {
            rs.outMotion = avoidMethod(vect_2d(rs.inBallPos.x - ORBIT_RADIUS/2, rs.inBallPos.y, false), ORBIT_RADIUS/2, ORBIT_RADIUS, rs.inBallPos, rs.inRobotPos);
            return;
        }
    }
    rs.outMotion = vect_2d(0, 0, true);
    return;
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
    if (rs.inFrontGate && is_angle_between(rs.inGoal.arg, IN_FRONT_MIN_ANGLE + IN_FRONT_ANGLE_BUFFER, 
        IN_FRONT_MAX_ANGLE - IN_FRONT_ANGLE_BUFFER) && robotState.inGoal.mag <= GOAL_SHOOT_DIST && canShoot){
        LOG_ONCE(TAG, "Ball in capture zone and facing goal and shoot permitted, shooting, angle: %f, range: %.2f", rs.inGoal.arg, rs.inGoal.mag);
        FSM_CHANGE_STATE_GENERAL(Shoot); // TODO: use real world units
    } else if (!robotState.inBallVisible && !rs.inFrontGate){
        LOG_ONCE(TAG, "Ball not visible, braking, distance: %f", robotState.inBallPos.mag);
        om_timer_start(&noBallTimer);
        FSM_MOTOR_BRAKE;
    } else if (rs.inBallPos.arg > IN_FRONT_MIN_ANGLE + IN_FRONT_ANGLE_BUFFER && rs.inBallPos.arg < IN_FRONT_MAX_ANGLE - IN_FRONT_ANGLE_BUFFER){
        LOG_ONCE(TAG, "Ball not in front, WILL REVERT, angle: %f, range: %d-%d", robotState.inBallPos.arg,
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
    robotState.outMotion = vect_2d(lerp(accelBegin, DRIBBLE_SPEED, constrain(accelProgress, 0.0f, 1.0f)), robotState.inBallPos.arg, true); 
    // Yeet towards te goal if visible
    // robotState.outDirection = robotState.inGoalVisible ? robotState.inGoalAngle : robotState.inBallPos.arg;
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
    } else if (rs.inGoal.mag <= GOAL_SHOOT_DIST && canShoot){
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
    } else if (rs.inGoal.mag <= GOAL_SHOOT_DIST && canShoot){
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
    else if (rs.inGoal.mag <= GOAL_SHOOT_DIST && canShoot)
    {
        LOG_ONCE(TAG, "Goal in range, shooting");
        FSM_CHANGE_STATE_GENERAL(Shoot); // Yet to implement throwing
    }

    // TODO: implement actual movement
}

// done with this macro
#undef rs