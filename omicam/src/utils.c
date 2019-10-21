#include <bits/types/time_t.h>
#include <time.h>
#include "utils.h"
#include <math.h>
#include <stdint.h>
#include <sys/time.h>

// source: https://stackoverflow.com/a/3756954/5007892
double utils_get_millis(){
    struct timeval  tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec) * 1000.0 + (tv.tv_usec) / 1000.0;
}