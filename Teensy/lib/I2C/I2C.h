#ifndef I2C_H
#define I2C_H

#include <Pinlist.h>
#include <i2c_t3.h>

void I2Cread(uint8_t address, uint8_t registerAddress, uint8_t nBytes, uint8_t *data);
void I2CwriteByte(uint8_t address, uint8_t registerAddress, uint8_t data);
void I2Cinit(); // could be done better but boi gotta get his robot movin'
void I2Cscan();

#endif
