/*
A quick/minimal ISR based button handler
*/
#include "InterruptButtons.h"
#include "driver/rtc_io.h"

InterruptButtons buttons;

#define DEBOUNCE_MS 200

void IRAM_ATTR InterruptButtons::isr()
{
    InterruptButtons &_this = buttons;

    uint32_t now = millis();
    // FIXME - this reading the GPIOs is kinda ugly, better to instead check why our ISR got invoked...
    for (int i = 0; i < NUM_BUTTONS; i++)
    {
        if (!digitalRead(_this.gpios[i])) // active low
            if (_this.pressMillis[i] == 0 || now >= _this.pressMillis[i] + DEBOUNCE_MS || now < _this.pressMillis[i])
            { // ignore bounces but be careful about when time rolls over
                _this.pressMillis[i] = now;
                _this.pressed[i] = true;
            }
    }
}

void InterruptButtons::setup()
{
    uint32_t now = millis();
    for (int i = 0; i < NUM_BUTTONS; i++)
    {
        rtc_gpio_deinit((gpio_num_t)gpios[i]); // Make sure it is disconnected from the RTC (where it was attached over sleep)

        pinMode(gpios[i], INPUT);

        // Check if the button is _already_ pressed (we assume active low)
        if (!digitalRead(gpios[i]))
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
        attachInterrupt(digitalPinToInterrupt(gpios[i]), isr, FALLING);
    }
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