#include <Arduino.h>

#include "LEDStrip.h"
#include "DisplayModes.h"

#include "Configuration.h"
#include "LedConfigurations.h"

extern uint8_t sqrt_lookup[256];

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
    lspi->pixel_offset = 0;
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
    VirtualPixels = 0;
    for (uint8_t strip_index = 0; strip_index < STRIPS; strip_index++) {
        Segments += pixel_info[strip_index].segments;
        VirtualPixels += pixel_info[strip_index].usable_pixel_count;
    }
    VirtualPixels *= OVERSAMPLING;

    fadeTimeMs = WS2812FX_FADE_TIME_MS;
    FadingOn = 0;

    Inverted = 0;
    Speed = WS2812FX_DEFAULT_SPEED;
    Brightness = WS2812FX_DEFAULT_BRIGHTNESS;
    SetColorRGB(WS2812FX_DEFAULT_COLOUR);
    ModeIndex = DisplayMode::WS2812FX_DEFAULT_MODE;

    currentIndex = 0;
    previousIndex = 1;

    current_lspi = &lspi[currentIndex];
    previous_lspi = &lspi[previousIndex];

    for (uint8_t i = 0; i < 2; i++) {
        lspi[i].string = this;
        lspi[i].reverse = false;
        lspi[i].oversampling = OVERSAMPLING;
        lspi[i].mode_index = (i == previousIndex) ? DisplayMode::DISPLAY_MODE_STATIC : ModeIndex;
        lspi[i].rgb = (i == previousIndex) ? RgbColor(0) : RGB;
        lspi[i].hsl = lspi[i].rgb;
        lspi[i].oversampling = OVERSAMPLING;
        lspi[i].pixel_offset = 0;

        lspi[i].R = new int32_t[VirtualPixels];
        lspi[i].G = new int32_t[VirtualPixels];
        lspi[i].B = new int32_t[VirtualPixels];

        start_mode_time(millis(), &lspi[i]);
    }

    NeoPixelBus<NeoFeature, NeoMethod>* bus_ptr;
    LEDStrip<NeoColor, NeoFeature, NeoMethod>* strip_ptr;
    for (uint8_t strip_index = 0; strip_index < STRIPS; strip_index++) {
        bus_ptr = new NeoPixelBus<NeoFeature, NeoMethod>(pixel_info[strip_index].total_pixel_count, pixel_info[strip_index].pixel_pin);
        strip_ptr = new LEDStrip<NeoColor, NeoFeature, NeoMethod>(bus_ptr, &pixel_info[strip_index], OVERSAMPLING);
        strips[strip_index] = strip_ptr;
    }
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
    if (strip_pixel_index >= VirtualPixels) {
        strip_pixel_index += VirtualPixels;
        strip_pixel_index %= VirtualPixels;
    }
    return (
        RgbColor(
            current_lspi->R[strip_pixel_index] >> SHIFT,
            current_lspi->G[strip_pixel_index] >> SHIFT,
            current_lspi->B[strip_pixel_index] >> SHIFT));
}

// --------------------------------------------------------------------------------------
// Set the colour of a pixel in the STRING
// --------------------------------------------------------------------------------------
void LEDString::SetStripPixel(uint16_t strip_pixel_index, RgbColor pixel_colour, bool use_direction)
{
    // if the pixel index is negative, it will wrap in the UINT space, and exceed the usable count.
    // before taking the MOD, add back the usable count, so that the number is smaller than the usable count in UINT space.
    if (strip_pixel_index >= VirtualPixels) {
        strip_pixel_index += VirtualPixels;
        strip_pixel_index %= VirtualPixels;
    }
    current_lspi->R[strip_pixel_index] = pixel_colour.R << SHIFT;
    current_lspi->G[strip_pixel_index] = pixel_colour.G << SHIFT;
    current_lspi->B[strip_pixel_index] = pixel_colour.B << SHIFT;
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
    for (uint16_t i = 0; i < VirtualPixels; i++) {
        SetStripPixel(i, c, false);
    }
}

void LEDString::SetBrightness(uint8_t brightness)
{
    Brightness = brightness;
}

void LEDString::SetColorHSI(float h, float s, float l)
{
    HSL = HslColor(h, s, 0.5f);
    RGB = HSL;
}

void LEDString::SetColorRGB(u_int8_t r, u_int8_t g, u_int8_t b)
{
    RGB = RgbColor(r, g, b);
    HSL = RGB;
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
    fadeTimeMs = WS2812FX_FADE_TIME_MS;
}

// --------------------------------------------------------------------------------------
// Get the current active mode
// --------------------------------------------------------------------------------------
uint8_t LEDString::GetMode()
{
    return ModeIndex;
}

// --------------------------------------------------------------------------------------
// Get the current active mode
// --------------------------------------------------------------------------------------
float LEDString::GetMode360()
{
    float result = ModeIndex;
    return ModeIndex;
}

void LEDString::SetMode(uint8_t mode)
{
    if (mode != ModeIndex) {
        Serial.printf("changing to mode [%d]\n", mode);
        // Make a copy of the current mode settings (mostly after the colour)
        previousIndex ^= 1;
        currentIndex ^= 1;
        current_lspi = &lspi[currentIndex];
        previous_lspi = &lspi[previousIndex];
        start_mode_time(millis(), current_lspi);
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
void LEDString::MaterialisePixelData(uint8_t time_delay_ms)
{
    uint8_t fading = FadingOn && fadeTimeMs > 0;
    current_lspi->rgb = RGB;
    current_lspi->hsl = HSL;

    uint32_t now = millis();
    current_lspi->now = now;
    current_lspi->time_since_start = now - current_lspi->mode_start_time;
    DisplayMode::display_modes[current_lspi->mode_index].display_mode(current_lspi);
    current_lspi->pixel_offset += Speed >> 2;
    current_lspi->pixel_offset %= VirtualPixels;

    uint8_t kc = 128, kp = 0;

    if (fading) {
        previous_lspi->now = now;
        previous_lspi->time_since_start = now - previous_lspi->mode_start_time;
        DisplayMode::display_modes[previous_lspi->mode_index].display_mode(previous_lspi);
        previous_lspi->pixel_offset += Speed >> 2;
        previous_lspi->pixel_offset %= VirtualPixels;
        uint32_t kc32 = kc;
        kc32 *= fadeTimeMs;
        kc32 /= WS2812FX_FADE_TIME_MS;
        kp = kc32;
        kc -= kp;
        Serial.printf("kc:%d, kp:%d | ", kc, kp);
        fadeTimeMs -= time_delay_ms;
    }

    uint8_t strip_index = 0;
    uint16_t pixel_index = 0;
    for (uint16_t pixel = 0, sample = 0; pixel < LEDString::VirtualPixels / OVERSAMPLING; pixel++) {
        int32_t pr = 0, pg = 0, pb = 0;
        for (uint8_t os = 0; os < OVERSAMPLING; os++) {
            pr += kc * current_lspi->R[sample];
            pg += kc * current_lspi->G[sample];
            pb += kc * current_lspi->B[sample];
            if (fading) {
                pr += kp * previous_lspi->R[sample];
                pg += kp * previous_lspi->G[sample];
                pb += kp * previous_lspi->B[sample];
            }
            sample++;
        }
        uint8_t shift = SHIFT + 7 + OVERSAMPLING_PWR2 + (FadingOn ? 1 : 0);
        pr >>= shift;
        pg >>= shift;
        pb >>= shift;
        if (pr > 255)
            pr = 255;
        if (pg > 255)
            pg = 255;
        if (pb > 255)
            pb = 255;
        pr *= Brightness;
        pr >>= 8;
        pg *= Brightness;
        pg >>= 8;
        pb *= Brightness;
        pb >>= 8;
        // T_COLOR_TYPE c(sqrt_lookup[pr], sqrt_lookup[pg], sqrt_lookup[pb]);
        NeoColor c(pr, pg, pb);
        // NeoColor c(pixel_index, pixel_index / 2, pixel_index / 4);
        if (strip_index >= STRIPS) {
            Serial.printf("!!!");
            break;
        }
        strips[strip_index]->SetStripPixel(pixel_index++, c, false);
        if (pixel_index == pixel_info[strip_index].usable_pixel_count) {
            strips[strip_index]->Show();
            pixel_index = 0;
            strip_index++;
        }
    }
}