#pragma once

#include "Arduino.h"

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

#define WS2812FX_SINGLE_SEGMENT_STRIP(led_type, led_count, pin) static WS2812FX_info_t pixel_info[1] = { { .pixel_type = led_type, .pixel_pin = pin, .total_pixel_count = led_count, .usable_pixel_count = led_count, .segments = 1, .segment_pixel_counts = (uint16_t[1]) { led_count }, .segment_offsets = (uint16_t[1]) { 0 }, .segment_directions = NULL, .pixel_x = NULL, .pixel_y = NULL, .pixel_r = NULL, .pixel_a = NULL, .MAX_X = 32 * led_count, .MAX_Y = 32 * led_count, .MAX_R = 16 * led_count + 32, .BAND_WIDTH = 16 } }
#define WS2812FX_SINGLE_SEGMENT_STRIP_POWER_MODEL(led_type, led_count, pin, Ir, Ig, Ib, Iw, Isupply_max)                                  \
    static WS2812FX_power_model_t power_model = { .i_red = Ir, .i_green = Ig, .i_blue = Ib, .i_white = Iw, .i_supply_max = Isupply_max }; \
    static WS2812FX_info_t pixel_info[1] = { { .pixel_type = led_type, .pixel_pin = pin, .total_pixel_count = led_count, .usable_pixel_count = led_count, .segments = 1, .segment_pixel_counts = (uint16_t[1]) { led_count }, .segment_offsets = (uint16_t[1]) { 0 }, .segment_directions = NULL, .pixel_x = NULL, .pixel_y = NULL, .pixel_r = NULL, .pixel_a = NULL, .MAX_X = 32 * led_count, .MAX_Y = 32 * led_count, .MAX_R = 16 * led_count + 32, .BAND_WIDTH = 16, .power_model = &power_model } }
