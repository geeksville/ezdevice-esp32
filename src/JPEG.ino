#include <JPEGDecoder.h>
#include "board.h"
#include <GxEPD.h>

// Return the minimum of two values a and b
#define minimum(a,b)     (((a) < (b)) ? (a) : (b))


//====================================================================================
//   Decode and paint onto the TFT screen
//====================================================================================
void jpegRender(int xpos, int ypos) {

    // retrieve infomration about the image
    uint16_t  *pImg;
    uint16_t mcu_w = JpegDec.MCUWidth;
    uint16_t mcu_h = JpegDec.MCUHeight;
    uint32_t max_x = JpegDec.width;
    uint32_t max_y = JpegDec.height;

    // Jpeg images are draw as a set of image block (tiles) called Minimum Coding Units (MCUs)
    // Typically these MCUs are 16x16 pixel blocks
    // Determine the width and height of the right and bottom edge image blocks
    uint32_t min_w = minimum(mcu_w, max_x % mcu_w);
    uint32_t min_h = minimum(mcu_h, max_y % mcu_h);

    // record the current time so we can measure how long it takes to draw an image
    uint32_t drawTime = millis();

    // save the coordinate of the right and bottom edges to assist image cropping
    // to the screen size
    max_x += xpos;
    max_y += ypos;

    // read each MCU block until there are no more
    while ( JpegDec.read()) {

        // save a pointer to the image block
        pImg = JpegDec.pImage;

        // calculate where the image block should be drawn on the screen
        int mcu_x = JpegDec.MCUx * mcu_w + xpos;
        int mcu_y = JpegDec.MCUy * mcu_h + ypos;

        // check if the image block size needs to be changed for the right edge
        uint32_t win_w;
        if (mcu_x + mcu_w <= max_x) win_w = mcu_w;
        else win_w = min_w;

        // check if the image block size needs to be changed for the bottom edge
        uint32_t win_h;
        if (mcu_y + mcu_h <= max_y) win_h = mcu_h;
        else win_h = min_h;

#ifdef DISP_ADAFRUIT
// copy pixels into a contiguous block
        if (win_w != mcu_w)
        {
            for (int h = 1; h < win_h-1; h++)
            {
                memcpy(pImg + h * win_w, pImg + (h + 1) * mcu_w, win_w << 1);
            }
        }

        // FIXME - implement for eink
        // draw image MCU block only if it will fit on the screen
        if ( ( mcu_x + win_w) <= disp.width() && ( mcu_y + win_h) <= disp.height())
        {
            // pImg is 565 RGB pixels
            disp.drawRGBBitmap(mcu_x, mcu_y, pImg, win_w, win_h);
        }

        // Stop drawing blocks if the bottom of the screen has been reached,
        // the abort function will close the file
        else if ( ( mcu_y + win_h) >= disp.height()) JpegDec.abort();
#else
        // a slow but simple version that works with eInk

        // calculate how many pixels must be drawn
        // uint32_t mcu_pixels = win_w * win_h;

        // draw image MCU block only if it will fit on the screen
        if ( ( mcu_x + win_w) <= disp.width() && ( mcu_y + win_h) <= disp.height())
        {
            //  disp.startWrite();

            // Now set a MCU bounding window on the TFT to push pixels into (x, y, x + width - 1, y + height - 1)
            // disp.setAddrWindow(mcu_x, mcu_y, win_w, win_h);

            // Write all MCU pixels to the TFT window
            //while (mcu_pixels--) disp.pushColor(*pImg++); // Send MCU buffer to TFT 16 bits at a time
            for(int y = 0; y < win_h; y++)
                for(int x = 0; x < win_w; x++) {
                    uint16_t p = pImg[x + y * mcu_w];

                    p &= 0x1f; // just keep the blue channel for this crude transform
                    // for now we assume server converted to greyscale

                    uint16_t pout;
                    // GxEPD_DARKGREY / GxEPD_LIGHTGREY not implemented on this screen
                    if(p >= 16)
                        pout = GxEPD_WHITE;
                    else
                        pout = GxEPD_BLACK;

                    disp.drawPixel(mcu_x + x, mcu_y + y, pout);
                }
            //  disp.endWrite();
        }

        // stop drawing blocks if the bottom of the screen has been reached
        // the abort function will close the file
        else if ( ( mcu_y + win_h) >= disp.height()) JpegDec.abort();
#endif
    }

    // after drawing the full frame do the slow eink update
    DISPLAY_UPDATE;

    // calculate how long it took to draw the image
    drawTime = millis() - drawTime;

    // print the results to the serial port
    Serial.print  ("Total render time was    : "); Serial.print(drawTime); Serial.println(" ms");
    Serial.println("=====================================");

}
