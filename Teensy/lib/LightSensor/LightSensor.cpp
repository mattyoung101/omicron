/*
 * This file is part of the Teensy 4 firmware project.
 * Copyright (c) 2019-2020 Team Omicron. All rights reserved.
 *
 * Team Omicron members: Lachlan Ellis, Tynan Jones, Ethan Lo,
 * James Talkington, Matt Young.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "LightSensor.h"

LightSensor::LightSensor(){
	// Allows initialisation of a light sensor array
}

void LightSensor::setup(int LS){
	// Stores pin value and sets pin mode to input
	sensor = LS;
}

int LightSensor::highLow(int input) {
	if(input == 1) {
		return HIGH;
	} else {
		return LOW;
	}
}

int LightSensor::read(){
	// Reads the light sensor, stores its value and returns it
	changeMUXChannel(sensor);
	// Serial.println(sensor);
    delayMicroseconds(1);
	readVal = analogRead(MUX_OUT);
	// if(sensor == 2) {
	// Serial.println(readVal);
	// }
	return readVal;
}

void LightSensor::changeMUXChannel(uint8_t LS) {
    // Change the multiplexer channel (0-31)
	digitalWrite(MUX_A4, (LS >> 4) & 0x1);
	digitalWrite(MUX_A3, (LS >> 3) & 0x1);
	digitalWrite(MUX_A2, (LS >> 2) & 0x1);
	digitalWrite(MUX_A1, (LS >> 1) & 0x1);
	digitalWrite(MUX_A0, LS & 0x1);
}

bool LightSensor::isOnWhite(){
	// Checks whether the light sensor is seeing white
	return readVal > threshold;
}

void LightSensor::setThresh(int val){
	threshold = val;
}
