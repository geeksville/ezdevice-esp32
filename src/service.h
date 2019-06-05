#pragma once

#include <Arduino.h>

// placeholder for moving all the server comms into one file

void publish(String suffix, String payload, bool retained = false);
