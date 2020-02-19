// PWM.h
//
// Copyright (c) 2019-2020 ASAHI,Michiharu
//
// Released under the MIT license
// see http://opensource.org/licenses/mit-license.php

#ifndef _PWM_H_
#define _PWM_H_

#include <Arduino.h>

class PWM
{
public:
    PWM(): value_buf(0){}
    virtual ~PWM(){}

    void set_value(byte value) { this->value = value; }
    byte update() {
        value_buf += value;
        if (value_buf > 128) {
            value_buf -= 128;
            return 1;
        }
        return 0;
    }

private:
    byte value;
    uint16_t value_buf;

};


#endif //_PWM_H_
