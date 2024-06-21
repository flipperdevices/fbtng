/**
 * @file input_i.h
 * Input: internal API
 */

#pragma once

#include <input/input.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <furi.h>

#define INPUT_DEBOUNCE_TICKS 4
#define INPUT_DEBOUNCE_TICKS_HALF (INPUT_DEBOUNCE_TICKS / 2)
#define INPUT_PRESS_TICKS 150
#define INPUT_LONG_PRESS_COUNTS 2
#define INPUT_THREAD_FLAG_ISR 0x00000001

/** Input pin state */
typedef struct {
    // State
    InputKey key;
    volatile bool state;
    volatile uint8_t debounce;
    FuriTimer* press_timer;
    volatile uint8_t press_counter;
    volatile uint32_t counter;
} InputPinState;

/** Input state */
typedef struct {
    FuriThreadId thread_id;
    FuriPubSub* event_pubsub;
    InputPinState* pin_states;
    volatile uint32_t counter;
} Input;
