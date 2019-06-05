#pragma once

#include <Arduino.h>

// Sources for periodic publishes of device state.
// generalize via inheritence to allow other input types or use a virtual function to read special value types
class PushSource
{
    const char *name; // source will be published with this identifier

public:
    PushSource(const char *_name) : name(_name) {}

    virtual void doPublish();

protected:
    virtual String getValueStr() = 0;
};

// Initially assume an analog input
class AnalogPushSource : public PushSource {
    int gpioNum;
    float scaling; // Used to prescale the read value before uploading
    float offset;  // uploaded value is rawValue * scale + offset

public:
    AnalogPushSource(const char *_name, int _gpioNum, float _scaling, float _offset) 
    : PushSource(_name)
    , gpioNum(_gpioNum)
    , scaling(_scaling)
    , offset(_offset) {}

protected:
    virtual String getValueStr();
};

class DHTTempSource : public PushSource {
public:
    DHTTempSource() : PushSource("Tamb") {}   

protected:
    virtual String getValueStr(); 
};

class DHTHumiditySource : public PushSource {
public:
    DHTHumiditySource() : PushSource("Hum") {}   

protected:
    virtual String getValueStr(); 
};

class BMETempSource : public PushSource {
public:
    BMETempSource() : PushSource("Tamb") {}   

protected:
    virtual String getValueStr(); 
};

class BMEHumiditySource : public PushSource {
public:
    BMEHumiditySource() : PushSource("Hum") {}   

protected:
    virtual String getValueStr(); 
};

class BMEPressureSource : public PushSource {
public:
    BMEPressureSource() : PushSource("hPa") {}   

protected:
    virtual String getValueStr(); 
};