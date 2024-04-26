#pragma once

#include "Arduino.h"

#include <NeoPixelBus.h>

#define WS2812FX_SINGLE_SEGMENT_STRIP(led_type, led_count) static WS2812FX_info_t pixel_info = { .pixel_type = led_type, .total_pixel_count = led_count, .segments = 1, .segment_pixel_counts = (uint16_t[1]) { led_count }, .segment_offsets = (uint16_t[1]) { 0 }, .segment_directions = NULL, .pixel_x = NULL, .pixel_y = NULL, .pixel_r = NULL, .pixel_a = NULL, .MAX_X = 32 * led_count, .MAX_Y = 32 * led_count, .MAX_R = 16 * led_count + 32, .BAND_WIDTH = 16 }
#define WS2812FX_SINGLE_SEGMENT_STRIP_POWER_MODEL(led_type, led_count, Ir, Ig, Ib, Iw, Isupply_max)                                       \
    static WS2812FX_power_model_t power_model = { .i_red = Ir, .i_green = Ig, .i_blue = Ib, .i_white = Iw, .i_supply_max = Isupply_max }; \
    static WS2812FX_info_t pixel_info = { .pixel_type = led_type, .total_pixel_count = led_count, .segments = 1, .segment_pixel_counts = (uint16_t[1]) { led_count }, .segment_offsets = (uint16_t[1]) { 0 }, .segment_directions = NULL, .pixel_x = NULL, .pixel_y = NULL, .pixel_r = NULL, .pixel_a = NULL, .MAX_X = 32 * led_count, .MAX_Y = 32 * led_count, .MAX_R = 16 * led_count + 32, .BAND_WIDTH = 16, .power_model = &power_model }

typedef enum {
    PIXEL_RGB = 0,
    PIXEL_GRB = 1,
    PIXEL_GRBW = 2,
    PIXEL_RGBW = 3,
    PIXEL_TYPES
} ws2812_pixeltype_t;

typedef enum {
    SD_LEFT = 0,
    SD_RIGHT = 1,
    SD_DOWN = 2,
    SD_UP = 3
} ws2812_segment_direction_t;

typedef struct
{
    const uint8_t i_red; // the current required to power the RED led at 100% brightness (mA)
    const uint8_t i_green; // the current required to power the GREEN led at 100% brightness (mA)
    const uint8_t i_blue; // the current required to power the BLUE led at 100% brightness (mA)
    const uint8_t i_white; // the current required to power the WHITE led at 100% brightness  (mA)(leave 0 if there is no WHITE led)
    const uint16_t i_supply_max; // the maximum current the the power supply can deliver (mA)
} WS2812FX_power_model_t;

typedef struct
{
    const ws2812_pixeltype_t pixel_type;
    const uint8_t pixel_pin;
    const uint16_t total_pixel_count; // number of LEDs in the strip
    uint16_t usable_pixel_count; // number of LEDs in the strip that can be used for display (sum of segment counts)
    const uint8_t segments;
    const uint16_t* segment_pixel_counts;
    const uint16_t* segment_offsets;
    const ws2812_segment_direction_t* segment_directions;
    const int16_t* pixel_x;
    const int16_t* pixel_y;
    const uint16_t* pixel_r;
    const uint16_t* pixel_a;
    const uint16_t MAX_X;
    const uint16_t MAX_Y;
    const uint16_t MAX_R;
    const uint16_t BAND_WIDTH;
    const WS2812FX_power_model_t* power_model;
} WS2812FX_info_t;

template <typename T_COLOR_TYPE, typename T_COLOR_FEATURE, typename T_METHOD>
class LEDStrip {
protected:
    NeoPixelBus<T_COLOR_FEATURE, T_METHOD>* strip;
    WS2812FX_info_t* pixel_info;

    // uint16_t strip_pixel_lookup[LED_COUNT_MAX][2], segment_pixel_lookup[LED_COUNT_MAX][2] /*, segment_lookup[LED_COUNT_MAX]*/;
    uint16_t *strip_pixel_lookup, *segment_pixel_lookup;

public:
    LEDStrip(NeoPixelBus<T_COLOR_FEATURE, T_METHOD>* strip, WS2812FX_info_t* pixel_info, const uint8_t oversampling);

    void ClearTo(T_COLOR_TYPE c);
    void ClearOversamplingTo(T_COLOR_TYPE c, const uint8_t SHIFT);
    T_COLOR_TYPE GetStripPixel(uint16_t strip_pixel_index, bool use_direction);
    void SetStripPixel(uint16_t strip_pixel_index, T_COLOR_TYPE pixel_colour, bool use_direction);
    void SetSegmentPixel(uint8_t segment_index, uint16_t segment_pixel_index, T_COLOR_TYPE pixel_colour, bool use_direction);
    void MaterialiseOversampling(const uint8_t SHIFT, const uint8_t OVERSAMPLING);
    void Show();

    // int32_t r[OVERSAMPLING_BUFFER_SIZE], g[OVERSAMPLING_BUFFER_SIZE], b[OVERSAMPLING_BUFFER_SIZE];
    int32_t *r, *g, *b;
    static uint8_t sqrt_lookup[256];
};

template <typename T_COLOR_TYPE, typename T_COLOR_FEATURE, typename T_METHOD>
LEDStrip<T_COLOR_TYPE, T_COLOR_FEATURE, T_METHOD>::LEDStrip(NeoPixelBus<T_COLOR_FEATURE, T_METHOD>* strip, WS2812FX_info_t* pixel_info, const uint8_t oversampling)
{
    for (uint16_t i = 0; i < 256; i++) {
        sqrt_lookup[i] = (i * i) >> 8;
        // Serial.printf("%d/", sqrt_lookup[i]);
    }

    this->strip = strip;
    this->pixel_info = pixel_info;
    strip->Begin(); // this includes a clear to 0

    // set up the pixel index lookup
    uint16_t usable_pixel_count = 0;
    for (uint8_t segment_index = 0; segment_index < pixel_info->segments; segment_index++) {
        Serial.printf("seg[%d]:", segment_index);
        usable_pixel_count += pixel_info->segment_pixel_counts[segment_index];
    }
    pixel_info->usable_pixel_count = usable_pixel_count;
    strip_pixel_lookup = new uint16_t[2 * usable_pixel_count];
    segment_pixel_lookup = new uint16_t[2 * usable_pixel_count];

    uint16_t strip_pixel_index = 0;
    for (uint8_t segment_index = 0; segment_index < pixel_info->segments; segment_index++) {
        uint16_t p = pixel_info->segment_offsets[segment_index];
        uint16_t q = p + pixel_info->segment_pixel_counts[segment_index] - 1;
        bool use_inverse_direction = (pixel_info->segment_directions[segment_index] == SD_LEFT || pixel_info->segment_directions[segment_index] == SD_DOWN);
        for (uint16_t offset_index = 0; offset_index < pixel_info->segment_pixel_counts[segment_index]; offset_index++) {
            strip_pixel_lookup[strip_pixel_index] = p;
            strip_pixel_lookup[strip_pixel_index + usable_pixel_count] = use_inverse_direction ? q : p;
            segment_pixel_lookup[p] = p;
            segment_pixel_lookup[p + usable_pixel_count] = use_inverse_direction ? q : p;
            Serial.printf("%d/%d,", strip_pixel_lookup[strip_pixel_index], strip_pixel_lookup[strip_pixel_index + usable_pixel_count]);
            // segment_lookup[strip_pixel_index_0] = segment_index;
            p++;
            q--;
            strip_pixel_index++;
        }
    }
    Serial.printf(",usable[%d])\n", pixel_info->usable_pixel_count);
}

// --------------------------------------------------------------------------------------
// Get the colour of a pixel in the STRING
// --------------------------------------------------------------------------------------
template <typename T_COLOR_TYPE, typename T_COLOR_FEATURE, typename T_METHOD>
T_COLOR_TYPE LEDStrip<T_COLOR_TYPE, T_COLOR_FEATURE, T_METHOD>::GetStripPixel(uint16_t strip_pixel_index, bool use_direction)
{
    // if the pixel index is negative, it will wrap in the UINT space, and exceed the usable count.
    // before taking the MOD, add back the usable count, so that the number is smaller than the usable count in UINT space.
    if (strip_pixel_index >= pixel_info->usable_pixel_count) {
        strip_pixel_index += pixel_info->usable_pixel_count;
        strip_pixel_index %= pixel_info->usable_pixel_count;
    }
    // if the segment direction must be used, add the pixel count to the offset to get to from array [] to array [1].
    if (use_direction)
        strip_pixel_index += pixel_info->usable_pixel_count;
    return (
        strip->GetPixelColor(strip_pixel_lookup[strip_pixel_index]));
}

// --------------------------------------------------------------------------------------
// Set the colour of a pixel in the STRING
// --------------------------------------------------------------------------------------
template <typename T_COLOR_TYPE, typename T_COLOR_FEATURE, typename T_METHOD>
void LEDStrip<T_COLOR_TYPE, T_COLOR_FEATURE, T_METHOD>::SetStripPixel(uint16_t strip_pixel_index, T_COLOR_TYPE pixel_colour, bool use_direction)
{
    // if the pixel index is negative, it will wrap in the UINT space, and exceed the usable count.
    // before taking the MOD, add back the usable count, so that the number is smaller than the usable count in UINT space.
    if (strip_pixel_index >= pixel_info->usable_pixel_count) {
        strip_pixel_index += pixel_info->usable_pixel_count;
        strip_pixel_index %= pixel_info->usable_pixel_count;
    }
    // if the segment direction must be used, add the pixel count to the offset to get to from array [] to array [1].
    if (use_direction)
        strip_pixel_index += pixel_info->usable_pixel_count;
    strip->SetPixelColor(strip_pixel_lookup[strip_pixel_index], pixel_colour);
}

// --------------------------------------------------------------------------------------
// Set the colour of a pixel in a SEGMENT
// --------------------------------------------------------------------------------------
template <typename T_COLOR_TYPE, typename T_COLOR_FEATURE, typename T_METHOD>
void LEDStrip<T_COLOR_TYPE, T_COLOR_FEATURE, T_METHOD>::SetSegmentPixel(uint8_t segment_index, uint16_t segment_pixel_index, T_COLOR_TYPE pixel_colour, bool use_direction)
{
    // if the pixel index is negative, it will wrap in the UINT space, and exceed the usable count.
    // before taking the MOD, add back the usable count, so that the number is smaller than the usable count in UINT space.
    if (segment_pixel_index >= pixel_info->segment_pixel_counts[segment_index]) {
        segment_pixel_index += pixel_info->segment_pixel_counts[segment_index];
        segment_pixel_index %= pixel_info->segment_pixel_counts[segment_index];
    }
    segment_pixel_index += pixel_info->segment_offsets[segment_index];
    // if the segment direction must be used, add the pixel count to the offset to get to from array [] to array [1].
    if (use_direction)
        segment_pixel_index += pixel_info->usable_pixel_count;
    // Serial.printf("lu[S%d,P%d,U%d,%d]", strip_index, segment_pixel_index, use_direction & 0x01, segment_pixel_lookup[segment_pixel_index][use_direction & 0x01]);
    strip->SetPixelColor(segment_pixel_lookup[segment_pixel_index], pixel_colour);
}

// --------------------------------------------------------------------------------------
// Set all the pixels in a strip to the specified colour
// --------------------------------------------------------------------------------------
template <typename T_COLOR_TYPE, typename T_COLOR_FEATURE, typename T_METHOD>
void LEDStrip<T_COLOR_TYPE, T_COLOR_FEATURE, T_METHOD>::ClearTo(T_COLOR_TYPE c)
{
    for (uint16_t i = 0; i < pixel_info->usable_pixel_count; i++) {
        SetStripPixel(i, c, false);
    }
}

// --------------------------------------------------------------------------------------
// Turn the oversampling buffer into a set of pixels that can be output
// --------------------------------------------------------------------------------------
template <typename T_COLOR_TYPE, typename T_COLOR_FEATURE, typename T_METHOD>
void LEDStrip<T_COLOR_TYPE, T_COLOR_FEATURE, T_METHOD>::MaterialiseOversampling(const uint8_t SHIFT, const uint8_t OVERSAMPLING)
{
    for (uint16_t pixel = 0, sample = 0; pixel < pixel_info->usable_pixel_count; pixel++) {
        int32_t pr = r[sample] >> SHIFT, pg = g[sample] >> SHIFT, pb = b[sample] >> SHIFT;
        sample++;
        for (uint8_t os = 1; os < OVERSAMPLING; os++) {
            pr += r[sample] >> SHIFT;
            pg += g[sample] >> SHIFT;
            pb += b[sample] >> SHIFT;
            sample++;
        }
        pr /= OVERSAMPLING;
        pg /= OVERSAMPLING;
        pb /= OVERSAMPLING;
        if (pr > 255)
            pr = 255;
        if (pg > 255)
            pg = 255;
        if (pb > 255)
            pb = 255;
        T_COLOR_TYPE c(sqrt_lookup[pr], sqrt_lookup[pg], sqrt_lookup[pb]);
        SetStripPixel(pixel, c, false);
    }
}

template <typename T_COLOR_TYPE, typename T_COLOR_FEATURE, typename T_METHOD>
void LEDStrip<T_COLOR_TYPE, T_COLOR_FEATURE, T_METHOD>::Show()
{
    strip->Show();
};