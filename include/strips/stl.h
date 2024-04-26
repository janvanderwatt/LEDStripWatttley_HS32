#pragma once

#if defined(STL_COMMON_CONFIGURATION)
#define OVERSAMPLING 4
#define NeoColor RgbwColor
#define NeoMethod NeoEsp32I2s1X8800KbpsMethod

#define WS2812FX_IO_PIN GPIO_NUM_22 // GPIO22 == D1
#define WS2812FX_DEFAULT_COLOUR 0x00CCCC
#define WS2812FX_DEFAULT_BRIGHTNESS 4
#define WS2812FX_DEFAULT_MODE 41
#define WS2812FX_DEFAULT_SPEED 32
#endif // defined(STL_COMMON_CONFIGURATION)

// --------------------------------------------------------------------------------------
// LED STRIP: Streicher City Kitchen
// --------------------------------------------------------------------------------------
#if defined(STL_KITCHEN_STRIP)
#define NeoFeature NeoGrbwFeature
WS2812FX_SINGLE_SEGMENT_STRIP(PIXEL_GRBW, 192);
#endif

// --------------------------------------------------------------------------------------
// LED STRIP: Streicher City Lounge
// --------------------------------------------------------------------------------------
#if defined(STL_LOUNGE_STRIP)
#define NeoFeature NeoGrbFeature
WS2812FX_SINGLE_SEGMENT_STRIP(PIXEL_GRB, 300);
#endif

// --------------------------------------------------------------------------------------
// LED STRIP: Streicher City Bedroom
// --------------------------------------------------------------------------------------
#if defined(STL_BEDROOM_STRIP)
#define NeoFeature NeoGrbwFeature
WS2812FX_SINGLE_SEGMENT_STRIP(PIXEL_GRBW, 87);
#endif

