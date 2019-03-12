
/*
A quick/minimal ISR based button handler
*/
#include <Arduino.h>
#include "board.h"

class InterruptButtons
{
    volatile uint32_t pressMillis[NUM_BUTTONS]; // time of press or 0 if not pressed
    volatile bool pressed[NUM_BUTTONS];

  public:
    // FIXME shouldn't really be public
    const uint8_t gpios[NUM_BUTTONS] = BUTTON_GPIOS; // FIXME - ugly init

    void setup();      // Call to attach to each of the GPIOs
    bool anyPressed(); // any unhandled buttons? (note: calling this function does not mark the buttons has handled)

    /**
     * Calling this function both checks for presses and marks the current press as handled. 
     * 
     * Return true if the indicated button has been pressed. false otherwise. 
     */
    bool handle(int buttonNum);

  private:
    static void isr();
};

extern InterruptButtons buttons;