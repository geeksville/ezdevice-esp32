# This project is in alpha-test

You probably don't want it yet. But if you are trying to make this work (i.e. I've sent you a note), please post in the group chat: [![Gitter](https://badges.gitter.im/ezdevice/esp32-client.svg)](https://gitter.im/ezdevice/esp32-client?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge)

![photo of cases](/doc/cases.jpg)

## Build instructions

This project uses the simple PlatformIO build system. You can use the IDE, but for brevity
in these instructions I describe use of their command line tool.

1. Purchase one of the TTGO boards (see below for options).
2. Install [PlatformIO](https://platformio.org/).
3. Download this git repo and cd into it.
4. pio run -t upload (This command will fetch dependencies, build the project and install it on the board via USB)

The first time you run your device you'll need to connect to the "EZDevice-????" wifi network. This device uses a small internal webserver to let you give the necessary information it needs to connect to _your_ wifi network. After the device is connected to your wifi, it should begin fetching sample images from our servers. You can now go to the QR code printed on the back of your unit to login, set preferences and control/customize your device.

When setting up wifi on your device your phone should look approximately like this:

![screen](/doc/screen1.png)

![screen](/doc/screen2.png)

![screen](/doc/screen3.png)

![screen](/doc/screen4.png)

## Master repository

The master repository for this project is [located here](https://github.com/geeksville/ezdevice-esp32).

# Supported hardware

This project works on many different ESP32 based boards mostly from [Lilygo](http://www.lilygo.cn/). Support for cameras is not yet included in this repo, but will be released in a couple of weeks.

## T-Gallery

Seems undocumented so far.

## 4 button 2.4" screen <https://github.com/LilyGO/TTGO-TM-ESP32>

Not yet tested, but should be easy to support. Web resources:

<https://github.com/moononournation/ESP32_BiJin_ToKei>
ESP32_TFT_Library can support ILI9341, ILI9488, ST7789V and ST7735. This time I am using a 2.4" ST7789V LCD, model number JLX240-00302-BN. This model is designed for SPI only, so it only have 10 pins (actually 9 pins). It can help the soldering work easier.
<https://github.com/loboris/ESP32_TFT_library>

## TTGO wifi \$10 1306 OLED 128x64 with battery

Works well
board type W in my arch
https://www.banggood.com/Wemos-TTGO-WiFi-Bluetooth-Battery-ESP32-0_96-Inch-OLED-Development-Tool-p-1213497.html?rmmds=myorder&cur_warehouse=CN
There is an integrated blue LED on digital pin 16
deep sleep works well on this board, but the sleep current draw is 10mA due to something buzzing on the board (regulator?)

## epaper TTGO T5_V2.3 250x122 2.13"

board type L in my arch
Works well
GDE0213B1 display
LED on IO19
Vbat analog in on GPIO35

## epaper TTGO T5_V1.6 red/black eink 2.9" 296x128

Board type K in my arch
GDEW029Z10
display size 66.89 × 29.05 (outer dimension 79.0 × 36.7)

## epaper TTGO T5s V1.9 264x176 2.7"

sleep works if a nasty workaround is done to wake every 25 seconds for 200ms and draw about 100mA.
Current draw while CPU running and wifi on: about 100mA
Current draw while asleep 10mA (probably the LEDs and audio amp)

Also my wife reads Chinese and she helped me read the IP5306 datasheet (it shuts down if it doesn't see at least 150ms of >45mA power consumption in any 32s window). The hack I used in sw to keep it from killing the board was to wake the CPU from deep sleep every 30 seconds have it burn 100mAish of power for 200ms and then go back to sleep (ugly, but no soldering needed).

LED from IO22 is blue.
AKK-BEV is the 16 pin chip next to audio in and speaker pins, I assume it is the audio amp. If it is like the T4 audio comes from IO25

SPH0645LM4H (little metal can) chip used for the mic input per https://github.com/LilyGO/TTGO-T5S-Epaper/blob/master/ESP32_MEMSMic/ESP32_MEMSMic.ino . To save power make pin 32 held high or low during sleep

i2s_pin_config_t pin_config = {
.bck_io_num = 32, //this is BCK pin OUT
.ws_io_num = 33, // this is LRCK pin OUT
.data_out_num = I2S_PIN_NO_CHANGE, // this is DATA output pin
.data_in_num = 27 //DATA IN
};

Uses good display b/w/r panel: GDEW027C44
<https://github.com/LilyGO/TTGO-T5S-Epaper>
<https://github.com/LilyGO/TTGO-T5S-Epaper/tree/master/epd2in7-demo>
waveshare makes sw for these screens: <https://www.waveshare.com/w/upload/4/4a/E-Paper_ESP32_Driver_Board_user_manual_en.pdf>
<https://www.waveshare.com/wiki/File:E-Paper_ESP32_Driver_Board_Code.7z>
<https://www.waveshare.com/wiki/2.7inch_e-Paper_HAT> &lt;- seems to be the display

## 3 button oled TTGO T4 1.1 240x320 ILI9341 screen

Works well
ID code O
<https://www.banggood.com/Wemos-TTGO-BTC-Ticker-ESP32-Module-For-Arduino-Source-Bitcoin-Price-Ticker-Program-4-MB-SPI-Flash-p-1303223.html?cur_warehouse=CN>
<https://github.com/LilyGO/TTGO-T4-DEMO>

## 3 botton TFT with case, battery, 4MB PSRAM

Same as TTGO T4
(sold as BTC ticker: https://www.aliexpress.com/item/TTGO-T-Watcher-BTC-Ticker-ESP32-for-Arduino-Bitcoin-Price-Ticker-Program-4-MB-SPI-Flash/32890756907.html?spm=a2g0s.9042311.0.0.26fc4c4d51EXTe)

# TTGO-Camera

This board looks interesting: https://github.com/LilyGO/esp32-camera-bme280
I haven't tried it but

# TTGO HiGrow

ID code G
code not written yet - FIXME see https://github.com/LilyGO/higrowopen/blob/master/HiGrowEsp32/HiGrowEsp32.ino
analogRead(32) for the conductive sensor
#define DHTTYPE DHT11 // DHT 22 (AM2302), AM2321
const int DHTPin = 22;
// Initialize DHT sensor.
DHT dht(DHTPin, DHTTYPE);
// Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
float h = dht.readHumidity();
// Read temperature as Celsius (the default)
float t = dht.readTemperature();

# Hardware to avoid

## early versions of the TTGO T5 seem to have issues with staying alive on battery

Anything below v1.2 of this board you probably don't want.
https://www.rogerclark.net/ttgo-t5-eink-display-update/
btw - they seem to have a 1.2 version of this board out and based on the schematic I found they seem to have a much better charge controller.
