#include <Arduino.h>
#include "t3spi.h"

T3SPI spi = T3SPI();
volatile uint8_t inData[10] = {0};
volatile uint8_t outData[10] = {'T', 'E', 'E', 'N', 'S', 'Y', '2', 'E', 'S', 'P'};

void setup() {
	spi.begin_SLAVE(SCK, MOSI, MISO, CS0_ActiveLOW);
	spi.setCTAR_SLAVE(8, SPI_MODE0);

	// Enable the SPI0 Interrupt
  	NVIC_ENABLE_IRQ(IRQ_SPI0);
}

void loop() {
	// Serial.println("Fuck");
}

void spi0_isr(void){
	spi.rxtx8(inData, outData, 10);

	Serial.print("Data received: ");
	for (int i = 0; i < 10; i++){
		Serial.print((char) inData[i]);
	}
	Serial.println();
}