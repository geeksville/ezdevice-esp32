#pragma once

#ifndef AUTOBUILD // If we are being invoked in a release script make sure these settings come from there
// #define BOARD_TTGO_T4
// #define BOARD_TTGO_T5s
// #define BOARD_TTGO_T5_23 // with a 2.13" screen
// #define BOARD_TTGO_T5_16_RED // with a red-black 2.9" screen
// #define BOARD_TTGO_T5_16_YELLOW // with a red-black 2.9" screen
// #define BOARD_TTGO_T_JOURNAL // camera oled
#define BOARD_TTGO_CAMERA // camera with oled, bme280 and motion sensor
// #define BOARD_TTGO_O // what I'm calling their TTGO oled board with a battery
#endif

#ifdef BOARD_TTGO_T4

// color TFT
#define JOYBOARD_TYPE 'R'

#define DISP_ADAFRUIT // If defined we assume either TFT or OLED which can be drawn with the regular adafruit draw operations
#define DISP_COLOR    // If defined we assume color display, else assumed mono

// Deep sleep works on this board, but the power manager will shut us down automatically after a while (I think - still TBD)
#define DEEPSLEEP_INTERVAL (365 * 24 * 60 * 60 * 1000ULL) // sleep after we've received one message from the server (or we ran out of time), sleep for this many msecs
#define DEEPSLEEP_IDLE (120 * 1000)                       // This this period passes without any activity (button press or message from server), go to sleep

#define TFT_CS 27 // If undefined we assume no TFT screen
#define TFT_DC 26
#define TFT_MOSI 23 // VSPI MOSI
#define TFT_CLK 18  // VSPI CLK
#define TFT_RST 5
#define TFT_MISO 12
#define TFT_LED 4 // backlight

// Button defs for this board
// FIXME add back GPIO 0/boot as button 4 but only after turning on internal pullup and testing
#define NUM_BUTTONS 3
#define BUTTON_GPIOS \
  {                  \
    38, 37, 39       \
  }
#define FACTORYRESET_BUTTON 38 // one
#define PANICUPDATE_BUTTON 37  // two

#define DISPLAY_ROTATION 3 // 0 & 2 Portrait. 1 & 3 landscape
#define DISPLAY_UPDATE     // command needed to flush the display to the actual hardware

#define BACKGROUND 0x4228 // Background color
#define FOREGROUND ILI9341_WHITE
#define ACCENT ILI9341_BLUE

#define DRAW_ANIM_DELAY 300

#elif defined(BOARD_TTGO_O)

#define JOYBOARD_TYPE 'O'

#define DISP_ADAFRUIT                            // If defined we assume either TFT or OLED which can be drawn with the regular adafruit draw operations
// #define DISP_COLOR // If defined we assume color display, else assumed mono

// deep sleep works well on this board, but the sleep current draw is 10mA due to something buzzing on the board (regulator?)
#define DEEPSLEEP_INTERVAL (24 * 60 * 60 * 1000) // sleep after we've received one message from the server (or we ran out of time), sleep for this many msecs
#define DEEPSLEEP_IDLE (30 * 1000)               // This this period passes without any activity (button press or message from server), go to sleep

#define STATUS_LED 16 // This board has a GPIO hooked to an LED, high is LED on

#define DISABLE_BROWNOUT // this board is powered by a battery with low voltage

#define OLED_SDA 5
#define OLED_SCL 4
#define OLED_ADDR 0x3c // i2c addr
// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET -1  // Reset pin # (or -1 if sharing Arduino reset pin)

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Button defs for this board
#define NUM_BUTTONS 1
#define BUTTON_GPIOS \
  {                  \
    0                \
  } // 0 is boot, 2 is en but hardwired to reset on the board
// #define FACTORYRESET_BUTTON 39 // three
// #define PANICUPDATE_BUTTON 38 // one

#define DISPLAY_ROTATION 0            // 0 & 2 Portrait. 1 & 3 landscape
#define DISPLAY_UPDATE disp.display() // command needed to flush the display to the actual hardware

#define BACKGROUND BLACK // Background color
#define FOREGROUND WHITE
#define ACCENT WHITE

#define BITMAP_WHITE BLACK // We are mostly drawing text, so we want a black background for OLED (and the server is sending white for the background)
#define BITMAP_BLACK WHITE

#define DRAW_ANIM_DELAY 300

#elif defined(BOARD_TTGO_T5s)

// assume eink
#define JOYBOARD_TYPE 'M'

// #define DISP_ADAFRUIT // If defined we assume either TFT or OLED which can be drawn with the regular adafruit draw operations
// #define DISP_COLOR // If defined we assume color display, else assumed mono

// Note: if not using DEEPSLEEP_CHARGE
#define DEEPSLEEP_INTERVAL (7 * 24 * 60 * 60 * 1000ULL) // sleep after we've received one message from the server (or we ran out of time), sleep for this many msecs
#define DEEPSLEEP_IDLE (1 * 60 * 1000)                  // This this period passes without any activity (button press or message from server), go to sleep

// This board does have a busted charge controller, but in the interest of maximum battery life I'm letting the controller kill has after we've been in deep sleep for five minutes
// #define DEEPSLEEP_CHARGEWAKE (25 * 1000)         // define this if there is a busted charge controller on the board and we need to wake periodically to keep it from killing us.

#define STATUS_LED 22 // This board has a GPIO hooked to an LED, high is LED on

// Button defs for this board
#define NUM_BUTTONS 3

// from left to right they are RESET, 38 37 39
// So I map these so that button "one" is the right most, then two is middle, then three is left
// I chose this odd mapping to keep the users finger away from the reset button
// This mapping is definitely correct - I need to fix my plastic buttons to not double press
#define BUTTON_GPIOS \
  {                  \
    37, 38, 39       \
  }

#define FACTORYRESET_BUTTON 37 // one
#define PANICUPDATE_BUTTON 38  // two

#define DISPLAY_ROTATION 1           // 0 & 2 Portrait. 1 & 3 landscape
#define DISPLAY_UPDATE disp.update() // command needed to flush the display to the actual hardware

#define BACKGROUND GxEPD_WHITE // Background color
#define FOREGROUND GxEPD_BLACK
#define ACCENT GxEPD_RED

#define BITMAP_WHITE GxEPD_WHITE
#define BITMAP_BLACK GxEPD_BLACK

#define DRAW_ANIM_DELAY 0 // This display can't show animations anyways, so just go fast

/*
   The connections to the eInk display do not seem to be listed anywhere,
   but I found that using an Arduino eInk library from https://github.com/ZinggJM/GxEPD
   Hence the pins used for the eInk display appear to be

   plus the default ESP32 SPI data and clock pins
 */

#define EPD_RESET 16 // can set to -1 and share with microcontroller Reset!
#define EPD_BUSY 4   // can set to -1 to not use a pin (will wait a fixed delay)
#define EPD_CS 5
#define EPD_MOSI 23
#define EPD_CLK 18
#define EPD_MISO -1 // unused
#define EPD_DC 17

#elif defined(BOARD_TTGO_T5_23)

// assume eink
#define JOYBOARD_TYPE 'L'
#define BOARD_TTGO_T5 // Most T5 setup is the same

// Button defs for this board
#define NUM_BUTTONS 1

// from left to right they are RESET, 38 37 39
// So I map these so that button "one" is the right most, then two is middle, then three is left
// I chose this odd mapping to keep the users finger away from the reset button
// This mapping is definitely correct - I need to fix my plastic buttons to not double press
#define BUTTON_GPIOS \
  {                  \
    39               \
  }

#define FACTORYRESET_BUTTON 39 // one
// #define PANICUPDATE_BUTTON 38  // two

#define STATUS_LED 19 // This board has a GPIO hooked to an LED, high is LED on

// Various datasources to poll perodically and upload to our server
#define PUSH_SOURCES         \
  {                          \
    {                        \
      "Vbat", 35, 1.0f, 0.0f \
    }                        \
  }

#elif defined(BOARD_TTGO_T5_16_RED)

#define JOYBOARD_TYPE 'K'
#define BOARD_TTGO_T5_16

#elif defined(BOARD_TTGO_T5_16_YELLOW)

#define JOYBOARD_TYPE 'Y'
#define BOARD_TTGO_T5_16

#elif defined(BOARD_TTGO_T_JOURNAL)

#define JOYBOARD_TYPE 'T'

#define CAM_CONFIG esp32cam_config

#define DISP_ADAFRUIT // If defined we assume either TFT or OLED which can be drawn with the regular adafruit draw operations
// #define DISP_COLOR // If defined we assume color display, else assumed mono

// deep sleep works well on this board, but the sleep current draw is 10mA due to something buzzing on the board (regulator?)
//#define DEEPSLEEP_INTERVAL (24 * 60 * 60 * 1000) // sleep after we've received one message from the server (or we ran out of time), sleep for this many msecs
//#define DEEPSLEEP_IDLE (60 * 1000)               // This this period passes without any activity (button press or message from server), go to sleep

// #define STATUS_LED 4 // This board has a GPIO hooked to an LED, high is LED on

// #define DISABLE_BROWNOUT // this board is powered by a battery with low voltage

#define OLED_SDA 14
#define OLED_SCL 13
#define OLED_ADDR 0x3c // i2c addr
// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET -1  // Reset pin # (or -1 if sharing Arduino reset pin)

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

// Button defs for this board
#define NUM_BUTTONS 1
#define BUTTON_GPIOS \
  {                  \
    32               \
  }
#define FACTORYRESET_BUTTON 32 // one
// #define PANICUPDATE_BUTTON 38 // one

#define DISPLAY_ROTATION 0            // 0 & 2 Portrait. 1 & 3 landscape
#define DISPLAY_UPDATE disp.display() // command needed to flush the display to the actual hardware

#define BACKGROUND BLACK // Background color
#define FOREGROUND WHITE
#define ACCENT WHITE

#define BITMAP_WHITE BLACK // We are mostly drawing text, so we want a black background for OLED (and the server is sending white for the background)
#define BITMAP_BLACK WHITE

#define DRAW_ANIM_DELAY 300

#elif defined(BOARD_TTGO_CAMERA)

#define JOYBOARD_TYPE 'C'

#define CAM_CONFIG esp32cam_ttgo_t_config

#define DISP_ADAFRUIT // If defined we assume either TFT or OLED which can be drawn with the regular adafruit draw operations
// #define DISP_COLOR // If defined we assume color display, else assumed mono

// deep sleep works well on this board, but the sleep current draw is 10mA due to something buzzing on the board (regulator?)
//#define DEEPSLEEP_INTERVAL (24 * 60 * 60 * 1000) // sleep after we've received one message from the server (or we ran out of time), sleep for this many msecs
//#define DEEPSLEEP_IDLE (30 * 1000)               // This this period passes without any activity (button press or message from server), go to sleep

// #define STATUS_LED 4 // This board has a GPIO hooked to an LED, high is LED on

// #define DISABLE_BROWNOUT // this board is powered by a battery with low voltage

#define OLED_SDA 21
#define OLED_SCL 22
#define OLED_ADDR 0x3c // i2c addr
// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET -1  // Reset pin # (or -1 if sharing Arduino reset pin)

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Button defs for this board
#define NUM_BUTTONS 1
#define BUTTON_GPIOS \
  {                  \
    34               \
  }
#define FACTORYRESET_BUTTON 34 // one
// #define PANICUPDATE_BUTTON 38 // one

#define DISPLAY_ROTATION 2            // 0 & 2 Portrait. 1 & 3 landscape
#define DISPLAY_UPDATE disp.display() // command needed to flush the display to the actual hardware

#define BACKGROUND BLACK // Background color
#define FOREGROUND WHITE
#define ACCENT WHITE

#define BITMAP_WHITE BLACK // We are mostly drawing text, so we want a black background for OLED (and the server is sending white for the background)
#define BITMAP_BLACK WHITE

#define DRAW_ANIM_DELAY 300

#else

#error board type not set

#endif

#ifdef BOARD_TTGO_T5_16
#define BOARD_TTGO_T5 // Most T5 setup is the same

// Button defs for this board
#define NUM_BUTTONS 3

// from left to right they are RESET, 38 37 39
// So I map these so that button "one" is the right most, then two is middle, then three is left
// I chose this odd mapping to keep the users finger away from the reset button
// This mapping is definitely correct - I need to fix my plastic buttons to not double press
#define BUTTON_GPIOS \
  {                  \
    38, 37, 39       \
  }

#define FACTORYRESET_BUTTON 38 // one
#define PANICUPDATE_BUTTON 37  // two

#define STATUS_LED 22 // This board has a GPIO hooked to an LED, high is LED on
#endif

#ifdef BOARD_TTGO_T5

// #define DISP_ADAFRUIT // If defined we assume either TFT or OLED which can be drawn with the regular adafruit draw operations
// #define DISP_COLOR // If defined we assume color display, else assumed mono

// Note: if not using DEEPSLEEP_CHARGE
#define DEEPSLEEP_INTERVAL ((24 / 2) * 60 * 60 * 1000ULL) // sleep after we've received one message from the server (or we ran out of time), sleep for this many msecs
#define DEEPSLEEP_IDLE (20 * 1000)                        // This this period passes without any activity (button press or message from server), go to sleep

// This board does have a busted charge controller, but in the interest of maximum battery life I'm letting the controller kill has after we've been in deep sleep for five minutes
// #define DEEPSLEEP_CHARGEWAKE (25 * 1000)         // define this if there is a busted charge controller on the board and we need to wake periodically to keep it from killing us.

#define DISPLAY_ROTATION 3           // 0 & 2 Portrait. 1 & 3 landscape
#define DISPLAY_UPDATE disp.update() // command needed to flush the display to the actual hardware

#define BACKGROUND GxEPD_WHITE // Background color
#define FOREGROUND GxEPD_BLACK
#define ACCENT GxEPD_RED

#define BITMAP_WHITE GxEPD_WHITE
#define BITMAP_BLACK GxEPD_BLACK

#define DRAW_ANIM_DELAY 0 // This display can't show animations anyways, so just go fast

/*
   The connections to the eInk display do not seem to be listed anywhere,
   but I found that using an Arduino eInk library from https://github.com/ZinggJM/GxEPD
   Hence the pins used for the eInk display appear to be

   plus the default ESP32 SPI data and clock pins
 */

#define EPD_RESET 16 // can set to -1 and share with microcontroller Reset!
#define EPD_BUSY 4   // can set to -1 to not use a pin (will wait a fixed delay)
#define EPD_CS 5
#define EPD_MOSI 23
#define EPD_CLK 18
#define EPD_MISO -1 // unused
#define EPD_DC 17

#endif
