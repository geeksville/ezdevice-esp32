#include "OV2640.h"
#include "board.h"
#include <HTTPClient.h>

#ifdef CAM_CONFIG

OV2640 cam;
WiFiClient camUploadClient;

void camSetup()
{
    // FIXME - causes a dereferernced null ptr in something called from esp_intr_get_cpu gpio_isr_handler_add camera_init
    // https://github.com/espressif/esp32-camera/blob/master/driver/camera.c#L1043
    // there is speculating on the net there is some sort of conflict between i2s interrupts and gpio interrupts, if both on there are problems.
    // For now my fix is to not use interrupts for GPIO detection on boards with cameras.
    int res = cam.init(CAM_CONFIG);
    Serial.printf("Camera init returned %d\n", res);
}

// Called from the server to take a new frame
// we should take the pict and PUT it to the specified URL
String camSnapshot(String destURL)
{
    HTTPClient http;
    http.begin(camUploadClient, destURL);
    http.addHeader("Content-Type", "image/jpeg");

    cam.run(); // queue up a read for next time

    int result = http.PUT(cam.getfb(), cam.getSize());
    Serial.printf("Frame uploaded, result %d\n", result);
    http.end();

    return String(result);
}

#endif

