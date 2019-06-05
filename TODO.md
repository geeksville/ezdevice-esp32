## Use PWM to control the LED brightness for the display backlight LED

Something like:
// Init the back-light LED PWM
ledcSetup(BLK_PWM_CHANNEL, 10000, 8);
ledcAttachPin(TFT_BL, BLK_PWM_CHANNEL);
ledcWrite(BLK_PWM_CHANNEL, 80);

