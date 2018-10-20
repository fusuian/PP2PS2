#ifndef _PWM_H_
#define _PWM_H_

#include <Arduino.h>

class PWM
{
public:
    PWM(): value_buf(0){}
    virtual ~PWM(){}

    void set_value(byte value) { this->value = value; }
    byte update();

private:
    byte value;
    uint16_t value_buf;

};


#endif //_PWM_H_
