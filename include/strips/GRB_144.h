#pragma once

// --------------------------------------------------------------------------------------
// LED STRIP: Desktop
// --------------------------------------------------------------------------------------
#if defined(GRB_144)
#define NeoFeature NeoGrbwFeature
#define NeoColor RgbwColor
#define NeoMethod NeoEsp32I2s1X8800KbpsMethod

WS2812FX_SINGLE_SEGMENT_STRIP(PIXEL_GRB, 144);
#define OVERSAMPLING 4
#endif