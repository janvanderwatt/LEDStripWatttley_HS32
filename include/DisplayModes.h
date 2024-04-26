#pragma once

#include <Arduino.h>

#include "LEDString.h"

#include <NeoPixelBus.h>

extern const char* WS2812FXJVDW_C_REV;

namespace DisplayMode {

typedef void (*InitialiseMode_t)();
typedef void (*DisplayMode_t)(LEDStripPixelInfo_t* dmi);

struct DisplayModeInfo_t {
    InitialiseMode_t initalise_mode;
    DisplayMode_t display_mode;
};

enum DisplayModeList {
    DISPLAY_MODE_FIREWORKS_RANDOM,
    DISPLAY_MODE_RAINBOW_CYCLE,
    DISPLAY_MODE_COMET,
    DISPLAY_MODE_FLASH_SPARKLE,
    DISPLAY_MODE_DUAL_SCAN,
    DISPLAY_MODE_TWINKLE_RANDOM,
    DISPLAY_MODE_FLICKER_IN_OUT,
    DISPLAY_MODE_STATIC,
    DISPLAY_MODES
};

extern DisplayModeInfo_t display_modes[DISPLAY_MODES];

} // namespace DisplayMode
