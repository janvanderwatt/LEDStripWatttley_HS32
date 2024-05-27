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
            // Serial.printf("%d/%d,", strip_pixel_lookup[strip_pixel_index], strip_pixel_lookup[strip_pixel_index + usable_pixel_count]);
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

void LEDString::set_next_mode_time(LEDStripPixelInfo_t* lspi)
{
    lspi->next_mode_change_time = 1; // transition time - make it a global!
    lspi->next_mode_change_time *= 1000;
    lspi->next_mode_change_time += lspi->now;
}

void LEDString::start_mode_time(uint32_t now, LEDStripPixelInfo_t* lspi_to_run)
{
    running_lspi = lspi_to_run;
    running_lspi->now = now;
    running_lspi->mode_start_time = now;
    running_lspi->time_since_start = 0;
    running_lspi->pixel_offset = 0;
    set_next_mode_time(running_lspi);
    // clear the mode storage
    memset(running_lspi->storage, 0, MODE_STORAGE);
    // call the mode with the RUN flag clear, to allow it to set up internal structures
    running_lspi->run = false;
    Serial.printf("INIT mode [%d]\n", running_lspi->mode_index);
    DisplayMode::display_modes[running_lspi->mode_index].display_mode(running_lspi);
    // when the mode is next called, the RUN flag is set
    running_lspi->run = true;
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

    currentIndex = 0;
    previousIndex = 1;

    Inverted = 0;
    Speed = WS2812FX_DEFAULT_SPEED;
    Brightness = WS2812FX_DEFAULT_BRIGHTNESS;
    SetColorRGB(WS2812FX_DEFAULT_COLOUR);
    Serial.printf("--> DEFAULT: rgb(%d,%d,%d)", RGB.R, RGB.G, RGB.B);
    ModeIndex = DisplayMode::WS2812FX_DEFAULT_MODE;

    fadeTimeMs = 0;
    FadingOn = 0;

    for (uint8_t i = 0; i < 2; i++) {
        lspi[i].string = this;
        lspi[i].reverse = false;
        lspi[i].oversampling = OVERSAMPLING;
        lspi[i].oversampling_pwr2 = OVERSAMPLING_PWR2;
        lspi[i].mode_index = (i == previousIndex) ? DisplayMode::DISPLAY_MODE_OFF : ModeIndex;
        lspi[i].rgb = RGB;
        lspi[i].hsl = HSL;
        lspi[i].pixel_offset = 0;
        lspi[i].speed = Speed;

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

#define LOG1(format, ...) Serial.print##__VA_OPT__(f)(format __VA_OPT__(, ) __VA_ARGS__);

    LOG1("============================= LED ===============================\n");
    LOG1("DEFAULTS: COLOUR: 0x%06X RGB(%d,%d,%d), BRIGHTNESS: %d, MODE: %d, SPEED: %d\n",
        WS2812FX_DEFAULT_COLOUR,
        (WS2812FX_DEFAULT_COLOUR >> 16) & 0xFF,
        (WS2812FX_DEFAULT_COLOUR >> 8) & 0xFF,
        (WS2812FX_DEFAULT_COLOUR >> 0) & 0xFF,
        WS2812FX_DEFAULT_BRIGHTNESS,
        DisplayMode::WS2812FX_DEFAULT_MODE,
        WS2812FX_DEFAULT_SPEED);
    // PRINT1("IO PIN: %d\n", WS2812FX_IO_PIN);
    LOG1("STRING PIXELS: %d -->\n", VirtualPixels);
    for (int strip_index = 0; strip_index < Segments; strip_index++) {
        LOG1("STRIP PIXELS: %d --> ", StripRealPixels(strip_index));
        for (uint8_t i = 0; i < StripSegments(strip_index); i++) {
            LOG1("\t[%d]=%d,%d ", i, StripSegmentOffset(strip_index, i), StripSegmentPixelCount(strip_index, i));
        }
        LOG1("\tTYPE: ");
        switch (StripPixelType(strip_index)) {
        case PIXEL_RGB:
            LOG1("\tRGB");
            break;
        case PIXEL_RGBW:
            LOG1("\tRGBW");
            break;
        case PIXEL_GRB:
            LOG1("\tGRB");
            break;
        case PIXEL_GRBW:
            LOG1("\tGRBW");
            break;
        default:
            LOG1("\tunknown(%d)", StripPixelType(strip_index));
            break;
        }
        LOG1("\n");
    }
    LOG1("============================= LED ===============================\n");
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
            running_lspi->R[strip_pixel_index] >> SHIFT,
            running_lspi->G[strip_pixel_index] >> SHIFT,
            running_lspi->B[strip_pixel_index] >> SHIFT));
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
    running_lspi->R[strip_pixel_index] = pixel_colour.R << SHIFT;
    running_lspi->G[strip_pixel_index] = pixel_colour.G << SHIFT;
    running_lspi->B[strip_pixel_index] = pixel_colour.B << SHIFT;
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
    // Serial.printf("Clearing to rgb(%d,%d,%d)\n", running_lspi->rgb.R, running_lspi->rgb.G, running_lspi->rgb.B);
    for (uint16_t i = 0; i < VirtualPixels; i++) {
        SetStripPixel(i, c, false);
    }
}

// --------------------------------------------------------------------------------------
// Set the brightness of the strip (applied when materialising only)
// --------------------------------------------------------------------------------------
void LEDString::SetBrightness(uint8_t brightness)
{
    Brightness = brightness;
}

// --------------------------------------------------------------------------------------
// Set the strip colour based on an HSI colour
// --------------------------------------------------------------------------------------
void LEDString::SetColorHSI(float h, float s, float l)
{
    Serial.printf("SetColorHSL(%.0f,%.0f,ignored(%.0f))\n", h, s, l);
    h /= 360.0f;
    s /= 100.0f;
    HSL = HslColor(h, s, 0.5f);
    RGB = HSL;
    Serial.printf(" --> RGB(%d,%d,%d)\n", RGB.R, RGB.G, RGB.B);
}

// --------------------------------------------------------------------------------------
// Set the strip colour based on an RGB colour (r, g, b)
// --------------------------------------------------------------------------------------
void LEDString::SetColorRGB(u_int8_t r, u_int8_t g, u_int8_t b)
{
    Serial.printf("SetColorRGB(%d,%d,%d)\n", r, g, b);
    RGB = RgbColor(r, g, b);
    HSL = RGB;
}

// --------------------------------------------------------------------------------------
// Set the strip colour based on an RGB colour INT (0xrrggbb)
// --------------------------------------------------------------------------------------
void LEDString::SetColorRGB(uint32_t rgb)
{
    SetColorRGB((rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF);
}

// --------------------------------------------------------------------------------------
// Set the speed of effects
// --------------------------------------------------------------------------------------
void LEDString::SetSpeed(uint8_t speed)
{
    Serial.printf("speed:%d", speed);
    Speed = speed;
    running_lspi->speed = speed;
}

// --------------------------------------------------------------------------------------
// Set the direction of effects
// --------------------------------------------------------------------------------------
void LEDString::SetInverted(uint8_t inverted)
{
    Serial.printf("inverted:%d", inverted);
    Inverted = inverted;
    running_lspi->reverse = inverted;
}

// --------------------------------------------------------------------------------------
// Start a mode transition
// --------------------------------------------------------------------------------------
void LEDString::StartModeTransition()
{
    if (FadingOn)
        fadeTimeMs = WS2812FX_FADE_TIME_MS;
}

// --------------------------------------------------------------------------------------
// Get the current mode as an INT
// --------------------------------------------------------------------------------------
uint8_t LEDString::GetMode()
{
    return ModeIndex;
}

// --------------------------------------------------------------------------------------
// Get the current mode as HUE
// --------------------------------------------------------------------------------------
float LEDString::GetMode360()
{
    float result = ModeIndex;
    return ModeIndex;
}

// --------------------------------------------------------------------------------------
// Set the current mode
// --------------------------------------------------------------------------------------
void LEDString::SetMode(uint8_t mode)
{
    if (mode != ModeIndex) {
        ModeIndex = mode;
        // Make a copy of the current mode settings (mostly after the colour)
        previousIndex ^= 1;
        currentIndex ^= 1;

        LEDStripPixelInfo_t *current_lspi = &lspi[currentIndex], *previous_lspi = &lspi[previousIndex];
        running_lspi = current_lspi;
        current_lspi->mode_index = mode;
        start_mode_time(millis(), current_lspi);
        StartModeTransition();
        Serial.printf("changed mode from [%d] to [%d]\n", previous_lspi->mode_index, current_lspi->mode_index);
        Serial.printf("previous_mode: RGB(%d,%d,%d)\n", previous_lspi->rgb.R, previous_lspi->rgb.G, previous_lspi->rgb.B);
    } else {
        Serial.printf("already in mode - no change required\n");
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

void LEDString::RunMode(uint32_t now, LEDStripPixelInfo_t* lspi_to_run)
{
    running_lspi = lspi_to_run;

    running_lspi->now = now;
    running_lspi->time_since_start = now - running_lspi->mode_start_time;
    uint32_t mod = ((uint32_t)VirtualPixels) << 8;
    running_lspi->pixel_fraction = running_lspi->pixel_offset;
    running_lspi->pixel_fraction /= mod;
    DisplayMode::display_modes[running_lspi->mode_index].display_mode(running_lspi);
    if (running_lspi->reverse) {
        running_lspi->pixel_offset -= running_lspi->speed;
        if (running_lspi->pixel_offset > mod) {
            running_lspi->pixel_offset += mod;
        }
    } else {
        running_lspi->pixel_offset += running_lspi->speed;
        if (running_lspi->pixel_offset > mod) {
            running_lspi->pixel_offset -= mod;
        }
    }
}

// --------------------------------------------------------------------------------------
// Turn the oversampling buffer into a set of pixels that can be output
// --------------------------------------------------------------------------------------
void LEDString::MaterialisePixelData(uint8_t time_delay_ms)
{
    LEDStripPixelInfo_t *current_lspi = &lspi[currentIndex], *previous_lspi = &lspi[previousIndex];
    uint8_t fading = FadingOn && fadeTimeMs > 0;
    if (current_lspi->rgb != RGB) {
        Serial.printf("going from old [%d]rgb(%d,%d,%d) to new rgb(%d,%d,%d)\n", currentIndex, current_lspi->rgb.R, current_lspi->rgb.G, current_lspi->rgb.B, RGB.R, RGB.G, RGB.B);
        current_lspi->rgb = RGB;
        current_lspi->hsl = HSL;
    }

    uint32_t now = millis();
    // Serial.printf("running mode [%d]:%d\n", current_lspi->mode_index, current_lspi->run);
    uint8_t kc = 128, kp = 0;
    if (fading) {
        // Serial.printf("running previous mode [%d]:%d\n", previous_lspi->mode_index, previous_lspi->run);
        RunMode(now, previous_lspi);

        float kc32 = fadeTimeMs;
        kc32 /= WS2812FX_FADE_TIME_MS;
        uint8_t not_halfway = kc32 <= 0.5f;
        kc32 = (not_halfway ? 0.5f - kc32 : kc32 - 0.5f) * 2.0f;

        kc32 = sqrt(kc32) * 0.5f;
        kc32 = not_halfway ? 0.5f - kc32 : 0.5f + kc32;
        kp = kc * kc32;
        kc -= kp;
        if (kc == 0) {
            Serial.printf("cmode:%d|%d rgb(%d,%d,%d), pmode:%d|%d rgb(%d,%d,%d)| ",
                currentIndex, current_lspi->mode_index, current_lspi->rgb.R, current_lspi->rgb.G, current_lspi->rgb.B,
                previousIndex, previous_lspi->mode_index, previous_lspi->rgb.R, previous_lspi->rgb.G, previous_lspi->rgb.B);
        }
        // Serial.printf("kc:%d, kp:%d | ", kc, kp);
        fadeTimeMs -= time_delay_ms;
    }

    RunMode(now, current_lspi);

    uint8_t strip_index = 0;
    uint16_t pixel_index = 0;
    for (uint16_t pixel = 0, sample = 0; pixel < LEDString::VirtualPixels / OVERSAMPLING; pixel++) {
        int32_t pr = 0, pg = 0, pb = 0;
        // Serial.printf("\npx:%d -> ", pixel);
        for (uint8_t os = 0; os < OVERSAMPLING; os++) {
            // Serial.printf("rgb(%d,%d,%d) : ", current_lspi->R[sample], current_lspi->G[sample], current_lspi->B[sample]);
            pr += kc * current_lspi->R[sample];
            pg += kc * current_lspi->G[sample];
            pb += kc * current_lspi->B[sample];
            if (fading) {
                // Serial.printf("prev_rgb(%d,%d,%d) : ", previous_lspi->R[sample], previous_lspi->G[sample], previous_lspi->B[sample]);
                pr += kp * previous_lspi->R[sample];
                pg += kp * previous_lspi->G[sample];
                pb += kp * previous_lspi->B[sample];
            }
            sample++;
        }
        uint8_t shift = SHIFT + 7 + OVERSAMPLING_PWR2;
        // Serial.printf("shift(%d) : ", shift);
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
        // Serial.printf("led_rgb(%d,%d,%d) : ", pr, pb, pg);
        NeoColor c(pr, pg, pb);
        // NeoColor c(pixel_index, pixel_index / 2, pixel_index / 4);
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
    for (strip_index = 0; strip_index < STRIPS; strip_index++) {
        strips[strip_index]->Show();
    }
}