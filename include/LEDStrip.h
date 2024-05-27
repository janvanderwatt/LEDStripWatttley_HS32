#pragma once

#include "Arduino.h"
#include <NeoPixelBus.h>

#include "Types.h"

// ================================================================================================================
// CLASS: LEDString: A virtual string of LEDS
// ================================================================================================================

#define MODE_STORAGE 256

class LEDString;
struct LEDStripPixelInfo_t {
    LEDString* string;
    uint8_t oversampling, oversampling_pwr2;
    uint32_t pixel_offset;
    float pixel_fraction;
    bool run;
    bool reverse = false;
    uint8_t speed;
    uint32_t mode_start_time, now, time_since_start;
    int8_t mode_index = -1;
    uint32_t next_mode_change_time;
    RgbColor rgb;
    HslColor hsl;
    uint8_t storage[MODE_STORAGE];
    int32_t *R, *G, *B;
};

class LEDString {
protected:
    LEDStripPixelInfo_t lspi[2];
    LEDStripPixelInfo_t* running_lspi;
    uint8_t ModeIndex, currentIndex, previousIndex;
    int16_t fadeTimeMs;
    RgbColor RGB;

public:
    uint8_t Segments, FadingOn;
    uint16_t VirtualPixels;
    uint8_t Speed, Inverted, Brightness;
    HslColor HSL;

public:
    LEDString();

    void ClearTo(RgbColor c);
    RgbColor GetStripPixel(uint16_t strip_pixel_index, bool use_direction);
    void SetStripPixel(uint16_t strip_pixel_index, RgbColor pixel_colour, bool use_direction);
    void SetSegmentPixel(uint8_t segment_index, uint16_t segment_pixel_index, RgbColor pixel_colour, bool use_direction);

    void SetBrightness(uint8_t brightness);
    void SetColorHSI(float h, float s, float l);
    void SetColorRGB(uint32_t rgb);
    void SetColorRGB(uint8_t r, uint8_t g, u_int8_t b);
    void SetSpeed(uint8_t speed);
    void SetInverted(uint8_t inverted);
    void SetMode(uint8_t mode);
    void SetMode360(float hue);

    void StartModeTransition();
    void SetTransitionModesWithFading(uint8_t faing_on);
    void set_next_mode_time(LEDStripPixelInfo_t* lspi);
    void start_mode_time(uint32_t now, LEDStripPixelInfo_t* lspi);

    uint8_t GetMode();
    float GetMode360();
    uint8_t SegmentPixels(uint8_t segment_index);

    uint8_t Strips();
    uint16_t StripRealPixels(uint8_t strip_index);
    ws2812_pixeltype_t StripPixelType(uint8_t strip_index);
    uint8_t StripSegments(uint8_t strip_index);
    uint16_t StripSegmentOffset(uint8_t strip_index, uint8_t segment_index);
    uint16_t StripSegmentPixelCount(uint8_t strip_index, uint8_t segment_index);

    void RunMode(uint32_t now, LEDStripPixelInfo_t* lspi_to_run);
    void MaterialisePixelData(uint8_t time_delay_ms);
};