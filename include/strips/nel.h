#pragma once

// --------------------------------------------------------------------------------------
// LED STRIP: Morne's Room
// --------------------------------------------------------------------------------------
#if defined(MORNE_ROOM_STRIP) || defined(STRIP_72WHITE_MORNE_ROOM)
#define OVERSAMPLING_PWR2 0
#define OVERSAMPLING (1 << OVERSAMPLING_PWR2)
#define SHIFT 0

#define NeoColor RgbColor
#define NeoFeature NeoRgbFeature
#define NeoMethod NeoEsp32I2s1X8800KbpsMethod

// #define LED_COUNT_MAX 711
// WS2812FX_SINGLE_SEGMENT_STRIP(PIXEL_GRB, LED_COUNT_MAX);

#define WS2812FX_DEFAULT_COLOUR 0xc0c0c0
#define WS2812FX_DEFAULT_BRIGHTNESS 50
#define WS2812FX_DEFAULT_SPEED 128
#define WS2812FX_DEFAULT_MODE DISPLAY_MODE_COMET
#define WS2812FX_FADE_TIME_MS 1000

#define SEGMENTS 1 // the number of "individual" segments on each strip
#define STRIPS 2 // the number of separate strips that make up the virtual strip
#define LED_USABLE_0 300 // the number of useable WS2812B leds on the strip
#define DRIVER_PIN_0 GPIO_NUM_18 // the IO pin on which to output the LED data
#define LED_USABLE_1 411 // the number of useable WS2812B leds on the strip
#define DRIVER_PIN_1 GPIO_NUM_19 // the IO pin on which to output the LED data

#if LED_USABLE_0 >= LED_USABLE_1
#define LED_COUNT_MAX LED_USABLE_0 // the larger of the number of addressed WS2812B leds on the strips
#else
#define LED_COUNT_MAX LED_USABLE_1 // the number of addressed WS2812B leds on the strip (the strip is 300 LEDs)
#endif

#define OVERSAMPLING_BUFFER_SIZE (OVERSAMPLING * LED_COUNT_MAX)

static WS2812FX_power_model_t power_model = {
    .i_red = 10,
    .i_green = 10,
    .i_blue = 10,
    .i_white = 0,
    .i_supply_max = 8000
};

static WS2812FX_info_t pixel_info[STRIPS] = {
    // STRIP #0
    { .pixel_type = PIXEL_GRB,
        .pixel_pin = DRIVER_PIN_0,
        .total_pixel_count = LED_COUNT_MAX,
        .usable_pixel_count = LED_USABLE_0,
        .segments = SEGMENTS,
        // everywhere where you see a + 0 there is a pixel on the corner that might still work as part of the segment, but for now, they are black
        .segment_pixel_counts = (uint16_t[SEGMENTS]) { LED_USABLE_0 },
        .segment_offsets = (uint16_t[SEGMENTS]) { 0 },
        .segment_directions = NULL,
        .pixel_x = NULL,
        .pixel_y = NULL,
        .pixel_r = NULL,
        .pixel_a = NULL,
        .MAX_X = 32 * LED_COUNT_MAX,
        .MAX_Y = 32 * LED_COUNT_MAX,
        .MAX_R = 32 * LED_COUNT_MAX,
        .BAND_WIDTH = 16,
        .power_model = &power_model },
    // STRIP #1
    { .pixel_type = PIXEL_GRB,
        .pixel_pin = DRIVER_PIN_1,
        .total_pixel_count = LED_COUNT_MAX,
        .usable_pixel_count = LED_USABLE_1,
        .segments = SEGMENTS,
        // everywhere where you see a + 0 there is a pixel on the corner that might still work as part of the segment, but for now, they are black
        .segment_pixel_counts = (uint16_t[SEGMENTS]) { LED_USABLE_1 },
        .segment_offsets = (uint16_t[SEGMENTS]) { 0 },
        .segment_directions = NULL,
        .pixel_x = NULL,
        .pixel_y = NULL,
        .pixel_r = NULL,
        .pixel_a = NULL,
        .MAX_X = 32 * LED_COUNT_MAX,
        .MAX_Y = 32 * LED_COUNT_MAX,
        .MAX_R = 32 * LED_COUNT_MAX,
        .BAND_WIDTH = 16,
        .power_model = &power_model }
};
#endif // defined(MORNE_ROOM_STRIP) || defined(STRIP_72WHITE_MORNE_ROOM)

// --------------------------------------------------------------------------------------
// LED STRIP: TEST
// --------------------------------------------------------------------------------------
#if defined(MORNE_ROOM_TEST)
#define OVERSAMPLING_PWR2 0
#define OVERSAMPLING (1 << OVERSAMPLING_PWR2)
#define SHIFT 0

#define NeoColor RgbColor
#define NeoFeature NeoRgbFeature
#define NeoMethod NeoEsp32I2s1X8800KbpsMethod

#define WS2812FX_DEFAULT_COLOUR 0x40FFC0
#define WS2812FX_DEFAULT_BRIGHTNESS 50
#define WS2812FX_DEFAULT_SPEED 128
#define WS2812FX_DEFAULT_MODE DISPLAY_MODE_COMET
#define WS2812FX_FADE_TIME_MS 1000

#define SEGMENTS 1 // the number of "individual" segments on each strip
#define STRIPS 1 // the number of separate strips that make up the virtual strip
#define LED_USABLE_0 10 // the number of useable WS2812B leds on the strip
#define DRIVER_PIN_0 GPIO_NUM_18 // the IO pin on which to output the LED data
#define LED_COUNT_MAX LED_USABLE_0 // the larger of the number of addressed WS2812B leds on the strips
#define OVERSAMPLING_BUFFER_SIZE (OVERSAMPLING * LED_COUNT_MAX)

WS2812FX_SINGLE_SEGMENT_STRIP(PIXEL_GRBW, 30, DRIVER_PIN_0);
#endif // defined(MORNE_ROOM_STRIP) || defined(STRIP_72WHITE_MORNE_ROOM)