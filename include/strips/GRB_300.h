#pragma once

// --------------------------------------------------------------------------------------
// LED STRIP: Generic 300-pixel GRB (often 12V)
// --------------------------------------------------------------------------------------
#if defined(STRIP_4WF_OFFICE)
#define OVERSAMPLING_PWR2 0
#define OVERSAMPLING (1 << OVERSAMPLING_PWR2)
#define SHIFT 0

#define NeoFeature NeoGrbwFeature
#define NeoColor RgbwColor
#define NeoMethod NeoEsp32I2s1X8800KbpsMethod

#define WS2812FX_DEFAULT_COLOUR 0xc0c0c0
#define WS2812FX_DEFAULT_BRIGHTNESS 50
#define WS2812FX_DEFAULT_SPEED 128
#define WS2812FX_DEFAULT_MODE DISPLAY_MODE_COMET
#define WS2812FX_FADE_TIME_MS 1000

#define SEGMENTS 1 // the number of "individual" segments on each strip
#define STRIPS 1 // the number of separate strips that make up the virtual strip

#define DRIVER_PIN_0 GPIO_NUM_22 // the IO pin on which to output the LED data ("D1" when looking at Wemos D1 mini labels)

WS2812FX_SINGLE_SEGMENT_STRIP(PIXEL_GRB, 300, DRIVER_PIN_0);
#endif