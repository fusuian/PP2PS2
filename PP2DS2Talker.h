// PP2PS2Talker: SideWinder Precision Pro to PlayStation2 Adapter
//
// Copyright (c) 2019-2020 ASAHI,Michiharu
//
// Released under the MIT license
// see http://opensource.org/licenses/mit-license.php

#ifndef _PP2DS2TALKER_H_
#define _PP2DS2TALKER_H_

#include "DualShock2Talker.h"
#include "PrecisionPro.h"

// DualShock - Precision Pro buttons bindings
#define DS_SELECT   pp->d()
#define DS_START    pp->a()

#define DS_CROSS    pp->fire()
#define DS_CIRCLE   pp->top()
#define DS_TRIANGLE pp->top_up()
#define DS_SQUARE   pp->top_down()

#define DS_L1       pp->c()
#define DS_R1       pp->b()
#define DS_R2       0 // pp->a()
#define DS_L2       0 // pp->d()


class PP2DS2Talker : public DualShock2Talker
{
  public:
    PP2DS2Talker(PrecisionPro * pp){ this->pp = pp; }
    ~PP2DS2Talker(){}

    byte sw1() {
      byte hat = pp->hat_switch();
      byte up = (hat == 1) || (hat == 2) || (hat == 8) || up_key.update();
      byte right = (hat >= 2) && (hat <= 4) || right_key.update();
      byte down = (hat >= 4) && (hat <= 6) || down_key.update();
      byte left = (hat >= 6) && (hat <= 8) || left_key.update();
      return ~(DS_SELECT | (DS_START << 3) |
        (up << 4) | (right << 5) | (down << 6) | (left << 7) );
    }


    byte sw2() {
        return ~(DS_L2 | (DS_R2 << 1) | (DS_L1 << 2) | (DS_R1 << 3) |
          (DS_TRIANGLE << 4) | (DS_CIRCLE << 5) | (DS_CROSS << 6) | (DS_SQUARE << 7));
    }


    void set_up_key(byte value) { up_key.set_value(value); }
    void set_down_key(byte value) { down_key.set_value(value); }
    void set_left_key(byte value) { left_key.set_value(value); }
    void set_right_key(byte value) { right_key.set_value(value); }

  private:
    PrecisionPro * pp;

    PWM up_key;
    PWM down_key;
    PWM left_key;
    PWM right_key;
};


#endif
