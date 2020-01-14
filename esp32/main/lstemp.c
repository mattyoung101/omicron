#include <lstemp.h>

// so im not gonna lie i dont know where this is gonna gonna
// TODO: work out where the fuck this goes because it'll be the only LS calculation code on the ESP
void movement_avoid_line(vect_2d_t avoidVect) {
    vect_2d_t newMovement = vect_2d(0.0f, 0.0f, false);

    if (is_angle_between(robotState.outDirection, avoidVect.arg + (avoidVect.mag < 0 ? 90 : -90), avoidVect.arg - (avoidVect.mag < 0 ? 90 : -90))) {
        if (avoidVect.mag != 0) {
            newMovement = vect_2d(robotState.outSpeed * sin(fmod(robotState.outDirection - avoidVect.arg, 360) * PI / 180), 90, true);

            if (newMovement.mag < 0) {
                newMovement = vect_2d(fabs(newMovement.mag), 270, true);
            }
            newMovement = vect_2d(newMovement.mag, fmod(newMovement.arg + avoidVect.arg, 360), true);

            if (avoidVect.mag > 0) {
                newMovement = add_vect_2d(newMovement, vect_2d((1 - avoidVect.mag) * 120, avoidVect.arg, true));
            }
        } else {
            newMovement = vect_2d(robotState.outSpeed, robotState.outDirection, true);
        }
        robotState.outSpeed = newMovement.mag;
        robotState.outDirection = newMovement.arg;
    } else {
        if (avoidVect.mag > 0) {
            vect_2d_t tempVect;
            tempVect = vect_2d(robotState.outSpeed, roboSpeed.outDirection, true);
            tempVect = add_vect_2d(tempVect, vect_2d((1 - avoidVect.mag) * 120, avoidVect.arg, true));
            robotState.outSpeed = tempVect.mag;
            robotState.outDirection = tempVect.arg;
        }
    }
}
