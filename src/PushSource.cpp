#include <Arduino.h>
#include "PushSource.h"
#include "service.h"

void PushSource::doPublish()
{
    String topic("push/");
    topic += name;

    publish(topic, getValueStr());
}

String AnalogPushSource::getValueStr()
{
    int raw = analogRead(gpioNum);
    float val = raw * scaling + offset;
    String valStr = String(val);

    return valStr;
}