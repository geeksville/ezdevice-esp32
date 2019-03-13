/*
   The MIT License (MIT)

   Copyright © 2019 S. Kevin Hester, kevinh@geeksville.com https://github.com/geeksville/ezdevice-esp32

   Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <WiFi.h>
#include <ByteStream.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Bounce2.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <HTTPClient.h>
#include <JPEGDecoder.h>
#include <ESP32-AutoUpdate.h>
#include <Fonts/FreeSans9pt7b.h>
#include <esp_sleep.h>
#include <esp_wifi.h>
#include <Arduino.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "driver/rtc_io.h"
#include "FS.h"
#include "SPIFFS.h"
#include "buildnum.h"
#include <InterruptButtons.h>
#include <AutoConnect.h>

// #include "wifikeys.h"
#include "board.h"

// nasty magic to stringify a numeric macro
#define xstr(s) str(s)
#define str(s) #s
#define VERSION_STRING "V0.1.6-" xstr(VERSION_NUM)

void jpegRender(int xpos, int ypos);

WebServer webServer;
AutoConnectConfig portalConfig;
AutoConnect portal(webServer);

// We construct our filename to be different for each board type
// NOTE: HTTPS DOES NOT WORK YET - Gets a -11 error
AutoUpdate update(String("http://") + xstr(BUCKETNAME) + ".s3.amazonaws.com/" + "firmware-J" + JOYBOARD_TYPE);

RTC_DATA_ATTR int bootCount = 0;

#ifdef DEEPSLEEP_CHARGEWAKE
RTC_DATA_ATTR uint64_t sleepsToSkip = 0; // if we are doing a hack to wake just for the charge controller, we sleep again after most of those wakes
#endif

#ifdef TFT_CS

#include <Adafruit_ILI9341.h>

// software spi
// Adafruit_ILI9341 disp = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST, TFT_MISO);
Adafruit_ILI9341 disp = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

#elif defined(OLED_ADDR)

#include <Wire.h>
#include <Adafruit_SSD1306.h>

Adafruit_SSD1306 disp(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#elif defined(EPD_CS)

#include <GxIO/GxIO_SPI/GxIO_SPI.h>
#include <GxIO/GxIO.h>

class GxIO_TTGO : public GxIO_SPI
{
  public:
    GxIO_TTGO() : GxIO_SPI(SPI, EPD_CS, EPD_DC, EPD_RESET)
    {
    }

    // We need to replace the standard init method with one that uses our nonstandard pins
    void init()
    {
        if (_cs >= 0)
        {
            digitalWrite(_cs, HIGH);
            pinMode(_cs, OUTPUT);
        }
        if (_dc >= 0)
        {
            digitalWrite(_dc, HIGH);
            pinMode(_dc, OUTPUT);
        }
        if (_rst >= 0)
        {
            digitalWrite(_rst, HIGH);
            pinMode(_rst, OUTPUT);
        }
        if (_bl >= 0)
        {
            digitalWrite(_bl, HIGH);
            pinMode(_bl, OUTPUT);
        }
        reset();
        _spi.begin(EPD_CLK, -1, EPD_MOSI, _cs);
    }
};

GxIO_TTGO epaperIO;

#ifdef BOARD_TTGO_T5s
#include <GxGDEW027C44/GxGDEW027C44.h>
GxGDEW027C44 disp(epaperIO, EPD_RESET, EPD_BUSY);
#endif

#ifdef BOARD_TTGO_T5_23
#include <GxGDE0213B1/GxGDE0213B1.h>
GxGDE0213B1 disp(epaperIO, EPD_RESET, EPD_BUSY);
#endif

#ifdef BOARD_TTGO_T5_16
#include <GxGDEW029Z10/GxGDEW029Z10.h>
GxGDEW029Z10 disp(epaperIO, EPD_RESET, EPD_BUSY);
#endif

#else
#error no display defined
#endif

// Sources for periodic publishes of device state.
// Initially assume an analog input
// FIXME: generalize via inheritence to allow other input types or use a virtual function to read special value types
struct PushSource
{
    const char *name; // source will be published with this identifier
    int gpioNum;
    float scaling; // Used to prescale the read value before uploading
    float offset;  // uploaded value is rawValue * scale + offset
};

#ifdef PUSH_SOURCES
const PushSource pushSources[] = PUSH_SOURCES;
#else
const PushSource pushSources[] = {};
#endif

#define EVENT_TOPIC "event" // was "press"

uint64_t wakeButtons;         // If we woke due to a button press these bits will be set
esp_sleep_source_t wakeCause; // the reason we booted this time

// Bitmap_WiFi
extern uint8_t wifi_1[];
extern uint8_t wifi_2[];
extern uint8_t wifi_3[];

unsigned long previousMillis = 0;
long interval = 0;

// supposedly the current version is busted: http://www.iotsharing.com/2017/08/how-to-use-esp32-mqtts-with-mqtts-mosquitto-broker-tls-ssl.html
// WiFiClientSecure wifiClient;
WiFiClient mqttClient;
PubSubClient mqtt(mqttClient);

WiFiClient clientForHttp;
HTTPClient http;
ByteStream *foreverImage = NULL; // Our default image
ByteStream *tempImage = NULL;    // An image to show only once
bool haveNewImage = false;       // Tells our main thread it has to show either tempImage or foreverImage
bool showingTempImage = false;
String persistence; // The current type of persistence requested for this new image
fs::FS *flashFS;

uint32_t updateAtMillis = 0; // the server asked us to do a software update (in the main thread)

void publish(const char *suffix, const char *payload, bool retained);

void doDeepSleep()
{
#if DEEPSLEEP_INTERVAL

#ifdef DEEPSLEEP_CHARGEWAKE
    uint64_t msecToWake = DEEPSLEEP_CHARGEWAKE; // IP5306 datasheet says it will power down after 32s
#else
    uint64_t msecToWake = DEEPSLEEP_INTERVAL;
#endif
    Serial.printf("Entering deep sleep %llu\n", msecToWake);

    if (mqtt.connected())
    {                                       // Tell server we are shutting down (so that it won't have to wait for us to timeout)
        publish("status", "offline", true); // publish that we are offline
        mqtt.disconnect();
    }

    esp_wifi_stop();

#ifdef OLED_ADDR
    // Turn off the oled, it burns lots of power
    disp.ssd1306_command1(SSD1306_DISPLAYOFF);
    disp.ssd1306_command1(0x22); // switch to an unused Vcc so we are sure it can't get power SSD1306_EXTERNALVCC
#endif

#ifdef TFT_LED
    // Turn off the TFT LED - it draws lots of power
    digitalWrite(TFT_LED, LOW); // HIGH == Turn on backlight
#endif

#ifndef EPD_CS
    // If we are on a screen that doesn't have persistence, save the current image so we can restore it as soon as we have power (without needing a server)
    if (foreverImage)
        saveImage("/forever.jpg", *foreverImage); // may not really be a jpeg but whatever...
#endif

    // FIXME - use an external 10k pulldown so we can leave the RTC peripherals powered off
    // until then we need the following lines
    // testing leaving these pullups off to save power
    //esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);

    // Only GPIOs which are have RTC functionality can be used in this bit map: 0,2,4,12-15,25-27,32-39.
    uint64_t gpioMask = 0;

    // FIXME change polarity so we can wake on ANY_HIGH instead - that would allow us to use all three buttons (instead of just the first)
    for (int i = 0; i < 1 /* sizeof(buttons.gpios) */; i++)
    {
        gpio_num_t wakeGpio = (gpio_num_t)buttons.gpios[i];

        // schematic says external pullup resistors on the T5 board, assuming they are on the others as well.  the switch shorts to ground
        //gpio_pullup_en(wakeGpio);
        //gpio_pulldown_dis(wakeGpio);

        gpioMask |= (1ULL << wakeGpio);
    }

    // FIXME wakes too quickly
    esp_sleep_enable_ext1_wakeup(gpioMask, ESP_EXT1_WAKEUP_ALL_LOW);
    esp_sleep_enable_timer_wakeup(msecToWake * 1000ULL); // call expects usecs
    esp_deep_sleep_start();                              // 0.25 mA sleep current (battery)
#else
    Serial.println("Deep sleep disabled on this board");
#endif
}

uint32_t lastActivityMsec = 0;

/**
   Update our activity counter to hold off inactivity sleep a little longer
 */
void delaySleep()
{
    lastActivityMsec = millis();
}

/**
   Called from _any_ idle loop.  If we've been too long since our last delaySleep()
   put the machine to sleep
 */
void perhapsSleep()
{
#if DEEPSLEEP_INTERVAL
    if (millis() - lastActivityMsec >= DEEPSLEEP_IDLE)
    {
        Serial.println("Idle time expired, going to sleep.\n");

#ifdef DEEPSLEEP_CHARGEWAKE
        sleepsToSkip = DEEPSLEEP_INTERVAL / DEEPSLEEP_CHARGEWAKE;
#endif

        doDeepSleep();
    }
#endif
}

/** If this board has a status led - toggle it */
void blinkStatus()
{
#ifdef STATUS_LED
    static int state = 0;

    pinMode(STATUS_LED, OUTPUT);
    digitalWrite(STATUS_LED, state);
    state ^= 1;
#endif
}

/**
   Call this to enable the display.  But delay it until after the check for panic
   firmware update if necessary */
void turnOnDisplay()
{
    static bool displayOn;
    if (displayOn)
        return;

    displayOn = true;

#ifdef DISP_ADAFRUIT
#ifdef OLED_ADDR
    Wire.begin(OLED_SDA, OLED_SCL);
    // datasheet says max clock rate of 2.5us per pulse
    Wire.setClock(400000);

    // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
    // we set periphBegin because we already did a custom i2c init
    if (!disp.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR, true, false))
    { // Address 0x3D for 128x64
        Serial.println("SSD1306 allocation failed");
    }

    // disp.display(); // show boot screen
#else
    disp.begin(); // a standard adafruit (TFT?) display
#endif
#else
    disp.init(115200); // enable diagnostic output on Serial
#endif

#ifdef TFT_LED
    // Note: we delay turning on backlight till here so the user doesn't see
    // flickering during init
    pinMode(TFT_LED, OUTPUT);
    digitalWrite(TFT_LED, HIGH); // HIGH == Turn on backlight
#endif

    disp.setFont(&FreeSans9pt7b);
    disp.setRotation(DISPLAY_ROTATION); // 0 & 2 Portrait. 1 & 3 landscape
    Serial.printf("Display running, width %d, height %d\n", disp.width(), disp.height());
}

char clientId[16];
const char *shortId;

static char *base36enc(uint64_t value)
{
    static const char base36[37] = "0123456789abcdefghijklmnopqrstuvwxyz";
    /* log(2**64) / log(36) = 12.38 => max 13 char + '\0' */
    char buffer[14];
    unsigned int offset = sizeof(buffer);

    buffer[--offset] = '\0';
    do
    {
        buffer[--offset] = base36[value % 36];
    } while (value /= 36);

    return strdup(&buffer[offset]); // warning: this must be free-d by the user
}

void initClientId()
{
    uint64_t chipid = ESP.getEfuseMac(); //The chip ID is essentially its MAC address(length: 6 bytes).

#if 1
    // Note: this is in opposite of usual macaddr ordering - the MSB of the mac
    // is in the bottom of these eight bytes
    snprintf(clientId, sizeof(clientId), "J%c%02X%02X%02X%02X%02X%02X", JOYBOARD_TYPE,
             (uint32_t)((chipid >> 0) & 0xff),
             (uint32_t)((chipid >> 8) & 0xff),
             (uint32_t)((chipid >> 16) & 0xff),
             (uint32_t)((chipid >> 24) & 0xff),
             (uint32_t)((chipid >> 32) & 0xff),
             (uint32_t)((chipid >> 40) & 0xff));

    shortId = clientId + 2 + 4 * 2; // Offset into the longer string for a short suffix from the macaddr
#else
    // base36 only saved two characters, not worth it
    char *base36 = base36enc(chipid);
    snprintf(clientId, sizeof(clientId), "J%c%s", JOYBOARD_TYPE, base36);
    free(base36);
#endif
}

const char *getTopic(const char *suffix, const char *direction = "dev")
{
    static char buf[64];

    snprintf(buf, sizeof(buf), "/ezd/%s/%s/%s", direction, clientId, suffix);
    return buf;
}

void publish(const char *suffix, const char *payload, bool retained = false)
{
    printf("publish %s = %s\n", suffix, payload);

    mqtt.publish(getTopic(suffix), payload, retained);
}

/** save an image file to a filesystem
 */
void saveImage(const char *filename, ByteStream &stream)
{
    Serial.printf("Writing file: %s\n", filename);

    if (!flashFS)
    {
        Serial.println("no flash filesystem, can't write");
        return;
    }

    File file = flashFS->open(filename, FILE_WRITE);
    if (!file)
    {
        Serial.println("- failed to open file for writing");
        return;
    }

    file.write(stream.getBytes(), stream.available());
    file.close();
}

/**
 * read a file into a byte stream object caller must delete it
 * return null for error
 */
ByteStream *loadImage(const char *filename)
{
    Serial.printf("Reading file: %s\n", filename);

    if (!flashFS)
    {
        Serial.println("no flash filesystem, can't read");
        return NULL;
    }

    File file = flashFS->open(filename, FILE_READ);
    if (!file)
    {
        Serial.println("- failed to open file for reading");
        return NULL;
    }

    Serial.println("- read from file:");
    uint32_t len = file.available();
    if (!len)
    {
        file.close();
        return NULL;
    }

    ByteStream *out = new ByteStream(len);

    file.read(out->getBytes(), len);
    out->setLen(len);
    file.close();
    return out;
}

/**
 * Show an image
 * 
 * @param doUpdate if false we assume the caller will do the update call
 */
void showImageNow(ByteStream &stream, bool doUpdate = true)
{
    int len = stream.available();
    const uint8_t *payload = stream.getBytes();
    Serial.printf("Showing image: %d bytes\n", len);

    turnOnDisplay();
#ifdef DISP_COLOR
    // for color/greyscale screens we use jpeg, see below for mono
    boolean decoded = JpegDec.decodeArray(payload, len);
    if (decoded)
    {
        // print information about the image to the serial port
        // jpegInfo();

        // render the image onto the screen at given coordinates
        jpegRender(0, 0);
    }
    else
    {
        Serial.println("Jpeg file format not supported!");
    }
#else
    // For mono screens we expect a raw bitmap from the server (uncompressed)
    //disp.drawBitmap(payload, len); oops - doesn't handle rotations have to do this the hard way
    int height = disp.height(), width = disp.width();

    uint8_t pixmask = 0x80; // start at msb bit as left pixel
    Serial.printf("drawing mono pix w %d, height %d, len %d\n", width, height, len);
    for (int y = 0; y < height; y++)
        for (int x = 0; x < width; x++)
        {
            disp.drawPixel(x, y, (pixmask & *payload) ? BITMAP_WHITE : BITMAP_BLACK);
            pixmask >>= 1;
            if (!pixmask)
            { // we just finished a byte advance to next
                pixmask = 0x80;
                payload++;
            }
        }
#endif

    if (doUpdate)
    {
        // the eink display call can take a very long time (8 seconds), so we reset the sleep timer on both sides of the call (for now)
        delaySleep();
        DISPLAY_UPDATE;
        delaySleep();
    }
}

void handleShow(String persistenceIn, const char *url)
{
    // When a new board boots the "boot" image is sent from the server immediately after connect, this can create a race condition when our device almost
    // immediately sends a button push message.  To ensure we always set the boot message, we ignore new images from the server until that image has been written to flash
    if (haveNewImage && persistence == "boot")
    {
        Serial.println("Ignoring new image till boot.jpg is written");
        return;
    }

    http.setReuse(true); // try to keep our connection alive for fast response
    http.begin(clientForHttp, url);
    int httpCode = http.GET();
    Serial.printf("HTTP get %d\n", httpCode);
    if (httpCode == HTTP_CODE_OK && http.getSize() > 0)
    {
        // String payload = http.getString(); // This string can be enormous
        int len = http.getSize();
        ByteStream *stream = new ByteStream(len);
        int numWritten = http.writeToStream(stream);
        if (numWritten != len)
        {
            Serial.printf("ERROR http respons had wrong byte count, wr %d, len %d", numWritten, len);
            delete stream;
        }
        else
        {
            // Success! The main thread will now deal with this image
            persistence = persistenceIn;

            if (persistence == "forever")
            {
                // Keep this buffer around because we might reuse it
                if (foreverImage)
                    delete foreverImage;
                foreverImage = stream;
            }
            else // note: any special handling of different types of persistence is done in the main thread
            {
                if (tempImage)
                    delete tempImage;
                tempImage = stream;
            }
            haveNewImage = true; // ask main thread to show it
        }
    }
    http.end(); // free buffers
}

// The server says we are now on a new channel - tell the user
void handleShowMessage(const char *channelName)
{
}

void handleUpgrade(const char *urlBase)
{
    // We add our board type to the base string, so that publishing is less board specific
    //AutoUpdate netUpdate(String(urlBase) + "-" + JOYBOARD_TYPE);
    Serial.println("Note: ignoring URL in update message"); // until I think more carefully about security, don't allow this

    updateAtMillis = millis() + 5000;
    // we do the update in the loop thread, not this callback, because it might take a while and we don't want to sleep while it does
    // we are also careful to wait a few seconds so our ack of the message has time to get sent
}

void mqttCallback(char *topic, byte *payload, unsigned int length)
{

    payload[length] = '\0'; // Make sure it is nul terminated
    Serial.printf("Handle msg [%s] %s\n", topic, payload);

    delaySleep(); // We just got a message from our server, stay alive to handle it

    String topicStr = String(topic);
    String showPrefix = String(getTopic("show", "todev"));
    String channelPrefix = String(getTopic("showMessage", "todev"));
    String upgradePrefix = String(getTopic("newFirmware", "todev"));
    // parse /ezd/todev/SR2/show/forever
    if (topicStr.startsWith(showPrefix))
        handleShow(topicStr.substring(showPrefix.length() + 1), (const char *)payload);
    else if (topicStr.startsWith(channelPrefix))
        handleShowMessage((const char *)payload);
    else if (topicStr.startsWith(upgradePrefix))
        handleUpgrade((const char *)payload);
    else
        Serial.println("Error: unknown topic");
}

/**
   returns true if we reconnected
 */
bool reconnect()
{
    Serial.println("Calling MQTT server");

    // we send status offline as our will (as a persistent message)
    if (mqtt.connect(clientId, "joyclient", "apes4cats", getTopic("status"), 1, true, "offline"))
    {
        Serial.printf("Connected to MQTT server, wifi=%s", WiFi.SSID().c_str());
        delaySleep(); // we just made some forward progress (note, we don't do this just because we _tried_ to connect)

        static char subsStr[64]; /* We keep this static because the mqtt lib
                                    might not be copying it */
        snprintf(subsStr, sizeof(subsStr), "/ezd/todev/%s/#", clientId);
        mqtt.subscribe(subsStr, 1); // we use qos 1 because we don't want to miss messages

        /*
            the status payload is four lines separated by newlines:
            status
            firmwareVersion: verstr  (sent from device in the status message)
            firmwareNum: integer  (sent from device in the status message)
            netinfo: the last network this device connected to for debugging (sent from device in the status message)
            */
        char statusbuf[64];
        snprintf(statusbuf, sizeof(statusbuf), "online\n%s\n%d\n%s", VERSION_STRING, VERSION_NUM, WiFi.SSID().c_str());
        publish("status", statusbuf, true);
        // do this _after_ subscribe to ensure we don't miss messages, it is important that this message marked as retained, so that if the server reboots (during development mainly) it sets the correct state when it comes up

        return true;
    }
    else
        return false;
}

/**
   Draw one cycle of the network problem graphic */
void drawNetworkAnimation()
{
    int maxy = disp.height(), maxx = disp.width();
    int xpos = maxx / 2 - 50, ypos = 40;

    delay(DRAW_ANIM_DELAY);
    disp.fillRect(xpos, ypos, 100, 100, BACKGROUND); // erase the wifi graphic
    delay(DRAW_ANIM_DELAY);
    disp.drawBitmap(xpos, ypos, wifi_1, 100, 100, FOREGROUND);
    delay(DRAW_ANIM_DELAY);
    disp.drawBitmap(xpos, ypos, wifi_2, 100, 100, FOREGROUND);
    delay(DRAW_ANIM_DELAY);
    disp.drawBitmap(xpos, ypos, wifi_3, 100, 100, FOREGROUND);
}

void showBootScreen(String msg)
{
    turnOnDisplay();
    disp.fillScreen(BACKGROUND);

    // Load the custom boot screen if possible
    ByteStream *bootImage = loadImage("/boot.jpg");
    if (bootImage)
    {
        showImageNow(*bootImage, false);
        delete bootImage;
    }
    else
    {
        disp.setTextColor(ACCENT);
        disp.setCursor(110, 20);
        //disp.setTextSize(2);
        disp.println("JoyFrame");
    }

    disp.setTextColor(FOREGROUND);
    disp.setTextWrap(false);

    // disp.setTextSize(1);
    int maxy = disp.height(), maxx = disp.width();

#if 0 // def TFT_CS // we no longer show version at boot
    // We don't show this on eink because the screen is tiny.
    disp.setCursor(5, maxy - 13);
    disp.setTextColor(FOREGROUND);
    disp.print(VERSION_STRING);
#endif

    Serial.printf("Boot msg: %s\n", msg.c_str());

    if (maxy >= 160)
    {
        disp.setCursor(5, maxy - 7);
        // disp.setTextSize(2);
        disp.print(msg);
    }

    if (!bootImage)
        drawNetworkAnimation();

    DISPLAY_UPDATE;
}

void dispTest()
{
    //disp.drawCornerTest();
    //delay(5000);

    uint8_t rotation = disp.getRotation();
    for (uint16_t r = 0; r < 4; r++)
    {
        disp.setRotation(r);
        disp.fillScreen(BACKGROUND);
        disp.fillRect(0, 0, 8, 8, FOREGROUND);
        disp.fillRect(disp.width() - 18, 0, 16, 16, FOREGROUND);
        disp.fillRect(disp.width() - 25, disp.height() - 25, 24, 24, FOREGROUND);
        disp.fillRect(0, disp.height() - 33, 32, 32, FOREGROUND);
        DISPLAY_UPDATE;
        delay(5000);
    }
    disp.setRotation(rotation); // restore
}

bool startCP(IPAddress ip)
{
    Serial.println("Config portal started, IP:" + WiFi.localIP().toString());
    showBootScreen(String("Config wifi at ") + portalConfig.apid);
    return true;
}

void setup()
{

#ifdef DISABLE_BROWNOUT
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector
#endif

    Serial.begin(115200);
    blinkStatus(); // Turn on the status LED asap so the user knows the board has power

    bootCount++;
    wakeCause = esp_sleep_get_wakeup_cause();
    wakeButtons = esp_sleep_get_ext1_wakeup_status();       // If one of these buttons is set it was the reason we woke
    if (wakeCause == ESP_SLEEP_WAKEUP_EXT1 && !wakeButtons) // we must have been using the 'all buttons rule for waking' to support busted boards, assume button one was pressed
        wakeButtons = ((uint64_t)1) << buttons.gpios[0];

    Serial.printf("booted, wake cause %d, buttons 0x%llx (boot count %d)\n", wakeCause, wakeButtons, bootCount);
    // FIXME ESP_SLEEP_WAKEUP_EXT1 button presses and ESP_SLEEP_WAKEUP_TIMER for idle timer

    initClientId();

    dump_partitions();

    delaySleep(); // Give a few seconds to try and get an internet connection
    Serial.println("joining wifi");

#ifdef PANICUPDATE_BUTTON
    pinMode(PANICUPDATE_BUTTON, INPUT);
    bool initialUpdateCheck = !digitalRead(PANICUPDATE_BUTTON); // 1 means not pressed
#endif

    // Init the filesystem early, because we might need to load a custom boot screen
    if (SPIFFS.begin(true))
    {
        Serial.println("SPIFFS files:");

        flashFS = &SPIFFS;
        File root = flashFS->open("/");
        File file = root.openNextFile();
        while (file)
        {
            if (file.isDirectory())
            {
                Serial.print("  DIR : ");
                Serial.println(file.name());
            }
            else
            {
                Serial.print("  FILE: ");
                Serial.print(file.name());
                Serial.print("\tSIZE: ");
                Serial.println(file.size());
            }
            file = root.openNextFile();
        }
    }

#if 0
    Serial.println("Begin display test");
    turnOnDisplay();
    dispTest();
    Serial.println("End display test");
#endif

    AutoConnectCredential ac(portalConfig.boundaryOffset);

#ifdef FACTORYRESET_BUTTON
    pinMode(FACTORYRESET_BUTTON, INPUT);
    if (!digitalRead(FACTORYRESET_BUTTON) && !wakeButtons)
    { // 1 means not pressed, but only if we were not waking from sleep for that press (i.e. we also had the reset button pressed)
        Serial.println("FACTORY RESET!");
        portalConfig.immediateStart = true; // show our connection webapp
        ac.del("geeksville");               // Don't accidentally ship devices with my home network SSID
    }
#endif

    for (int8_t i = 0; i < ac.entries(); i++)
    {
        struct station_config entry;
        ac.load(i, &entry);
        Serial.printf("Saved SSID: %s\n", entry.ssid);
    }

    if (ac.entries() == 0)
        portalConfig.immediateStart = true; // If there are no saved wifi networks, we must show the portal
    portalConfig.apid = String("Ezdevice-") + shortId;
    portalConfig.psk = ""; // require no password
    portalConfig.title = "EZDevice";
    portalConfig.hostName = String("ezdev") + shortId;
    // portalConfig.autoRise = true;
    // portalConfig.autoReconnect = true; we don't want this because the esp32 will have already tried to connect to our network
    portalConfig.portalTimeout = 10 * 60 * 1000; // If the user fails to setup in this time, go back to deep sleep
    portal.onDetect(startCP);
    //portalConfig.homeUri = "/_ac/config"; // doesn't work
    //portalConfig.bootUri = AC_ONBOOTURI_HOME;
    portal.config(portalConfig);

    // We try to connect to the regular wifi for an hour (essentially forever - because we will enter deep-sleep before then)
    portal.begin(NULL, NULL, 60 * 60 * 1000);

    delaySleep(); // we just got connected to wifi?, give time for something interesting to happen

#ifdef DEEPSLEEP_CHARGEWAKE
    // FIXME count wakes properly
    // we need to do this _after_ wifi init because we need wifi on to guarantee >45mA draw
    if (wakeCause == ESP_SLEEP_WAKEUP_TIMER)
    {
        if (sleepsToSkip)
        {
            Serial.println("Doing sleephack");
            // burn some power long enough to keep shitty charge controller from shutting down for good
            uint32_t start = millis();
            while (millis() - start < 300) // 500ms seems long enough, 100ms was not, 200ms was marginal, 300ms seems fine so far
                Serial.print('.');
            doDeepSleep();
        }
        else
            sleepsToSkip--;
    }
#endif

// If the user held down the panic update button (and is still holding it down, see if we can fetch an update from the server)
#ifdef PANICUPDATE_BUTTON
    bool stillWantUpdate = !digitalRead(PANICUPDATE_BUTTON);
    if (initialUpdateCheck && stillWantUpdate)
    {
        showBootScreen("Firmware update...");
        update.update(true); // If an update succeeds it will reboot
    }
#endif

    // wifiClient.setCACert(mosquitto_org_ca_cert);
    mqtt.setServer("devsrv.ezdevice.net", 1883);
    mqtt.setCallback(mqttCallback);

    buttons.setup();

    delaySleep(); // We will wait to get a message from the server before we go to sleep (hopefully, eventually we will just bail)
    Serial.println("done with setup");
}

void epdTest();

/** Send all of our push publishes - FIXME, allow variable polling intervals.  Currently we send once at boot */
void sendPushSources()
{
    static bool isFirstCheck = true;

    if (isFirstCheck)
    {
        int numSources = sizeof(pushSources) / sizeof(pushSources[0]);
        for (int i = 0; i < numSources; i++)
        {
            const PushSource &s = pushSources[i];

            String topic("push/");
            topic += s.name;

            int raw = analogRead(s.gpioNum);
            float val = raw * s.scaling + s.offset;
            String valStr = String(val);

            publish(topic.c_str(), valStr.c_str());
        }

        isFirstCheck = false;
    }
}

/**
   Check to see if the user just released a button or a button press was the reason we booted most recently
 */
void buttonCheck()
{
    static bool isFirstCheck = true;
    static const char *buttonNames[] = {
        "one", "two", "three", "four"}; // we also use a special button name "reset" if the board was just cold booted

    if (isFirstCheck && (wakeCause == ESP_SLEEP_WAKEUP_TIMER || wakeCause == ESP_SLEEP_WAKEUP_UNDEFINED))
    { // If we booted because our timer ran out or the user pressed reset, send those as fake events
        const char *reason = (wakeCause == ESP_SLEEP_WAKEUP_TIMER) ? "timeout" : "reset";

        publish(EVENT_TOPIC, reason);
        delaySleep(); // if someone presses anything, stay alive a little longer
    }
    else
    {
        bool hasHandledSomething = false;

        for (int i = 0; i < NUM_BUTTONS; i++)
        {
            bool wakePress = isFirstCheck && (wakeButtons & (((uint64_t)1) << buttons.gpios[i]));

            // If the user has pressed multiple buttons (because they were waiting for a slow eink draw we accept the first one and then ignore the others)
            // we still need to call handle() to mark that we are throwing away that press
            if ((buttons.handle(i) || wakePress) && !hasHandledSomething)
            {
                // either send press to server or use it to cancel the current image
                if (showingTempImage)
                {
                    if (foreverImage)
                        showImageNow(*foreverImage);

                    showingTempImage = false;
                }
                else
                    publish(EVENT_TOPIC, buttonNames[i]);
                // epdTest();
                delaySleep(); // if someone presses anything, stay alive a little longer
                hasHandledSomething = true;
            }
        }
    }

    isFirstCheck = false; // we only check wake buttons on first call of this function
}

// Our network handler has requested the main thread to show an image
void showNewImages()
{
    if (haveNewImage)
    {
        haveNewImage = false;
        if (persistence == "boot")
        {
            saveImage("/boot.jpg", *tempImage); // may not really be a jpeg but whatever...
            delete tempImage;
            tempImage = NULL;
        }
        else if (persistence != "forever")
        {
            showImageNow(*tempImage);
            showingTempImage = true; // changes button behaviors
            delete tempImage;
            tempImage = NULL;
        }
        else if (foreverImage)
        { // this should always be true here but just in case
            showImageNow(*foreverImage);
            showingTempImage = false;
        }
    }
}

void loop()
{
    static uint32_t startMsec = millis();
    unsigned long currentMillis = millis();
    static bool firstAttempt = true;

    portal.handleClient();

    perhapsSleep();

    showNewImages();

    if (mqtt.connected())
    { // We only check for buttons when we are connected and subscribed for responses
        buttonCheck();
        sendPushSources();
    }

    if (currentMillis - previousMillis >= interval)
    {
        previousMillis = currentMillis;
        interval = 500;
        blinkStatus();

        if ((WiFi.status() == WL_CONNECTED))
        {
            //Note: we only check for update requests occasionally so that the MQTT server has a chance to receive the ack that we received the request
            if (updateAtMillis && updateAtMillis >= currentMillis)
            {
                updateAtMillis = 0;
                update.update(true); // Force an update to this revision
            }

            if (!mqtt.connected())
            { // If our first connection fails show a message
                if (!reconnect() && firstAttempt)
                {
                    showBootScreen("Looking for our servers");
                    firstAttempt = false;
                }
                else
                {
                    Serial.printf("Time required for server connect: %lu\n", millis() - previousMillis);
                    portal.end(); // No need to keep burning CPU for our setup portal
                }
            }
        }
        else
        {
            // we try to wait up to 5 seconds before deciding we need a boot screen - to tell the user something is wrong
            static bool didShowWifiMessage = false;

            if (millis() - startMsec >= 5000 && !didShowWifiMessage)
            {
                didShowWifiMessage = true; // Only show the message once to keep eink from flickering
                showBootScreen(String("Looking for ") + WiFi.SSID());
            }
        }
    }

    mqtt.loop();
}
