#pragma once
#include "DG_dynarr.h"
#include "HandmadeMath.h"
#include <math.h>
#include "utils.h"

DA_TYPEDEF(hmm_vec2, point_list_t);

void pf_calculate_nearest(point_list_t points, float robotX, float robotY);