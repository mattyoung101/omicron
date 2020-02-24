#include "LightSensorController.h"
#include "Pinlist.h"

//TODO: Check LS Avoidance Functionality

//TODO: Clean up LS Code

LightSensorController::LightSensorController() {

}

void LightSensorController::setup(){
	// Sets up light sensors and calibrates
	Serial.print("yeeting");
	pinMode(MUX_A0, OUTPUT);
	pinMode(MUX_A1, OUTPUT);
	pinMode(MUX_A2, OUTPUT);
	pinMode(MUX_A3, OUTPUT);
	pinMode(MUX_A4, OUTPUT);
	pinMode(MUX_OUT, INPUT);

	for(int i = 0; i < LS_RING_NUM; i++){
		lsRing[i].setup(lsRingPins[i]);
	}
}

void LightSensorController::setThresholds() {
	for(int i = 0; i < LS_RING_NUM; i++){
		lsThresholds[i] = EEPROM.read(FIRST_CALIB_REGISTER + i * 2) + (EEPROM.read(FIRST_CALIB_REGISTER + i * 2 + 1) << 8);
		ringThresholds[i] = lsThresholds[i];
		lsRing[i].setThresh(lsThresholds[i]);
	}
}

void LightSensorController::calibrate(){
	int maxRead[LS_RING_NUM] = {0};
	int minRead[LS_RING_NUM] = {4095};
	int curRead;
	int calibs[LS_RING_NUM] = {0};
	
    readNum = 0;
	calibTimer.update();
	while(!calibTimer.timeHasPassedNoUpdate() || digitalRead(BUTTON1) == 0) {
		for(int i = 0; i < LS_RING_NUM; i++) {
			curRead = lsRing[i].read();
			// Serial.print(i);
			// Serial.print(" = ");
			// Serial.println(curRead);
			if(curRead > maxRead[i] || readNum == 0) {
				maxRead[i] = curRead;
			} else if (curRead < minRead[i] || readNum || minRead[i] == 0) {
				minRead[i] = curRead;
			} else if (readNum == 0) {
				minRead[i] = curRead;
				maxRead[i] = curRead;
			}
			readNum += 1;
		}
	}

	for(int i = 0; i < LS_RING_NUM; i++) {
        calibs[i] = ((maxRead[i] + minRead[i])/2);
        EEPROM.write(FIRST_CALIB_REGISTER + i * 2, calibs[i] & 0xFF);
        EEPROM.write(FIRST_CALIB_REGISTER + i * 2 + 1, calibs[i] >> 8);
    }
}

Vector LightSensorController::update(double heading){
	// Reads light sensors and calculates position of the line

	robotHeading = heading;
	read();
	line = calcLine();
	// Serial.print("Line angle = ");
	// Serial.println(line.arg);
	// Serial.print(heading);
	// Serial.print(" and ");
	line.arg = mod(line.arg + robotHeading, 360);
	// Serial.println(line.arg);
	if(firstFrame && line.mag != 0) {
		firstFrame = false;
		initialAngle = line.arg;
		prevAngle = angle;
		prevDepth = depth;
	}
	if(!wasOnLine && line.mag != 0 && !isOut) {
		initialAngle = line.arg;
	}
	if(line.mag != 0 && wasOnLine && !angleIsInside(line.arg, mod(prevAngle - 90, 360), mod(prevAngle + 90, 360))) {
		if(isOut) {
			isOut = false;
		} else {
			isOut = true;
		}
	}

	wasOnLine = (line.mag != 0);
	if(line.mag != 0.00) {
		prevAngle = line.arg;
		prevDepth = line.mag;
	}
	if(!isOut) {
		line = Vector(-line.mag, mod(line.arg + 180, 360));
	}
	if(isOut && line.mag == 0) {
		line = Vector(0.01, prevAngle);
	}
	line = Vector(constrain(line.mag, -0.99, 0.99), line.arg - robotHeading, true);
	// Serial.println(prevDepth);
	// Serial.print("PrevAngle = ");
	// Serial.println(prevAngle);
	// Serial.println(isOut);
		

	
	// Serial.print("Is the robot out? ");
	// Serial.print(isOut);
	// Serial.print(". Was it on the line? ");
	// Serial.print(wasOnLine);
	// Serial.print(". The initial angle was ");
	// Serial.println(initialAngle);
	return line;
}

void LightSensorController::read(){
	// Reads values and stores them for debug.
	// Assigns the current light Vector.
	ringCount = 0;
	lineVector = Vector(0, 0, false);
	for(int i = 0; i < LS_RING_NUM; i++){
		// Serial.print(i);
		// Serial.print(" : ");
		// Serial.print(lsThresholds[i]);
		// Serial.print(", ");
		// Serial.print(lsRing[i].read());
		// Serial.print(" = ");
		// Serial.println(lsRingOnWhite[i]);
		ringValues[i] = lsRing[i].read();
		lsRingOnWhite[i] = lsRing[i].isOnWhite();
		if(lsRingOnWhite[i] == true) {
			ringCount += lsRingOnWhite[i];
			isOnLine[i] = true;
 		} else {
			isOnLine[i] = false;
		}
	}

	for(int i = 0; i < LS_RING_NUM; i++) {
		if(!isOnLine[mod(i - 1, LS_RING_NUM)] && !isOnLine[mod(i +1, LS_RING_NUM)] && isOnLine[i]) {
			isOnLine[i] = false;
			ringCount -= 1;
		} else if(isOnLine[mod(i - 1, LS_RING_NUM)] && isOnLine[mod(i +1, LS_RING_NUM)] && !isOnLine[i]) {
			isOnLine[i] = true;
			ringCount += 1;
		}
	}
	// if(isOnLine[26] == true && isOnLine[29] == true) {
	// 	isOnLine[27] = true;
	// 	isOnLine[28] = true;
	// 	// Serial.println("hello im a fucking retard");
	// } else {
	// 	isOnLine[27] = false;
	// 	isOnLine[28] = false;
	// }

	// for(int i = 0; i < LS_RING_NUM; i++) {
	// 	// Serial.print(isOnLine[i]);
	// }
	// Serial.println("");
}

void LightSensorController::calculateClusters() {
    // Finds clusters of activated lightsensors
    // Reset Values
    numClusters = 0;
    findClusterStart = 1;
    for (int i = 0; i < 4; i++){
        starts[i] = -1;
        ends[i] = -1;
    }
	int counterShit = 0;
	for(int b = 0; b < LS_RING_NUM; b++) {
		if(isOnLine[b]) {
			counterShit += 1;
		}
	}
	if(counterShit == 32) {
        // All the light sensors are returning high
		for(int b = 0; b < LS_RING_NUM; b++) {
			isOnLine[b] = false;
		}
	}
    for (int i = 0; i < LS_RING_NUM; i++){ // Loop through lightSensors to find clusters
        if (findClusterStart){ //Find first cluster value
            if (isOnLine[i]){ 
                findClusterStart = 0;
                starts[numClusters] = i;
                numClusters += 1;
            }
        } else { //Found start of cluster, find how many sensors
            if (!isOnLine[i]){ // Cluster ended 1 ls ago
                findClusterStart = 1;
                ends[numClusters - 1] = i - 1;
            }
        }
    }
     //If final light sensor sees white end cluster before, on last ls
    if (isOnLine[LS_RING_NUM - 1]){
        ends[numClusters - 1] = LS_RING_NUM -1;
    }
     // If first and last light sensor see line, merge both clusters together
    if (isOnLine[LS_RING_NUM - 1])
    {
        ends[numClusters - 1] = LS_RING_NUM -1;
    }
    if (isOnLine[0] && isOnLine[LS_RING_NUM - 1])
    {
        starts[0] = starts[numClusters - 1];
        starts[numClusters - 1] = -1;
        ends[numClusters - 1] = -1;
        numClusters -=  1;
    }
}

Vector LightSensorController::calcLine(){

	lsMultiplier = 360/LS_RING_NUM;

    calculateClusters();
        
    if (numClusters > 0){
        double cluster1Angle = midAngleBetween(starts[0] * lsMultiplier, ends[0] * lsMultiplier);
        double cluster2Angle = midAngleBetween(starts[1] * lsMultiplier, ends[1] * lsMultiplier);
        double cluster3Angle = midAngleBetween(starts[2] * lsMultiplier, ends[2] * lsMultiplier);
		// Serial.print("There are ");
		// Serial.print(numClusters);
		// Serial.println(" clusters.");
		// Serial.println(cluster1Angle);

        if (numClusters == 1){
            angle = cluster1Angle;
            size = 1 - cos(DEG_RAD * angleBetween(starts[0] * lsMultiplier, ends[0] * lsMultiplier) / 2.0);

        } else if (numClusters ==2){
			// Serial.print("The angle between is ");
			// Serial.println(angleBetween(cluster1Angle, cluster2Angle));
			// Serial.print("The two involved angles are ");
			// Serial.print(cluster1Angle);
			// Serial.print(" and ");
			// Serial.println(cluster2Angle);
            angle = angleBetween(cluster1Angle, cluster2Angle) <= 180 ? midAngleBetween(cluster1Angle, cluster2Angle) : midAngleBetween(cluster2Angle, cluster1Angle);
            size = 1 - cos(DEG_RAD * angleBetween(cluster1Angle, cluster2Angle) <= 180 ? angleBetween(cluster1Angle, cluster2Angle) / 2.0 : angleBetween(cluster2Angle, cluster1Angle) / 2.0);

        } else {
            double angleDiff12 = angleBetween(cluster1Angle, cluster2Angle);
            double angleDiff23 = angleBetween(cluster2Angle, cluster3Angle);
            double angleDiff31 = angleBetween(cluster3Angle, cluster1Angle);
            double biggestAngle = max(angleDiff12, max(angleDiff23, angleDiff31));
            if (biggestAngle == angleDiff12){
                angle = midAngleBetween(cluster2Angle, cluster1Angle);
                size = angleBetween(cluster2Angle, cluster1Angle) <= 180 ? 1 - cos(DEG_RAD * (angleBetween(cluster2Angle, cluster1Angle) / 2.0)) : 1;
            } else if (biggestAngle == angleDiff23){
                angle = midAngleBetween(cluster3Angle, cluster2Angle);
                size = angleBetween(cluster3Angle, cluster2Angle) <= 180 ? 1 - cos(DEG_RAD * (angleBetween(cluster3Angle, cluster2Angle) / 2.0)) : 1;
            } else {
                angle = midAngleBetween(cluster1Angle, cluster3Angle);
                size = angleBetween(cluster1Angle, cluster3Angle) <= 180 ? 1 - cos(DEG_RAD * (angleBetween(cluster1Angle, cluster3Angle) / 2.0)) : 1;
            }
        }
    } else {
        angle = 0;
        size = 0;
    }
	return Vector(size, angle, true);
}
