#pragma once

#include "Arduino.h"

#include "LEDStrip.h"
#include <NeoPixelBus.h>

#define MODE_STORAGE 256

class LEDString;
struct LEDStripPixelInfo_t {
    LEDString* string;
    uint8_t strip_number;
    uint16_t usable_pixel_count = 0;
    uint32_t pixel_offset;
    bool run;
    bool reverse = false;
    uint32_t mode_start_time, now, time_since_start;
    uint8_t mode_index, previous_mode_index;
    uint32_t next_mode_change_time;
    RgbColor rgb;
    HslColor hsl;
    uint8_t storage[MODE_STORAGE];
};

class LEDString {
protected:
    void* void_strips;
    LEDStripPixelInfo_t lspi;

public:
    LEDString();

    void ClearTo(RgbColor c);
    void ClearOversamplingTo(RgbColor c, const uint8_t SHIFT);
    RgbColor GetStripPixel(uint16_t strip_pixel_index, bool use_direction);
    void SetStripPixel(uint16_t strip_pixel_index, RgbColor pixel_colour, bool use_direction);
    void SetSegmentPixel(uint8_t segment_index, uint16_t segment_pixel_index, RgbColor pixel_colour, bool use_direction);
    void MaterialiseOversampling(const uint8_t SHIFT);
    void Show();

    void SetBrightness(uint8_t brightness) { }
    void SetColorHSI(float h, float s, float l) { }
    void SetColorRGB(u_int8_t r, u_int8_t g, u_int8_t b) { }
    void SetSpeed(uint8_t speed) { }
    void SetInverted(uint8_t inverted) { }
    void SetMode360(float hue) { }
    void SetTransitionModesWithFading(uint8_t faing_on) { }

    uint8_t GetMode() { return 0; }

    uint8_t Strips();
    uint16_t Pixels(uint8_t strip_index);
    ws2812_pixeltype_t PixelType(uint8_t strip_index);
    uint8_t Segments();
    uint8_t SegmentPixels(uint8_t segment_index);

    void Update();

    static uint8_t sqrt_lookup[256];
};