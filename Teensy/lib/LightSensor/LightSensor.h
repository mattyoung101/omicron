#ifndef LIGHT_SENSOR_H_
#define LIGHT_SENSOR_H_

#include <Arduino.h>
#include <Common.h>
#include <Defines.h>
#include <Pins.h>
#include <MuxPins.h>

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