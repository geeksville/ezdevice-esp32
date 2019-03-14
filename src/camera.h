#pragma once

#include "Arduino.h"
#include "board.h"

#ifdef CAM_CONFIG

void camSetup();

// Called from the server to take a new frame
// we should take the pict and PUT it to the specified URL
String camSnapshot(String destURL);

#endif