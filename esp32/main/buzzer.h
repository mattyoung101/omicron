#pragma once
#include <defines.h>
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "driver/gpio.h"
#include "driver/mcpwm.h"
#include "driver/timer.h"
#include "esp_system.h"
#include "esp_log.h"

typedef struct {
    /** true if note on, false if note off **/
    bool type : 1;
    /** frequency of note: 27.5 Hz to 4186 Hz, max: 8192 Hz **/
    uint16_t frequency : 13;
    /** time the note is on for in ms, max: 65536 ms **/
    uint16_t time : 16;
    /** pad to 32 bits **/
    char : 2;
} music_note_t;

/** A buzzer, which music will be played through **/
typedef struct {
    /** The current note playing on the buzzer or none **/
    music_note_t currentNote;
    bool playing;
} music_dev_t;

/** Initialises buzzer pins **/
void buzzer_init(void);
/** Plays a frequency to a buzzer **/
void play_frequency(float frequency, uint8_t number, uint8_t duration, uint8_t volume);
/** Play inidividual voice (part, instrument, track, whatever) **/
void play_voice(music_note_t *voice, uint8_t buzzerNum, uint8_t volume);
/** Play entire song **/
void play_song(uint8_t volume);