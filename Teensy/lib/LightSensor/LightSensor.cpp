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

int LightSensor::calibrate(){
	// Calibrate light sensors using mean and std

	// Sample the light sensors a number of times
	int calibrationValues[LS_CALIBRATION_COUNT] = {0};
	for(int i = 0; i < LS_CALIBRATION_COUNT; i++){
		calibrationValues[i] = read();
		// delayMicroseconds(250);
	}

	// Find mean of the sample
	int meanSum = 0;
	for(int i = 0; i < LS_CALIBRATION_COUNT; i++){
		meanSum += calibrationValues[i];
	}
	double mean = ((double) meanSum) / ((double) LS_CALIBRATION_COUNT);
	// Serial.print(sensor.relPin);
	// Serial.print("\t");
	// Serial.println(mean);

	// Find the standard deviation of the sample
	double stdSum = 0.0;
	for(int i = 0; i < LS_CALIBRATION_COUNT; i++){
		stdSum += pow(calibrationValues[i] - mean,2);
	}
	double std = sqrt(stdSum / ((double) LS_CALIBRATION_COUNT - 1));
	// 99.7% of normal data lies within 3 standard deviations of the mean
	
	int aimed_thresh = round(mean + (LS_STD_SCALAR * std));
	int min_thresh = mean + LS_MIN_THRESH_JUMP;
	
	threshold = min(max(aimed_thresh, min_thresh), LS_MAX_THRESH);

	// for(int i = 0; i < LS_CALIBRATION_COUNT; i++){
	// 	Serial.print(calibrationValues[i]);
	// 	Serial.print("\t");
	// }
	// Serial.println();
	// Serial.print(mean);
	// Serial.print("\t");
	// Serial.println(std);
	// Serial.print("\t");
	// Serial.println(threshold);
	return threshold;
}

int LightSensor::read(){
	// Reads the light sensor, stores its value and returns it
	changeMUXChannel(sensor);
	// Serial.println(sensor);
	readVal = analogRead(LS_MUX1_AOUT);
	// if(sensor == 2) {
	// Serial.println(readVal);
	// }
	return readVal;
}

void LightSensor::changeMUXChannel(uint8_t LS) {
    // Change the multiplexer channel (0-31)
	digitalWrite(MUX1_A4, (LS >> 4) & 0x1);
	digitalWrite(MUX1_A3, (LS >> 3) & 0x1);
	digitalWrite(MUX1_A2, (LS >> 2) & 0x1);
	digitalWrite(MUX1_A1, (LS >> 1) & 0x1);
	digitalWrite(MUX1_A0, LS & 0x1);
}

bool LightSensor::isOnWhite(){
	// Checks whether the light sensor is seeing white
	return readVal > threshold;
}

void LightSensor::setThresh(int val){
	threshold = val;
}