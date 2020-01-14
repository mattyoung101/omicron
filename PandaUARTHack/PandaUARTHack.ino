// Hack to route UART from the Latte Panda out to the other devices
// NOTE: currently untested 

void setup(){
    Serial.begin(115200); // Panda -> ATMega
    Serial1.begin(115200); // ATMega -> rest of the robot
    Serial.println("UART hack initialised successfully");
} 

void loop(){
    while (Serial.available()){
        Serial1.write(Serial.read());
    }

    while (Serial1.available()){
        Serial.write(Serial1.read());
    }
}
