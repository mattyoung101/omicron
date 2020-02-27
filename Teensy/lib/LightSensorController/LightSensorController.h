#ifndef LIGHT_SENSOR_CONTROLLER_H
#define LIGHT_SENSOR_CONTROLLER_H

#include <LightSensor.h>
#include <Utils.h>
#include <Vector.h>
#include <Config.h>
#include <EEPROM.h>
#include <Timer.h>
#include <Pinlist.h>


class LightSensorController{
	public:
		LightSensorController();

		void setup();
		void setThresholds();
		void calibrate();

		Vector update(double heading);

		int getLight(int pin);
		int getThresh(int pin);
		int getWhite(int pin);

		void setPriority(int priority);
		int getPriority();
		int getRelFieldAngle();

	private:
		void read();
		Vector calcLine();

		LightSensor lsRing[LS_RING_NUM];
		int lsRingPins[LS_RING_NUM] = {31, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30};

		int ringThresholds[LS_RING_NUM] = {0};

		int ringValues[LS_RING_NUM] = {0};

		bool lsRingOnWhite[LS_RING_NUM] = {false};

		bool isOnLine[LS_RING_NUM] = {false};

		Vector lsPointSet[LS_RING_NUM];

		Vector lineVector;

		float perpAngle;

		float prevAngle = -361;

		int ringCount = 0;

		bool isOut = false;
		
		double movementAngle;

		double depth;
            
		int lsThresholds[64] = {834, 967, 922, 699, 695, 795, 1183, 1052, 1163, 1203, 1213, 1301, 1366, 1275, 1202, 1008, 1135, 814, 1066, 1030, 1098, 1077, 1111, 1147, 874, 780, 999, 1043, 811, 1299, 1081, 939, 1455, 803, 899, 813, 860, 854, 1328, 950, 850, 791, 866, 1030, 1162, 1008, 915, 1018, 915, 1049, 1018, 964, 1003, 1166, 1255, 1052, 954, 950, 938, 975, 1098, 925, 920, 4095};
		const int lsIgnore[] = {};

		double robotHeading = 0;

		bool wasOnLine = false;

		float initialAngle;

		void calculateClusters();

		int numClusters = 0;

		int findClusterStart = 1;

		int starts[3] = {0};
		int ends[3] = {0};

		float size = 0;
		float angle = 0;
		float lsMultiplier;
		Vector line = Vector(0, 0, true);
		bool firstFrame = true;
		float prevDepth;
		Timer calibTimer = Timer(1000000);
		int readNum = 0;
};

#endif


