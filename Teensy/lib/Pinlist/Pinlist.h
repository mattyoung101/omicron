//Header file for defining pins used
#ifndef PINLIST_H
#define PINLIST_H

#include <Arduino.h>
#include <Config.h>

// --- Light Sensors --- //

// #define MUX_EN 43 // TODO: FIX
#define MUX_A0 6
#define MUX_A1 5
#define MUX_A2 4
#define MUX_A3 3
#define MUX_A4 2
// #define MUX_WR 44
#define MUX_OUT 22

// --- Serial --- //

#define ESPSERIAL Serial1

// --- Debug --- //

#define BUTTON1 0 // TODO: FIX

// --- Lightgate --- //

#define FRONTGATE A0 // TODO: FIX
#define BACKGATE A1

#endif // PINLIST_H