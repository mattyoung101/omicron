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
#ifndef LIGHT_SENSOR_H_
#define LIGHT_SENSOR_H_

#include <Arduino.h>
#include <Utils.h>
#include <Config.h>
#include <Pinlist.h>

class LightSensor{
	public:
		LightSensor();
		int highLow(int input);

		void setup(int LS);
		int calibrate();

		int read();
		void changeMUXChannel(uint8_t LS);
		bool isOnWhite();
		void setThresh(int val);
		
		int threshold, readVal;
	private:
		int sensor;
};

#endif
