#include <Arduino.h>
#include "t3spi.h"

T3SPI spi = T3SPI();
volatile uint8_t inData[10] = {0};
volatile uint8_t outData[10] = {'T', 'E', 'E', 'N', 'S', 'Y', '2', 'E', 'S', 'P'};

static void gpio_spi_isr(void){
	Serial.println("Data received: ");

	// read incoming data
	uint8_t data = SPI0_POPR;
	while (data){
		Serial.printf("%c, ", data);
	}
	Serial.println("");

	// FIXME hardcoded
	for (int i = 0; i < 10; i++){
		SPI0_PUSHR_SLAVE = outData[i];
	}

	// send it
	SPI0_SR |= SPI_SR_RFDF;
}

void setup() {
	spi.begin_SLAVE(ALT_SCK, MOSI, MISO, CS0);
	spi.setCTAR_SLAVE(8, SPI_MODE0);

	attachInterrupt(15, gpio_spi_isr, LOW);
}

uint16_t i = 0;
void loop() {
	if (i++ > 10000){
		Serial.println("Alive");
		i = 0;
	}
}