/*
 * This file is part of the Omicam project.
 * Copyright (c) 2019-2020 Team Omicron. All rights reserved.
 *
 * Team Omicron members: Lachlan Ellis, Tynan Jones, Ethan Lo,
 * James Talkington, Matt Young.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
// Hack to route UART from the Latte Panda out to the other devices
// NOTE: currently untested 

int ticks = 0;

void setup(){
    Serial.begin(115200); // Panda -> ATMega
    Serial1.begin(115200); // ATMega -> rest of the robot
    Serial.println("UART hack initialised successfully");
    pinMode(LED_BUILTIN, OUTPUT);
} 

void loop(){
    while (Serial.available()){
        Serial1.write(Serial.read());
    }

    while (Serial1.available()){
        Serial.write(Serial1.read());
    }

    // check to make sure it's running
    if (ticks++ % 4096 == 0){
        digitalWrite(LED_BUILTIN, HIGH);
    } else {
        digitalWrite(LED_BUILTIN, LOW);
    }
}
