#include <Arduino.h>
/*********************************************************************************
 *  MIT License
 *
 *  Copyright (c) 2020 Gregg E. Berman
 *
 *  https://github.com/HomeSpan/HomeSpan
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 *
 ********************************************************************************/

#include "Configuration.h"
#include "LEDStrip.h"
#include "DisplayModes.h"

////////////////////////////////////////////////////////////
//                                                        //
//    HomeSpan: A HomeKit implementation for the ESP32    //
//    ------------------------------------------------    //
//                                                        //
////////////////////////////////////////////////////////////
#include "HomeSpan.h"
#include "DEV_Identify.h" // This is where we store all code for the DEV_Identify Service
#include "DEV_LED.h" // This is where we store all code for the DEV_LED types

#include <Update.h>
#include <esp32FOTA.h>

#include "wifi_configurations.h"
#include "qrcode_configurations.h"

#define PRINT1(x, ...)                                          \
    if (homeSpan.getLogLevel() > 0) {                           \
        char buffer[256];                                       \
        snprintf(buffer, sizeof(buffer) - 1, x, ##__VA_ARGS__); \
        Serial.print(buffer);                                   \
    }
#define PRINT2(x, ...)                                          \
    if (homeSpan.getLogLevel() > 1) {                           \
        char buffer[256];                                       \
        snprintf(buffer, sizeof(buffer) - 1, x, ##__VA_ARGS__); \
        Serial.print(buffer);                                   \
    }

#define OTA_FIRMWARE_TYPE "32-HS-LED"
#define OTA_FIRMWARE_VERSION static_cast<uint16_t>(100 * (static_cast<float>(SOFTWARE_VERSION)))
#define OTA_CHECK_URL "http://linode.webhop.org/ota/fota-esp32/otacheck.php"
#define OTA_FIRST_CHECK_SECONDS 60
#define OTA_CHECK_INTERVAL_HOURS 23

#define WS2812FX_DIRECTION_LEFT 0
#define WS2812FX_DIRECTION_RIGHT 1
#define WS2812FX_DIRECTION_NOT_INVERTED 0
#define WS2812FX_DIRECTION_INVERTED 1
#define WS2812FX_DIRECTION_RANDOM 2
#define WS2812FX_DIRECTION_KEEP 3

// #define HOMEKIT_MENG
#define HOMEKIT_4THEWF

LEDString* led_string;

void LED_on_HomeKit_change();
void FX_on_HomeKit_change();

LED_changes_t LED = { .power = 0, .H = 60, .S = 100, .V = 80, .on_HomeKit_change = LED_on_HomeKit_change };
LED_changes_t FX = { .power = 0, .H = 0, .S = 90, .V = 50, .on_HomeKit_change = FX_on_HomeKit_change };

void MODE_on_HomeKit_change(int);
const char* names[] = {
    "Static",
    "Rainbow",
    "Comet",
    "Pulsar"
};

modes_t modes[] = {
    { .id = 0, .mode_state = 0, .FX_power = 0, .FX_mode = -1, .FX_speed = 0, .FX_direction = WS2812FX_DIRECTION_KEEP }, // static
    { .id = 1, .mode_state = 0, .FX_power = 1, .FX_mode = DisplayMode::DISPLAY_MODE_RAINBOW_CYCLE, .FX_speed = 128, .FX_direction = WS2812FX_DIRECTION_RANDOM }, // rainbow cycle
    { .id = 2, .mode_state = 0, .FX_power = 1, .FX_mode = DisplayMode::DISPLAY_MODE_COMET, .FX_speed = 32, .FX_direction = WS2812FX_DIRECTION_RANDOM }, // comet
    { .id = 3, .mode_state = 0, .FX_power = 1, .FX_mode = DisplayMode::DISPLAY_MODE_FIREWORKS_RANDOM, .FX_speed = 128, .FX_direction = WS2812FX_DIRECTION_KEEP } // pulsar
}; // pulsar

int mode_switches = 0;
char dev_guid_prefix[] = "32HS";
char dev_guid_mac[9] = "aabbccdd";
char manufacturer[] = "JvdW";
char ap_ssid[24];
char host_name[24];
char* model;
char* version;

esp32FOTA esp32FOTAtool(OTA_FIRMWARE_TYPE, OTA_FIRMWARE_VERSION, false);

#define MAKE_NEXT_DEV_GUID                                                                            \
    this_dev_guid = new char[dev_guid_len];                                                           \
    snprintf(this_dev_guid, dev_guid_len, "%s-%s-%02d", dev_guid_prefix, dev_guid_mac, accessory_id); \
    accessory_id++

uint64_t next_ota_check_time = OTA_FIRST_CHECK_SECONDS * 1000000ULL;

//////////////////////////////////////

void LED_on_HomeKit_change()
{
    LOG1("Processing LED changes\n");
    led_string->SetColorHSI(LED.H, LED.S, 100);
    if (LED.power) {
        led_string->SetBrightness((uint8_t)(LED.V * 2.55));
        // if the FX was switched ON when the LED was last switched OFF, switch it ON again now
        if (FX.power == 128) {
            FX.power = 1;
        }
    } else {
        led_string->SetBrightness(0);
        // if the FX is currently switched ON, switch if OFF and flag to switch it ON again when the LED is turned ON again
        if (FX.power != 0) {
            FX.power = 128;
        }
    }
    FX_on_HomeKit_change();
}

//////////////////////////////////////

void FX_on_HomeKit_change()
{
    LOG1("Processing FX changes\n");
    uint8_t fx_speed;
    uint8_t fx_direction;
    if (FX.V > 50) {
        fx_direction = 0;
        fx_speed = FX.V - 50;
    } else {
        fx_direction = 1;
        fx_speed = 50 - FX.V;
    }
    fx_speed *= 5.1;
    led_string->SetInverted(fx_direction);
    led_string->SetSpeed(fx_speed);
    if (LED.power) {
        // only use bit 0 to determine whether to switch on the FX
        if (FX.power & 1) {
            led_string->SetMode360(FX.H);
        } else {
            led_string->SetMode(DisplayMode::DISPLAY_MODE_STATIC);
        }
    }
    uint8_t fx_mode = led_string->GetMode360();
    LOG2("checking if there is a relevant mode to switch ON, switching off all non-relevant modes\n");
    LOG2("--> target: FX.power:");
    LOG2(FX.power);
    LOG2(", FX.H:");
    LOG2(FX.H);
    LOG2(", fx_mode:");
    LOG2(fx_mode);
    LOG2(", fx_speed:");
    LOG2(fx_speed);
    LOG2(", fx_direction:");
    LOG2(fx_direction);
    LOG2("\n");
    for (uint8_t mode = 0; mode < mode_switches; mode++) {
        uint8_t desired_state = 0;
        // modes can only be ON if the LED is also ON
        if (LED.power > 0) {
            //     {.id = 1, .mode_state = 0, .FX_power = 1, .FX_mode = 60, .FX_speed = 128, .FX_direction = 2},   // rainbow cycle
            // map the HUE to the mode count
            uint8_t mode_mode = ((uint16_t)DisplayMode::DISPLAY_MODES) * ((uint16_t)modes[mode].FX_mode) / 360;

            LOG2("  --? compare: [");
            LOG2(names[mode]);
            LOG2("], [mode].FX_power:");
            LOG2(modes[mode].FX_power);
            LOG2(", [mode].FX_mode:");
            LOG2(modes[mode].FX_mode);
            LOG2(", mode_mode:");
            LOG2(mode_mode);
            LOG2(", [mode].FX_speed:");
            LOG2(modes[mode].FX_speed);
            LOG2(", [mode].FX_direction:");
            LOG2(modes[mode].FX_direction);
            LOG2("\n");

            // check for matching FX enable settings (using bit 0 only)
            if (modes[mode].FX_power == (FX.power & 1)) {
                // check the INT versions of the mode
                if (modes[mode].FX_mode < 0 || fx_mode == mode_mode) {
                    // we have matching mode, check the speed
                    // if no speed setting is required inthe mode, or the speed is within range after mapping from % back to BYTE
                    if (modes[mode].FX_speed < 0 || (abs(modes[mode].FX_speed - fx_speed) <= 6)) {
                        // if the speed is 0 (direction irrelevant), directions match, or the mode definition is set to irrelevant/random, then we have a complete match
                        if (fx_speed == 0 || (modes[mode].FX_direction == fx_direction) || modes[mode].FX_direction == WS2812FX_DIRECTION_KEEP || modes[mode].FX_direction == WS2812FX_DIRECTION_RANDOM) {
                            desired_state = 1;
                        } else {
                            LOG2("fx_speed == 0 || (modes[mode].FX_direction == fx_direction) || modes[mode].FX_direction == %d || modes[mode].FX_direction == %d\n", WS2812FX_DIRECTION_KEEP, WS2812FX_DIRECTION_RANDOM);
                        }
                    } else {
                        LOG2("(failed test)modes[mode].FX_speed < 0 || (abs(modes[mode].FX_speed - fx_speed) <= 6\n");
                    }
                } else {
                    LOG2("(failed test) modes[mode].FX_mode < 0 || fx_mode == mode_mode\n");
                }
            } else {
                LOG2("(failed test) modes[mode].FX_power == (FX.power & 1)\n");
            }
        }
        // only change the state, and notify HomeKit, if the  mode isn't already in that state
        if (modes[mode].mode_state != desired_state) {
            LOG2("switching ");
            LOG2(desired_state == 0 ? "[off]" : "[on]");
            LOG2(" MODE [");
            LOG2(names[mode]);
            LOG2("]\n");
            modes[mode].mode_switch->power->setVal(desired_state);
            // Align the internal state with the target state
            modes[mode].mode_state = desired_state;
        }
    }
}

//////////////////////////////////////

void MODE_on_HomeKit_change(int changed_mode)
{
    // A mode has changed state
    LOG1("switching off ALL other modes except MODE [%d] ", changed_mode);
    LOG1(names[changed_mode]);
    LOG1("\n");
    for (uint8_t other_mode = 0; other_mode < mode_switches; other_mode++) {
        // Switch OFF all the >>other<< modes
        if (other_mode != changed_mode) {
            // only switch off, and notify HomeKit, if the other mode isn't already off
            if (modes[other_mode].mode_state != 0) {
                LOG1("switching off MODE [");
                LOG1(names[other_mode]);
                LOG1("]\n");
                modes[other_mode].mode_switch->power->setVal(0);
                // Align the internal state with the target state
                modes[other_mode].mode_state = 0;
            }
        }
    }
    if (modes[changed_mode].mode_state != 0) {
        // If the mode has switched ON, then set up the LED according to the desired mode
        LED.power = 1;
        if (modes[changed_mode].FX_power >= 0) {
            // this also overrides any memory of whether the FX were switched ON when the LED was last switched OFF
            FX.power = modes[changed_mode].FX_power;
            // If the FX is desired to be ON, then check the other parameters
            if (modes[changed_mode].FX_power == 1) {
                if (modes[changed_mode].FX_mode >= 0) {
                    FX.H = modes[changed_mode].FX_mode;
                    FX.H /= DisplayMode::DISPLAY_MODES;
                    FX.H *= 360;
                }
                if (modes[changed_mode].FX_speed >= 0) {
                    int8_t direction = modes[changed_mode].FX_direction;
                    // if a direction was specified, then set it
                    if (direction != WS2812FX_DIRECTION_KEEP) {
                        // if a random direction was specified, randomly set to 0 or 1
                        if (direction == WS2812FX_DIRECTION_RANDOM)
                            direction = random(2);
                        FX.V = 50.499f + (direction == 0 ? 1 : -1) * ((float)modes[changed_mode].FX_speed / 5.1f);
                        FX.V = floorf((FX.V < 0 ? 0 : (FX.V > 100) ? 100
                                                                   : FX.V));
                    }
                }
            }
        }
    } else {
        // If the mode has switched OFF, then switch OFF the LED and the FX
        // this also overrides any memory of whether the FX were switched ON when the LED was last switched OFF
        LED.power = 0;
        FX.power = 0;
    }
    LOG1("new settings: ");
    LOG1("LED_power=");
    LOG1(LED.power);
    LOG1(", FX_power=");
    LOG1(FX.power);
    LOG1(", FX_H=");
    LOG1(FX.H);
    LOG1(", FX_V=");
    LOG1(FX.V);
    LOG1("\n");
    // call the HomeKit change functions to process the changes mades to the LED and FX settings
    LED_on_HomeKit_change();
    // FX_on_HomeKit_change();
}

/*size_t last_progress;
uint8_t last_percentage;
void progress_updater(size_t progress, size_t size)
{
    if (last_progress == 0) {
        last_progress = 1;
        led_string->SetTransitionModesWithFading(0);
        led_string->SetMode360(0); // static
    }
    if (progress > last_progress) {
        last_progress = progress;
        size_t led_progress = progress;
        // led_progress *= WS2812FX_getLength();
        led_progress /= size;
        // WS2812FX_set_static_progress(led_progress);

        progress *= 100;
        progress /= size;
        if (progress != last_percentage) {
            last_percentage = progress;
            PRINT1("%d%%..", progress);
        }
    }
}*/

//////////////////////////////////////

void setup()
{
    Serial.begin(115200);
    delay(10);

    int dev_guid_len = strlen(dev_guid_prefix) + strlen(dev_guid_mac) + 6;
    int accessory_id = 0;
    char* this_dev_guid;
    String model_temp = "";

    led_string = new LEDString();
    led_string->SetTransitionModesWithFading(true);

    homeSpan.setLogLevel(1);
#if defined(WIFI_SSID) && defined(WIFI_PASSWORD)
    homeSpan.setWifiCredentials(WIFI_SSID, WIFI_PASSWORD);
#endif
    LOG1("============================================================\n");

    for (uint8_t strip_index = 0; strip_index < led_string->Strips(); strip_index++) {
        if (model_temp.length() > 0) {
            model_temp += "-";
        }
        switch (led_string->StripPixelType(strip_index)) {
        case PIXEL_RGB:
            model_temp += "RGB";
            break;
        case PIXEL_GRB:
            model_temp += "grB";
            break;
        case PIXEL_RGBW:
            model_temp += "RGBW";
            break;
        case PIXEL_GRBW:
            model_temp += "grBW";
            break;
        }
    }
    model_temp += "-LED";
    model = new char[model_temp.length() + 1];
    snprintf(model, model_temp.length() + 1, "%s", model_temp.c_str());

    // "$Revision: 1.5 $"
    // Note: HomeKit only allows versions in the format X.Y.Z, so we can't append the library header revision as well
    String revision_code = WS2812FXJVDW_C_REV;

    version = new char[revision_code.length() + 1];
    snprintf(version, revision_code.length() + 1, "%s", revision_code.c_str());
    PRINT1("VERSION: %s\n", version);

    String mac = WiFi.macAddress();
    LOG1("MAC ADDRESS: " + mac + "\n");
    snprintf(dev_guid_mac, sizeof(dev_guid_mac) - 1, "%c%c%c%c%c%c%c%c", mac[6], mac[7], mac[9], mac[10], mac[12], mac[13], mac[15], mac[16]);

    snprintf(ap_ssid, sizeof(ap_ssid) - 1, "HSLED-%c%c%c%c%c%c", mac[9], mac[10], mac[12], mac[13], mac[15], mac[16]);
    PRINT1("AP ID: %s\n", ap_ssid);

    snprintf(host_name, sizeof(host_name) - 1, "HSLED-%c%c%c%c%c%c", mac[9], mac[10], mac[12], mac[13], mac[15], mac[16]);
    PRINT1("WIFI HOST NAME: %s\n", host_name);
    WiFi.setHostname(host_name);

    uint16_t startup_delay = ((uint16_t)mac[15]) << 4 ^ mac[16];
    PRINT1("waiting : %d ms\n", startup_delay);
    delay(startup_delay); // wait a random time based on MAC address before starting

    LOG1("============================================================\n");

    // see here for details: https://github.com/HomeSpan/HomeSpan/blob/master/docs/Reference.md
    // homeSpan.enableOTA();
    // By not calling these functions, HomeSpan will not attempt to use those features
    // homeSpan.setControlPin(21); // default is 21
    homeSpan.setStatusPin(GPIO_NUM_2); // default is 13
    homeSpan.setApSSID(ap_ssid); // default is "HomeSpan-Setup"
    homeSpan.setApPassword("homespan"); // default is "homespan"
    homeSpan.setQRID(HOMEKIT_SETUP_QRCODE); // default is "HSPN"
    // homeSpan.setQRID(HOMEKIT_SETUP_QRCODE); // default is "HSPN"
    // homeSpan.processSerialCommand("S 07072071");

    // homeSpan.defaultSetupCode = "08801118"; // default is DEFAULT_SETUP_CODE = "46637726"
    //  must be called last
    homeSpan.enableAutoStartAP(); // enables automatic start-up of WiFi Access Point if WiFi Credentials are not found at boot time
    homeSpan.begin(Category::Bridges, "HomeSpan Bridge");

    new SpanAccessory();
    MAKE_NEXT_DEV_GUID;
    new DEV_Identify("Bridge #1", manufacturer, this_dev_guid, model, "0.1", 3);
    new Service::HAPProtocolInformation();
    new Characteristic::Version("1.1.0");

    new SpanAccessory();
    MAKE_NEXT_DEV_GUID;
    new DEV_Identify("RGB LED", manufacturer, this_dev_guid, model, version, 0);
    new DEV_RgbLED(&LED);

    new SpanAccessory();
    MAKE_NEXT_DEV_GUID;
    new DEV_Identify("RGB FX", manufacturer, this_dev_guid, model, version, 0);
    new DEV_RgbLED(&FX);

    new SpanAccessory();
    MAKE_NEXT_DEV_GUID;
    new DEV_Identify(names[mode_switches], manufacturer, this_dev_guid, model, version, 0);
    modes[mode_switches].mode_switch = new DEV_LED(&modes[mode_switches]);
    modes[mode_switches].on_HomeKit_change = MODE_on_HomeKit_change;
    mode_switches++;

    new SpanAccessory();
    MAKE_NEXT_DEV_GUID;
    new DEV_Identify(names[mode_switches], manufacturer, this_dev_guid, model, version, 0);
    modes[mode_switches].mode_switch = new DEV_LED(&modes[mode_switches]);
    modes[mode_switches].on_HomeKit_change = MODE_on_HomeKit_change;
    mode_switches++;

    new SpanAccessory();
    MAKE_NEXT_DEV_GUID;
    new DEV_Identify(names[mode_switches], manufacturer, this_dev_guid, model, version, 0);
    modes[mode_switches].mode_switch = new DEV_LED(&modes[mode_switches]);
    modes[mode_switches].on_HomeKit_change = MODE_on_HomeKit_change;
    mode_switches++;

    new SpanAccessory();
    MAKE_NEXT_DEV_GUID;
    new DEV_Identify(names[mode_switches], manufacturer, this_dev_guid, model, version, 0);
    modes[mode_switches].mode_switch = new DEV_LED(&modes[mode_switches]);
    modes[mode_switches].on_HomeKit_change = MODE_on_HomeKit_change;
    mode_switches++;

    homeSpan.poll();

    const uint8_t WS2812FX_DEFAULT_COLOUR = 0, WS2812FX_DEFAULT_BRIGHTNESS = 0, WS2812FX_DEFAULT_MODE = 0, WS2812FX_DEFAULT_SPEED = 0;

    LOG1("============================= LED ===============================\n");
    PRINT1("DEFAULTS: COLOUR: 0x%06X RGB(%d,%d,%d), BRIGHTNESS: %d, MODE: %d, SPEED: %d\n",
        WS2812FX_DEFAULT_COLOUR,
        (WS2812FX_DEFAULT_COLOUR >> 16) & 0xFF,
        (WS2812FX_DEFAULT_COLOUR >> 8) & 0xFF,
        (WS2812FX_DEFAULT_COLOUR >> 0) & 0xFF,
        WS2812FX_DEFAULT_BRIGHTNESS,
        WS2812FX_DEFAULT_MODE,
        WS2812FX_DEFAULT_SPEED);
    // PRINT1("IO PIN: %d\n", WS2812FX_IO_PIN);
    PRINT1("STRING PIXELS: %d -->\n", led_string->VirtualPixels);
    for (int strip_index = 0; strip_index < led_string->Segments; strip_index++) {
        PRINT1("STRIP PIXELS: %d --> ", led_string->StripRealPixels(strip_index));
        for (uint8_t i = 0; i < led_string->StripSegments(strip_index); i++) {
            PRINT1("\t[%d]=%d,%d ", i, led_string->StripSegmentOffset(strip_index, i), led_string->StripSegmentPixelCount(strip_index, i));
        }
        LOG1("\tTYPE: ");
        switch (led_string->StripPixelType(strip_index)) {
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
            PRINT1("\tunknown(%d)", led_string->StripPixelType(strip_index));
            break;
        }
        LOG1("\n");
    }
    LOG1("============================= LED ===============================\n");
} // end of setup()

void loop()
{
    homeSpan.poll();

    // check whether it is time to perform a check for new software
    uint64_t now = micros();
    if (now > next_ota_check_time) {
        /*PRINT1("%lld: doing OTA check\n", now);
        String manifest_url = OTA_CHECK_URL;
        String mac = WiFi.macAddress();
        mac.replace(":", "");
        manifest_url += "?id=" + mac;
        manifest_url += "&t=";
        manifest_url += OTA_FIRMWARE_TYPE;
        manifest_url += "&v=";
        manifest_url += String(OTA_FIRMWARE_VERSION);
        uint32_t free_space = ESP.getFreeSketchSpace();
        manifest_url += "&f=";
        manifest_url += free_space;
        int8_t rssi = WiFi.RSSI();
        manifest_url += "rssi=";
        manifest_url += rssi;
        esp32FOTAtool.setManifestURL(manifest_url);
        esp32FOTAtool.useDeviceId(false);
        bool updatedNeeded = esp32FOTAtool.execHTTPcheck();
        if (updatedNeeded) {
            last_progress = 0;
            last_percentage = -1;
            led_string->SetBrightness(4);
            led_string->SetColorRGB(0, 255, 0);
            led_string->SetSpeed(128);
            led_string->SetMode360(199); // comet
            esp32FOTAtool.setProgressCb(progress_updater);
            esp32FOTAtool.execOTA();
            led_string->SetColorRGB(255, 0, 0);
        }
        next_ota_check_time = now + (60ULL * 60ULL * 1000000ULL) * OTA_CHECK_INTERVAL_HOURS;
        PRINT1("next OTA check: %lld\n", next_ota_check_time);*/
    }

    led_string->MaterialisePixelData(MAIN_LOOP_DELAY);
    delay(MAIN_LOOP_DELAY);
} // end of loop()
