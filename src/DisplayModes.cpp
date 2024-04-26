#include <Arduino.h>

#include "DisplayModes.h"

const char* WS2812FXJVDW_C_REV = "3.00";

namespace DisplayMode {

void initialise()
{
    // Offer each mode the opportunity to initialise based on all the strips and segment configurations that they can see
    for (uint8_t i = 0; i < DISPLAY_MODES; i++) {
        if (display_modes[i].initalise_mode) {
            display_modes[i].initalise_mode();
        }
    }
}

// --------------------------------------------------------------------------------------
// PATTERN: comet
// --------------------------------------------------------------------------------------
void mode_comet(LEDStripPixelInfo_t* lspi)
{
    if (!lspi->run) {
        // Initialise the mode
        lspi->hsl.H = random(0, 360);
        lspi->hsl.H /= 360;
    } else {
        // Run the mode
        lspi->string->ClearTo(0);
        const uint16_t comets = lspi->usable_pixel_count / 20;
        const uint16_t increment = lspi->usable_pixel_count / comets;
        const uint16_t comet_length = increment / 2;
        uint16_t pixel_offset = lspi->pixel_offset >> 4;

        for (uint16_t i = 0; i < comet_length; i++) {
            float l = lspi->hsl.L;
            l *= i;
            l /= comet_length * 2;
            HslColor c(lspi->hsl.H, 1.0f, l);
            for (uint16_t comet = 0, pixel = i + pixel_offset; comet < comets; comet++, pixel += increment) {
                lspi->string->SetStripPixel(pixel, c, false);
            }
        }
    }
}

// --------------------------------------------------------------------------------------
// PATTERN: rotating rainbow
// --------------------------------------------------------------------------------------
void mode_rainbow_cycle(LEDStripPixelInfo_t* lspi)
{
    if (!lspi->run) {
        // Initialise the mode
    } else {
        // Run the mode
        float hue = 0, hue_increment = 1.0f / lspi->usable_pixel_count;
        uint16_t pixel_offset = lspi->pixel_offset >> 1;
        for (uint16_t pixel_index = 0; pixel_index < lspi->usable_pixel_count; pixel_index++) {
            HslColor c(hue, 1.0, 0.5);
            lspi->string->SetStripPixel(pixel_index + pixel_offset, c, false);
            hue += hue_increment;
        }
    }
}

// --------------------------------------------------------------------------------------
// PATTERN: sparkling
// --------------------------------------------------------------------------------------
void mode_flash_sparkle(LEDStripPixelInfo_t* lspi)
{
    if (!lspi->run) {
        // Initialise the mode
        lspi->hsl.H = random(0, 90);
        lspi->hsl.H -= 45;
        lspi->hsl.H /= 360;
    } else {
        // Run the mode
        lspi->string->ClearTo(HslColor(lspi->hsl.H, 1.0f, 0.5f));
        RgbColor white(255, 255, 255);
        uint16_t random_range = 50;
        for (uint16_t i = 0; i < lspi->usable_pixel_count; i++) {
            if (random(0, random_range) < 1) {
                lspi->string->SetStripPixel(i, white, false);
            }
        }
    }
}

// --------------------------------------------------------------------------------------
// PATTERN: fireworks with a random colour
// --------------------------------------------------------------------------------------
void mode_fireworks_random(LEDStripPixelInfo_t* lspi)
{
    if (!lspi->run) {
        // Initialise the mode
        lspi->string->ClearTo(0);
    } else {
        // Run the mode
        uint16_t px_r = 0;
        uint16_t px_g = 0;
        uint16_t px_b = 0;

        // set a background grey/smoke that is fairly constant regardless of the current brightness setting (looks a bit weird during transitions though)
        const uint16_t low_b = 2;

        for (uint16_t i = 0; i < lspi->usable_pixel_count; i++) {
            RgbColor px = lspi->string->GetStripPixel(i, false);

            px_r = px.R;
            px_g = px.G;
            px_b = px.B;
            // fade out (k=0.875)
            px.R = (92 * px_r) / 100;
            if (px.R < low_b) {
                px.R = low_b;
            }
            px.G = (92 * px_g) / 100;
            if (px.G < low_b) {
                px.G = low_b;
            }
            px.B = (92 * px_b) / 100;
            if (px.B < low_b) {
                px.B = low_b;
            }

            lspi->string->SetStripPixel(i, px, false);
        }

        RgbColor px_left(0), px_center(0), px_right(0);

        for (uint16_t i = 0; i < lspi->usable_pixel_count; i++) {
            // Check that we have passed the first LED before attempting to fetch the colour of the LEFT neighbour
            if (i > 0) {
                px_left = lspi->string->GetStripPixel(i - 1, false);
            }
            px_center = lspi->string->GetStripPixel(i, false);
            // Check that we have not yet reached the 2nd last LED before attempting to fetch the colour of the RIGHT neighbour
            if (i < (lspi->usable_pixel_count - 1)) {
                px_right = lspi->string->GetStripPixel(i + 1, false);
            }

            // set brightness(i) = ((brightness(i-1)/2 + brightness(i+1)) / 2) + brightness(i)
            px_r = 5 * ((((px_left.R >> 1) + (px_right.R >> 0)) >> 1) + px_center.R) >> 3;
            if (px_r >= 255) {
                px_r = 255;
            }
            px_g = 5 * ((((px_left.G >> 1) + (px_right.G >> 0)) >> 1) + px_center.G) >> 3;
            if (px_g >= 255) {
                px_g = 255;
            }
            px_b = 5 * ((((px_left.B >> 1) + (px_right.B >> 0)) >> 1) + px_center.B) >> 3;
            if (px_b >= 255) {
                px_b = 255;
            }

            lspi->string->SetStripPixel(i, RgbColor(px_r, px_g, px_b), false);
        }

        for (uint16_t i = 0; i < 1 + (lspi->usable_pixel_count / 20); i++) {
            if (random(0, 36) == 0) {
                HslColor c(((float)random(0, 360)) / 360.f, 1.0f, 0.5f);
                lspi->string->SetStripPixel(random(0, lspi->usable_pixel_count), c, false);
            }
        }
    }
}

// --------------------------------------------------------------------------------------
// PATTERN: flickering in and out
// --------------------------------------------------------------------------------------
void mode_flicker_in_out(LEDStripPixelInfo_t* lspi)
{
    if (!lspi->run) {
        // Initialise the mode
        lspi->hsl.H = random(220, 310);
        lspi->hsl.H /= 360;
    } else {
        const uint32_t FLICKER_TIME = 11000, RAMP_UP = FLICKER_TIME * 0.6;

        // Run the mode
        uint32_t time_since_start = lspi->time_since_start % FLICKER_TIME;
        if (time_since_start >= (FLICKER_TIME / 2))
            time_since_start = FLICKER_TIME - time_since_start;
        if (random(0, RAMP_UP) < time_since_start) {
            float l = time_since_start;
            l /= RAMP_UP * 2;
            if (l > 0.6f)
                l = 0.6f;
            lspi->string->ClearTo(HslColor(lspi->hsl.H, 1.0f - l / 10.0f, l));
            for (uint16_t i = 0; i < lspi->usable_pixel_count / 10; i++) {
                lspi->string->SetStripPixel(random(0, lspi->usable_pixel_count), RgbColor((uint8_t)(l * 256)), false);
            }
        } else {
            lspi->string->ClearTo(RgbColor(0));
        }
    }
}

// --------------------------------------------------------------------------------------
// PATTERN: scanner from both sides of each segment
// --------------------------------------------------------------------------------------
void mode_dual_scan(LEDStripPixelInfo_t* lspi)
{
    if (!lspi->run) {
        // Initialise the mode
        lspi->hsl.H = random(90, 180);
        lspi->hsl.H /= 360;
        lspi->hsl.S = 1.0f;
        lspi->hsl.L = 0.5f;
        lspi->rgb = RgbColor(lspi->hsl);
    } else {
        // Run the mode
        uint16_t TIME_SPAN = 2000;
        uint32_t time_since_start = lspi->time_since_start % TIME_SPAN;
        if (time_since_start >= (TIME_SPAN >> 1))
            time_since_start = TIME_SPAN - time_since_start;
        lspi->string->ClearTo(0);
        for (uint8_t segment_index = 0; segment_index < lspi->string->Segments(); segment_index++) {
            uint32_t i = lspi->string->SegmentPixels(segment_index);
            i *= time_since_start;
            i /= (TIME_SPAN >> 1);
            lspi->string->SetSegmentPixel(segment_index, i, lspi->rgb, false);
            lspi->string->SetSegmentPixel(segment_index, i, lspi->rgb, true);
        }
    }
}

// --------------------------------------------------------------------------------------
// PATTERN: Twinkle random
// --------------------------------------------------------------------------------------
void mode_twinkle_random(LEDStripPixelInfo_t* lspi)
{
    if (!lspi->run) {
        lspi->string->ClearTo(0);
    } else {
        lspi->hsl.H = random(0, 360);
        lspi->hsl.H /= 360;
        lspi->hsl.S = 1.0f;
        lspi->hsl.L = 0.5f;

        const uint8_t PIXELS = 30;
        const uint8_t RANDOM = 3;

        for (uint8_t i = 0; i < PIXELS; i++) {
            uint16_t p = random(0, lspi->usable_pixel_count);
            if (random(0, 10) < RANDOM) {
                lspi->string->SetStripPixel(p, RgbColor(0), false);
            } else {
                lspi->string->SetStripPixel(p, lspi->hsl, false);
            }
        }
    }
}

// --------------------------------------------------------------------------------------
// PATTERN: static
// --------------------------------------------------------------------------------------
void mode_static(LEDStripPixelInfo_t* lspi)
{
    if (!lspi->run) {
        // Initialise the mode
    } else {
        // Run the mode
        lspi->string->ClearTo(lspi->rgb);
    }
}

// --------------------------------------------------------------------------------------
// Initialise the list of modes
// --------------------------------------------------------------------------------------
DisplayModeInfo_t display_modes[DISPLAY_MODES] = {
    { NULL, &mode_fireworks_random },
    { NULL, &mode_rainbow_cycle },
    { NULL, &mode_comet },
    { NULL, &mode_flash_sparkle },
    { NULL, &mode_dual_scan },
    { NULL, &mode_twinkle_random },
    { NULL, &mode_flicker_in_out },
    { NULL, &mode_static }
};

} // namespace DisplayMode
