#pragma once
#include <Arduino.h>
#include <Config.h>
#include <Utils.h>

typedef struct goal_t {
    int x, y;
    bool exists;
    int angle, length;
};

class Camera {
public:
    goal_t blue;
    goal_t yellow;
    goal_t orange;
    void read();
    void setup();
    void calc();
private:
    int buf[CAM_DATA_LENGTH];
    int lastCall;
};
