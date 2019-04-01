#include "OV2640.h"
#include "board.h"
#include <HTTPClient.h>

#ifdef CAM_CONFIG

OV2640 cam;
WiFiClient camUploadClient;
HTTPClient httpUpload;

void camSetup()
{
    // FIXME - causes a dereferernced null ptr in something called from esp_intr_get_cpu gpio_isr_handler_add camera_init
    // https://github.com/espressif/esp32-camera/blob/master/driver/camera.c#L1043
    // there is speculating on the net there is some sort of conflict between i2s interrupts and gpio interrupts, if both on there are problems.
    // For now my fix is to not use interrupts for GPIO detection on boards with cameras.
    // CAM_CONFIG.fb_count = 1; // we don't need the fast DMA version? - it sucks power
    int res = cam.init(CAM_CONFIG);
    Serial.printf("Camera init returned %d\n", res);
}

// Called from the server to take a new frame
// we should take the pict and PUT it to the specified URL
String camSnapshot(String arguments)
{
    int result = 0;
    int argsplit = arguments.indexOf(' ');

    if (argsplit <= 0)
    {
        Serial.println("Invalid snapshot arguments");
        result = -3;
    }
    else
    {
        String destURL = arguments.substring(0, argsplit);
        int numFrames = arguments.substring(argsplit + 1).toInt();

        Serial.printf("Beginning snapshot %d\n", numFrames); // FIXME, support multiple frames, by manually stuffing each frame in the PUT see:
        // https://github.com/espressif/arduino-esp32/blob/master/libraries/HTTPClient/src/HTTPClient.cpp#L509

        cam.run(); // read the next frame (this frame might have been acquired LONG ago, so for now we just discard it and read a new fresh frame)
        cam.run(); // This frame should reflect recent history

        uint8_t *bytes = cam.getfb();
        size_t numbytes = cam.getSize();
        if (bytes && numbytes)
        {
            httpUpload.begin(camUploadClient, destURL);
            httpUpload.addHeader("Content-Type", numFrames > 1 ? "video/x-motion-jpeg" : "image/jpeg");

            result = httpUpload.PUT(bytes, numbytes);
            Serial.printf("Frame uploaded, result %d\n", result);

            httpUpload.end();
        }
        else
        {
            Serial.println("FIXME - unexpected null framebuffer");
            result = -2;
        }
    }

    return String(result);
}

#endif
