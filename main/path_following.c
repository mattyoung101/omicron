#include "path_following.h"

void pf_calculate_nearest(point_list_t points, float robotX, float robotY){
    size_t listSize = da_count(points);
    float m, c, lowestDistance = -1, curDistance;
    hmm_vec2 lowestPoint = HMM_Vec2(0.0f, 0.0f);
    hmm_vec2 curClosest = HMM_Vec2(0.0f, 0.0f);
    hmm_vec2 nextLowest = HMM_Vec2(0.0f, 0.0f);

    for (int i = 0; i < listSize - 1; i++){
        hmm_vec2 point = da_get(points, i);
        hmm_vec2 nextPoint = da_get(points, i + 1);

        if((nextPoint.X - point.X) == 0) {
            curClosest.X = point.X;
            curClosest.Y = robotY;
        } else {
            m = (nextPoint.Y - point.Y)/(nextPoint.X - point.X);
            c = point.Y - m * point.X;
    
            curClosest.X = ((robotX + m * robotY) - m * c)/(powf(m, 2) + 1);
            curClosest.Y = (m * (robotX + m * robotY) + c)/(powf(m, 2) + 1);
        }
    
        if (nextPoint.X > point.X) {
            curClosest.X = constrain(curClosest.X, point.X, nextPoint.X);
        } else {
            curClosest.Y = constrain(curClosest.Y, nextPoint.X, point.X);
        }
    
        if(nextPoint.Y > point.Y) {
            curClosest.X = constrain(curClosest.Y, point.Y, nextPoint.Y);
        } else {
            curClosest.X = constrain(curClosest.Y, nextPoint.Y, point.Y);
        }

        curDistance = sqrtf(powf((curClosest.X - robotX), 2) + powf((curClosest.Y - robotY), 2));

        if(curDistance <= lowestDistance || lowestDistance == -1) {
            lowestDistance = curDistance;
            lowestPoint.X = curClosest.X;
            lowestPoint.Y = curClosest.Y;
            nextLowest.X = nextPoint.X;
            nextLowest.Y = nextPoint.Y;
        }
    }
}