#pragma once
#include "DG_dynarr.h"
#include "Vector.h"
#include <math.h>
#include "utils.h"

DA_TYPEDEF(vect_2d_t, point_list_t);

void pf_calculate_nearest(point_list_t points, float robotX, float robotY);
vect_2d_t pf_follow_path(point_list_t points, float robotX, float robotY);

vect_2d_t lowestPoint;
vect_2d_t nextLowest;
float lowestDistance;