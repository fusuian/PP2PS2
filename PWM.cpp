#include "PWM.h"


byte
PWM::update()
{
    value_buf += value;
    if (value_buf > 128) {
        value_buf -= 128;
        return 1;
    }
    return 0;
}
