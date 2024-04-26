
////////////////////////////////////
//   DEVICE-SPECIFIC LED SERVICES //
////////////////////////////////////

#include "led_configurations.h"

typedef void (*LED_change_callback)(void);
typedef void (*mode_change_callback)(int);

typedef struct
{
  uint8_t power;
  float H;
  float S;
  int V;
  LED_change_callback on_HomeKit_change;
} LED_changes_t;

struct DEV_LED;
typedef struct
{
  uint8_t id;
  uint8_t mode_state;
  uint8_t FX_power;
  int FX_mode;
  int FX_speed;
  int FX_direction;
  DEV_LED *mode_switch;
  mode_change_callback on_HomeKit_change;
} modes_t;

////////////////////////////////////

struct DEV_LED : Service::LightBulb
{ // ON/OFF LED

  SpanCharacteristic *power; // reference to the On Characteristic
  modes_t *mode_configuration;

  DEV_LED(
      modes_t *mode_configuration) : Service::LightBulb()
  { // constructor() method

    power = new Characteristic::On();
    mode_configuration->mode_state = power->getVal();

    this->mode_configuration = mode_configuration;

    //Serial.print("Configuring On/Off LED: ");   // initialization message
    //Serial.print((uint32_t)LED_power);
    //Serial.print("\n");
  } // end constructor

  boolean update()
  { // update() method

    LOG1("Updating On/Off LED");
    LOG1(":  Current Power=");
    LOG1(power->getVal() ? "true" : "false");
    LOG1("  New Power=");
    LOG1(power->getNewVal() ? "true" : "false");
    LOG1("\n");

    mode_configuration->mode_state = power->getNewVal();
    mode_configuration->on_HomeKit_change(mode_configuration->id);

    return (true); // return true

  } // update
};

////////////////////////////////////

struct DEV_RgbLED : Service::LightBulb
{ // RGB LED (Command Cathode)

  SpanCharacteristic *power; // reference to the On Characteristic
  SpanCharacteristic *H;     // reference to the Hue Characteristic
  SpanCharacteristic *S;     // reference to the Saturation Characteristic
  SpanCharacteristic *V;     // reference to the Brightness Characteristic
  LED_changes_t *changes;
  uint32_t last_debug_output = 0;

  DEV_RgbLED(
      LED_changes_t *changes) : Service::LightBulb()
  { // constructor() method

    power = new Characteristic::On();
    H = new Characteristic::Hue(changes->H);        // instantiate the Hue Characteristic with an initial value of 0 out of 360
    S = new Characteristic::Saturation(changes->S); // instantiate the Saturation Characteristic with an initial value of 100%
    V = new Characteristic::Brightness(changes->V); // instantiate the Brightness Characteristic with an initial value of 100%
    // new SpanRange(5, 100, 1);                // sets the range of the Brightness to be from a min of 1%, to a max of 100%, in steps of 1%

    this->changes = changes;

    //char cBuf[128];
    //sprintf(cBuf,"Configuring RGB LED: Channels=(%d,%d,%d)\n",redChannel,greenChannel,blueChannel);
    //Serial.print(cBuf);

  } // end constructor

  boolean update()
  { // update() method

    boolean p;
    float h, s;
    int v;

    h = H->getVal<float>(); // get and store all current values.  Note the use of the <float> template to properly read the values
    s = S->getVal<float>();
    v = V->getVal();
    p = power->getVal();

    char cBuf[128];
    //sprintf(cBuf,"Updating RGB LED on pins=(%d,%d,%d): ",redPin->getPin(),greenPin->getPin(),bluePin->getPin());
    //LOG1(cBuf);

    if (power->updated())
    {
      changes->power = p = power->getNewVal();
      sprintf(cBuf, "Power=%s->%s, ", power->getVal() ? "true" : "false", p ? "true" : "false");
    }
    else
    {
      sprintf(cBuf, "Power=%s, ", p ? "true" : "false");
    }
    LOG1(cBuf);

    if (H->updated())
    {
      changes->H = h = H->getNewVal<float>();
      sprintf(cBuf, "H=%.0f->%.0f, ", H->getVal<float>(), h);
    }
    else
    {
      sprintf(cBuf, "H=%.0f, ", h);
    }
    LOG1(cBuf);

    if (S->updated())
    {
      changes->S = s = S->getNewVal<float>();
      sprintf(cBuf, "S=%.0f->%.0f, ", S->getVal<float>(), s);
    }
    else
    {
      sprintf(cBuf, "S=%.0f, ", s);
    }
    LOG1(cBuf);

    if (V->updated())
    {
      changes->V = v = V->getNewVal();
      sprintf(cBuf, "V=%d->%d  ", V->getVal(), v);
    }
    else
    {
      sprintf(cBuf, "V=%d  ", v);
    }
    LOG1(cBuf);

    changes->on_HomeKit_change();

    return (true); // return true

  } // update

  void loop()
  {
    // look for changes in the power, H, S or V due to external factors

    uint8_t p;
    int v;
    float h, s;
    char cBuf[128];

    h = H->getVal<float>(); // get and store all current values.  Note the use of the <float> template to properly read the values
    s = S->getVal<float>();
    v = V->getVal();
    p = power->getVal();

    /*uint32_t now = millis();
    if (now - last_debug_output > 10000) {
      last_debug_output = now;
      sprintf(cBuf, "power=%d->%d, H=%.0f->%.0f, S=%.0f->%.0f, V=%d->%d\n", p, changes->power, h, changes->H, s, changes->S, v, changes->V);
      LOG1(cBuf);
    }*/

    if ((p & 1) != (changes->power & 1))
    {
      sprintf(cBuf, "power=%d->%d\n", p, changes->power);
      LOG1(cBuf);
      // use bit 0 only (bit 7 is used to indicate whether the FX were ON when the LED was switched off)
      power->setVal(changes->power & 1);
    }

    if (h != changes->H)
    {
      sprintf(cBuf, "H=%.0f->%.0f\n", h, changes->H);
      LOG1(cBuf);
      H->setVal(changes->H);
    }

    if (s != changes->S)
    {
      sprintf(cBuf, "S=%.0f->%.0f\n", s, changes->S);
      LOG1(cBuf);
      S->setVal(changes->S);
    }

    if (v != changes->V)
    {
      sprintf(cBuf, "V=%d->%d\n", v, changes->V);
      LOG1(cBuf);
      V->setVal(changes->V);
    }
  } // loop
};

//////////////////////////////////
