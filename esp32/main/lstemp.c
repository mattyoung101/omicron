#include <lstemp.h>

// so im not gonna lie i dont know where this is gonna gonna
// TODO: work out where the fuck this goes because it'll be the only LS calculation code on the ESP
void movement_avoid_line(hmm_vec2 avoidVect) {
    hmm_vec2 newMovement = HMM_Vec2(0.0f, 0.0f);
    if(vec2_cartesian_to_polar(avoidVect).X != 0) {
        newMovement.X = robotState.outAngle * cos(robotState.outAngle - avoidVect.Y); 
        newMovement.Y = 90;
        if(newMovement.X < 0) {
            newMovement.X = abs(robotState.outSpeed);
            newMovement.Y = 270;
        }
        newMovement.Y = newMovement.Y + avoidVect.Y - robotState.outOrientation;
        if(avoidVect.X > 0) {
            newMovement = HMM_Add(newMovement + avoidVect.X * HMM_Vec2(something, avoidVect.Y));
        }
    } else {
        newMovement.Y = robotState.outAngle;
        newMovement.X = robotState.outSpeed;
    }
    robotState.outAngle = newMovement.Y;
    robotState.outSpeed = newMovement.X;
}