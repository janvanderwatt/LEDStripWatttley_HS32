#include <Arduino.h>

#include "Configuration.h"
#include "DisplayModes.h"
#include "LedConfigurations.h"

#include "LEDString.h"
#include "LEDStrip.h"

uint8_t LEDString::sqrt_lookup[256];

void set_next_mode_time(LEDStripPixelInfo_t* lspi)
{
    lspi->next_mode_change_time = 1; // transition time - make it a global!
    lspi->next_mode_change_time *= 1000;
    lspi->next_mode_change_time += lspi->now;
}

void start_mode_time(uint32_t now, LEDStripPixelInfo_t* lspi)
{
    lspi->now = now;
    lspi->mode_start_time = now;
    lspi->time_since_start = 0;
    set_next_mode_time(lspi);
    // clear the mode storage
    memset(lspi->storage, 0, MODE_STORAGE);
    // call the mode with the RUN flag clear, to allow it to set up internal structures
    lspi->run = false;
    DisplayMode::display_modes[lspi->mode_index].display_mode(lspi);
    // when the mode is next called, the RUN flag is set
    lspi->run = true;
}

LEDString::LEDString()
{
    uint32_t now = millis();
    LEDStrip<NeoColor, NeoFeature, NeoMethod>* strip[STRIPS];
    NeoPixelBus<NeoFeature, NeoMethod>* bus_ptr;
    LEDStrip<NeoColor, NeoFeature, NeoMethod>* strip_ptr;
    for (uint8_t strip_index = 0; strip_index < STRIPS; strip_index++) {
        bus_ptr = new NeoPixelBus<NeoFeature, NeoMethod>(pixel_info[strip_index].total_pixel_count, pixel_info[strip_index].pixel_pin);
        strip_ptr = new LEDStrip<NeoColor, NeoFeature, NeoMethod>(bus_ptr, &pixel_info[strip_index], OVERSAMPLING);
        strip[strip_index] = strip_ptr;
    }

    lspi.pixel_offset = 0;
    uint8_t DISPLAY_STARTING_MODE = 0;
    lspi.mode_index = DISPLAY_STARTING_MODE;
    start_mode_time(now, &lspi);
}

// --------------------------------------------------------------------------------------
// Get the number of strips that make up the virtual strip
// --------------------------------------------------------------------------------------
uint8_t LEDString::Strips()
{
    return (STRIPS);
}

// --------------------------------------------------------------------------------------
// Get the number of pixels in a strip
// --------------------------------------------------------------------------------------
uint16_t LEDString::Pixels(uint8_t strip_index)
{
    return pixel_info[strip_index].total_pixel_count;
}

// --------------------------------------------------------------------------------------
// Get the pixel type of a strip
// --------------------------------------------------------------------------------------
ws2812_pixeltype_t LEDString::PixelType(uint8_t strip_index)
{
    return pixel_info[strip_index].pixel_type;
}

// --------------------------------------------------------------------------------------
// Return the number of segments on the string
// --------------------------------------------------------------------------------------
uint8_t LEDString::Segments()
{
    uint8_t segments = 0;
    for (uint8_t strip_index = 0; strip_index < STRIPS; strip_index++) {
        segments += pixel_info[strip_index].segments;
    }
    return (segments);
}

// --------------------------------------------------------------------------------------
// Return the number of pixels in a segment on the string
// --------------------------------------------------------------------------------------
uint8_t LEDString::SegmentPixels(uint8_t segment_index)
{
    uint8_t strip_index = 0;
    while (segment_index >= pixel_info[strip_index].segments) {
        segment_index -= pixel_info[strip_index].segments;
        strip_index++;
    }
    return (pixel_info[strip_index].segment_pixel_counts[segment_index]);
}

// --------------------------------------------------------------------------------------
// Get the colour of a pixel in the STRING
// --------------------------------------------------------------------------------------
RgbColor LEDString::GetStripPixel(uint16_t strip_pixel_index, bool use_direction)
{
    LEDStrip<NeoColor, NeoFeature, NeoMethod>* strips = (LEDStrip<NeoColor, NeoFeature, NeoMethod>*)void_strips;
    uint8_t strip_index = 0;
    while (strip_pixel_index >= pixel_info[strip_index].usable_pixel_count) {
        strip_pixel_index -= pixel_info[strip_index].usable_pixel_count;
        strip_index++;
    }
    NeoColor result = strips[strip_index].GetStripPixel(strip_pixel_index, use_direction);
    return (RgbColor(0, 0, 0));
}

// --------------------------------------------------------------------------------------
// Set the colour of a pixel in the STRING
// --------------------------------------------------------------------------------------
void LEDString::SetStripPixel(uint16_t strip_pixel_index, RgbColor pixel_colour, bool use_direction)
{
    LEDStrip<NeoColor, NeoFeature, NeoMethod>* strips = (LEDStrip<NeoColor, NeoFeature, NeoMethod>*)void_strips;
    uint8_t strip_index = 0;
    while (strip_pixel_index >= pixel_info[strip_index].usable_pixel_count) {
        strip_pixel_index -= pixel_info[strip_index].usable_pixel_count;
        strip_index++;
    }
    strips[strip_index].SetStripPixel(strip_pixel_index, NeoColor(pixel_colour), use_direction);
}

// --------------------------------------------------------------------------------------
// Set the colour of a pixel in a SEGMENT
// --------------------------------------------------------------------------------------
void LEDString::SetSegmentPixel(uint8_t segment_index, uint16_t segment_pixel_index, RgbColor pixel_colour, bool use_direction)
{
    LEDStrip<NeoColor, NeoFeature, NeoMethod>* strips = (LEDStrip<NeoColor, NeoFeature, NeoMethod>*)void_strips;
    uint8_t strip_index = 0;
    while (segment_index >= pixel_info[strip_index].segments) {
        segment_index -= pixel_info[strip_index].segments;
        strip_index++;
    }
    strips[strip_index].SetSegmentPixel(segment_index, segment_pixel_index, NeoColor(pixel_colour), use_direction);
}

// --------------------------------------------------------------------------------------
// Set all the pixels in a strip to the specified colour
// --------------------------------------------------------------------------------------
void LEDString::ClearTo(RgbColor c)
{
    for (uint16_t i = 0; i < pixel_info->usable_pixel_count; i++) {
        SetStripPixel(i, c, false);
    }
}

void LEDString::ClearOversamplingTo(RgbColor c, const uint8_t SHIFT)
{
    LEDStrip<NeoColor, NeoFeature, NeoMethod>* strips = (LEDStrip<NeoColor, NeoFeature, NeoMethod>*)void_strips;
    for (uint8_t strip_index = 0; strip_index < STRIPS; strip_index++) {
        strips[strip_index].ClearOversamplingTo(c, SHIFT);
    }
}

void LEDString::MaterialiseOversampling(const uint8_t SHIFT)
{
    LEDStrip<NeoColor, NeoFeature, NeoMethod>* strips = (LEDStrip<NeoColor, NeoFeature, NeoMethod>*)void_strips;
    for (uint8_t strip_index = 0; strip_index < STRIPS; strip_index++) {
        strips[strip_index].MaterialiseOversampling(SHIFT, OVERSAMPLING);
    }
}

void LEDString::Update()
{
    uint32_t now = millis();
    LEDStrip<NeoColor, NeoFeature, NeoMethod>* strips = (LEDStrip<NeoColor, NeoFeature, NeoMethod>*)void_strips;
    lspi.now = now;
    lspi.time_since_start = now - lspi.mode_start_time;
    DisplayMode::display_modes[lspi.mode_index].display_mode(&lspi);
    for (uint8_t strip_index = 0; strip_index < STRIPS; strip_index++) {
        // Serial.printf("DM[%d:%d]", strip_index, display_mode_index[strip_index]);
        strips[strip_index].Show();
    }

    lspi.pixel_offset += 3;
    lspi.pixel_offset %= ((LED_COUNT_MAX * OVERSAMPLING) << 4);
};
