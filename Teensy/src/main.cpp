#include <Arduino.h>
#include "t3spi.h"

T3SPI spi = T3SPI();

void setup() {
	pinMode(13, OUTPUT);
	digitalWrite(13, HIGH);

	spi.begin_SLAVE(SCK, MOSI, MISO, CS0);
	spi.setCTAR_SLAVE(16, SPI_MODE0);

	// Enable the SPI0 Interrupt
  	NVIC_ENABLE_IRQ(IRQ_SPI0);
}

void loop() {
	// put your main code here, to run repeatedly:
}

//Interrupt Service Routine to handle incoming data
void spi0_isr(void){
	//Function to handle data
	SPI_SLAVE.rxtx8 (data, returnData, dataLength);
}