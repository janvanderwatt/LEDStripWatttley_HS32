#include <Arduino.h>

#include "LEDStrip.h"
#include "DisplayModes.h"

#include "Configuration.h"
#include "LedConfigurations.h"

extern uint8_t sqrt_lookup[256];
const uint8_t SHIFT = 0;

// ================================================================================================================
// CLASS: LEDStrip: A strip of LEDs connected to a pin
// ================================================================================================================
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
    void Show();
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
        bool use_inverse_direction = false;
        if (pixel_info->segment_directions) {
            use_inverse_direction = (pixel_info->segment_directions[segment_index] == SD_LEFT || pixel_info->segment_directions[segment_index] == SD_DOWN);
        }
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
    strip->SetPixelColor(strip_pixel_lookup[strip_pixel_index], NeoColor(pixel_colour));
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
    strip->SetPixelColor(segment_pixel_lookup[segment_pixel_index], NeoColor(pixel_colour));
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

template <typename T_COLOR_TYPE, typename T_COLOR_FEATURE, typename T_METHOD>
void LEDStrip<T_COLOR_TYPE, T_COLOR_FEATURE, T_METHOD>::Show()
{
    strip->Show();
};

uint8_t sqrt_lookup[256];

// ================================================================================================================
// CLASS: LEDString: A virtual string of LEDS
// ================================================================================================================
LEDStrip<NeoColor, NeoFeature, NeoMethod>* strips[STRIPS];

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
    Segments = 0;
    Pixels = 0;
    for (uint8_t strip_index = 0; strip_index < STRIPS; strip_index++) {
        Segments += pixel_info[strip_index].segments;
        Pixels += pixel_info[strip_index].usable_pixel_count;
    }
    Pixels *= OVERSAMPLING;

    NeoPixelBus<NeoFeature, NeoMethod>* bus_ptr;
    LEDStrip<NeoColor, NeoFeature, NeoMethod>* strip_ptr;
    for (uint8_t strip_index = 0; strip_index < STRIPS; strip_index++) {
        bus_ptr = new NeoPixelBus<NeoFeature, NeoMethod>(pixel_info[strip_index].total_pixel_count, pixel_info[strip_index].pixel_pin);
        strip_ptr = new LEDStrip<NeoColor, NeoFeature, NeoMethod>(bus_ptr, &pixel_info[strip_index], OVERSAMPLING);
        strips[strip_index] = strip_ptr;
    }

    lspi.string = this;
    lspi.virtual_pixel_count = Pixels;
    lspi.reverse = false;

    lspi.pixel_offset = 0;

    R = new int32_t[Pixels];
    G = new int32_t[Pixels];
    B = new int32_t[Pixels];
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
uint16_t LEDString::StripRealPixels(uint8_t strip_index)
{
    return pixel_info[strip_index].usable_pixel_count;
}

// --------------------------------------------------------------------------------------
// Get the pixel type of a strip
// --------------------------------------------------------------------------------------
ws2812_pixeltype_t LEDString::StripPixelType(uint8_t strip_index)
{
    return pixel_info[strip_index].pixel_type;
}

// --------------------------------------------------------------------------------------
// Return the number of segments in a strip
// --------------------------------------------------------------------------------------
uint8_t LEDString::StripSegments(uint8_t strip_index)
{
    return (pixel_info[strip_index].segments);
}

// --------------------------------------------------------------------------------------
// Return the offset of a segment on a strip
// --------------------------------------------------------------------------------------
uint16_t LEDString::StripSegmentOffset(uint8_t strip_index, uint8_t segment_index)
{
    return (pixel_info[strip_index].segment_offsets[segment_index]);
}

uint16_t LEDString::StripSegmentPixelCount(uint8_t strip_index, uint8_t segment_index)
{
    return (pixel_info[strip_index].segment_pixel_counts[segment_index]);
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
    // if the pixel index is negative, it will wrap in the UINT space, and exceed the usable count.
    // before taking the MOD, add back the usable count, so that the number is smaller than the usable count in UINT space.
    if (strip_pixel_index >= Pixels) {
        strip_pixel_index += Pixels;
        strip_pixel_index %= Pixels;
    }
    return (RgbColor(R[strip_pixel_index] >> SHIFT, G[strip_pixel_index] >> SHIFT, B[strip_pixel_index] >> SHIFT));
}

// --------------------------------------------------------------------------------------
// Set the colour of a pixel in the STRING
// --------------------------------------------------------------------------------------
void LEDString::SetStripPixel(uint16_t strip_pixel_index, RgbColor pixel_colour, bool use_direction)
{
    // if the pixel index is negative, it will wrap in the UINT space, and exceed the usable count.
    // before taking the MOD, add back the usable count, so that the number is smaller than the usable count in UINT space.
    if (strip_pixel_index >= Pixels) {
        strip_pixel_index += Pixels;
        strip_pixel_index %= Pixels;
    }
    R[strip_pixel_index] = pixel_colour.R << SHIFT;
    G[strip_pixel_index] = pixel_colour.G << SHIFT;
    B[strip_pixel_index] = pixel_colour.B << SHIFT;
}

// --------------------------------------------------------------------------------------
// Set the colour of a pixel in a SEGMENT
// --------------------------------------------------------------------------------------
void LEDString::SetSegmentPixel(uint8_t segment_index, uint16_t segment_pixel_index, RgbColor pixel_colour, bool use_direction)
{
    uint8_t strip_index = 0;
    while (segment_index >= pixel_info[strip_index].segments) {
        segment_index -= pixel_info[strip_index].segments;
        strip_index++;
    }
    if (strip_index < STRIPS) {
        strips[strip_index]->SetSegmentPixel(segment_index, segment_pixel_index, NeoColor(pixel_colour), use_direction);
    } else {
        Serial.printf("@");
    }
}

// --------------------------------------------------------------------------------------
// Set all the pixels in a strip to the specified colour
// --------------------------------------------------------------------------------------
void LEDString::ClearTo(RgbColor c)
{
    for (uint16_t i = 0; i < lspi.virtual_pixel_count; i++) {
        SetStripPixel(i, c, false);
    }
}

void LEDString::Update()
{
    uint32_t now = millis();
    lspi.now = now;
    lspi.time_since_start = now - lspi.mode_start_time;
    lspi.reverse = Inverted;
    DisplayMode::display_modes[lspi.mode_index].display_mode(&lspi);
    MaterialiseOversampling(0 /*SHIFT*/, OVERSAMPLING);
    for (uint8_t strip_index = 0; strip_index < STRIPS; strip_index++) {
        strips[strip_index]->Show();
    }

    lspi.pixel_offset += 3;
    lspi.pixel_offset %= ((LED_COUNT_MAX * OVERSAMPLING) << 4);
};

void LEDString::SetBrightness(uint8_t brightness)
{
    lspi.hsl.L = brightness;
    lspi.hsl.L /= 100;
    lspi.hsl.L *= 0.5;
    lspi.rgb = lspi.hsl;
}

void LEDString::SetColorHSI(float h, float s, float l)
{
    lspi.hsl.L = l;
    lspi.hsl.L /= 100;
    lspi.hsl.L *= 0.5;
    lspi.rgb = lspi.hsl;
}

void LEDString::SetColorRGB(u_int8_t r, u_int8_t g, u_int8_t b)
{
    lspi.rgb.R = r;
    lspi.rgb.G = g;
    lspi.rgb.B = b;

    float l = lspi.hsl.L;
    lspi.hsl = lspi.rgb;
    lspi.hsl.L = l;

    lspi.rgb = lspi.hsl;
}

void LEDString::SetColorRGB(uint32_t rgb)
{
    SetColorRGB(rgb >> 16, rgb >> 8, rgb);
}

void LEDString::SetSpeed(uint8_t speed)
{
    Speed = speed;
}

void LEDString::SetInverted(uint8_t inverted)
{
    Inverted = inverted;
}

void LEDString::StartModeTransition()
{
    for (int16_t i = 0; i < Pixels; i++) {
        R[i] = 0;
        G[i] = 0;
        B[i] = 0;
    }
}

void LEDString::SetMode(uint8_t mode)
{
    if (mode != lspi.mode_index) {
        Serial.printf("changing to mode [%d]\n", mode);
        lspi.previous_mode_index = lspi.mode_index;
        lspi.mode_index = mode;
        start_mode_time(millis(), &lspi);
        StartModeTransition();
    }
}

void LEDString::SetMode360(float hue)
{
    Serial.printf("changing to mode via HUE [%.0f]\n", hue);
    hue /= 360;
    hue *= DisplayMode::DISPLAY_MODES;
    SetMode((uint8_t)hue);
}

void LEDString::SetTransitionModesWithFading(uint8_t fading_on)
{
    FadingOn = fading_on;
}

// --------------------------------------------------------------------------------------
// Turn the oversampling buffer into a set of pixels that can be output
// --------------------------------------------------------------------------------------
void LEDString::MaterialiseOversampling(const uint8_t shift, const uint8_t oversampling)
{
    uint8_t strip_index = 0;
    uint16_t pixel_index = 0;
    for (uint16_t pixel = 0, sample = 0; pixel < Pixels / OVERSAMPLING; pixel++) {
        int32_t pr = R[sample] >> shift, pg = G[sample] >> shift, pb = B[sample] >> shift;
        sample++;
        for (uint8_t os = 1; os < oversampling; os++) {
            pr += R[sample] >> shift;
            pg += G[sample] >> shift;
            pb += B[sample] >> shift;
            sample++;
        }
        pr /= oversampling;
        pg /= oversampling;
        pb /= oversampling;
        if (pr > 255)
            pr = 255;
        if (pg > 255)
            pg = 255;
        if (pb > 255)
            pb = 255;
        // T_COLOR_TYPE c(sqrt_lookup[pr], sqrt_lookup[pg], sqrt_lookup[pb]);
        NeoColor c(pr, pg, pb);
        if (strip_index >= STRIPS) {
            Serial.printf("!!!");
            break;
        }
        strips[strip_index]->SetStripPixel(pixel_index++, c, false);
        if (pixel_index == pixel_info[strip_index].usable_pixel_count) {
            pixel_index = 0;
            strip_index++;
        }
    }
}