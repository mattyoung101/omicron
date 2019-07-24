#include "I2C.h"
#include <Pinlist.h>

void I2Cinit(){
    Wire1.begin(I2C_MASTER, 0x00, I2C_PINS_37_38, I2C_PULLUP_EXT, 400000);
    Wire1.setDefaultTimeout(10000);
}

void I2Cread(uint8_t address, uint8_t registerAddress, uint8_t nBytes, uint8_t *data) {
    Wire1.beginTransmission(address);
    Wire1.write(registerAddress);
    int statusCode = Wire1.endTransmission();
    // Serial.printf("read status code: %d\n", statusCode);

    Wire1.requestFrom(address, nBytes);
    uint8_t index = 0;
    while (Wire1.available()) {
        data[index] = Wire1.read();
        index++;
    }
}

void I2CwriteByte(uint8_t address, uint8_t registerAddress, uint8_t data) {
    Wire1.beginTransmission(address);
    Wire1.write(registerAddress);
    Wire1.write(data);
    int statusCode = Wire1.endTransmission();

    // Serial.printf("write status code: %d\n", statusCode);
}

void I2Cscan()
{
// scan for i2c devices
byte error, address;
int nDevices;

Serial.println("Scanning...");

nDevices = 0;
for(address = 1; address < 127; address++ )
{
 // The i2c_scanner uses the return value of
  // the Write.endTransmisstion to see if
 // a device did acknowledge to the address.
 Wire.beginTransmission(address);
 error = Wire1.endTransmission();

 if (error == 0)
 {
     Serial.print("I2C device found at address 0x");
   if (address<16)
     Serial.print("0");
   Serial.print(address,HEX);
   Serial.println("  !");

   nDevices++;
}
 else if (error==4)
{
  Serial.print("Unknow error at address 0x");
  if (address<16)
   Serial.print("0");
  Serial.println(address,HEX);
}
}
 if (nDevices == 0)
 Serial.println("No I2C devices found\n");
 else
 Serial.println("done\n");

 }