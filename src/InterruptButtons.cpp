/*
A quick/minimal ISR based button handler
*/
#include "InterruptButtons.h"
#include "driver/rtc_io.h"

InterruptButtons buttons;

#ifndef DEBOUNCE_MS 
#define DEBOUNCE_MS 200
#endif

void IRAM_ATTR InterruptButtons::isr()
{
    InterruptButtons &_this = buttons;

    uint32_t now = millis();
    // FIXME - this reading the GPIOs is kinda ugly, better to instead check why our ISR got invoked...
    for (int i = 0; i < NUM_BUTTONS; i++)
    {
        bool val = digitalRead(_this.gpios[i]);
        if(!_this.isActiveHigh[i])
            val = !val;

        if (val) // pressed
            if (_this.pressMillis[i] == 0 || now >= _this.pressMillis[i] + DEBOUNCE_MS || now < _this.pressMillis[i])
            { // ignore bounces but be careful about when time rolls over
                _this.pressMillis[i] = now;
                _this.pressed[i] = true;
            }
    }
}

void InterruptButtons::setup(bool _useInterrupts)
{
    useInterrupts = _useInterrupts;

    uint32_t now = millis();
    for (int i = 0; i < NUM_BUTTONS; i++)
    {
        rtc_gpio_deinit((gpio_num_t)gpios[i]); // Make sure it is disconnected from the RTC (where it was attached over sleep)

        pinMode(gpios[i], INPUT);

        // Check if the button is _already_ pressed 
        bool val = digitalRead(gpios[i]);
        if(!isActiveHigh[i])
            val = !val;

        if (val)
        {
            pressed[i] = true;
            pressMillis[i] = now;
        }
        else
        {
            pressed[i] = false;
            pressMillis[i] = 0;
        }

        // Set motionSensor pin as interrupt, assign interrupt function
        if (useInterrupts)
            attachInterrupt(digitalPinToInterrupt(gpios[i]), isr, isActiveHigh[i] ? RISING : FALLING);
    }
}

void InterruptButtons::loop()
{
    // If we aren't allowed to use interrupts we just fake it with polling
    if (!useInterrupts)
        isr();
}

bool InterruptButtons::anyPressed()
{
    for (int i = 0; i < NUM_BUTTONS; i++)
    {
        if (pressed[i])
            return true;
    }
    return false;
}

bool InterruptButtons::handle(int buttonNum)
{
    noInterrupts();
    bool wasPressed = pressed[buttonNum];
    if (wasPressed)
        pressed[buttonNum] = false;
    interrupts();

    return wasPressed;
}