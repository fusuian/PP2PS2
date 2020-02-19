#ifndef _PRECISIONPRO_
#define _PRECISIONPRO_

#include "portmacro.h"

#include "sw_data_t.h"

volatile byte reg_data[50];
volatile byte *pdata;
void oneclock();



class PrecisionPro
{
  private:
    int mosi_pin;
    int sck_pin;
    int trigger_pin;

    volatile sw_data_t sw_data;
    volatile byte pos;

  public:

  PrecisionPro(int pin_trigger, int mosi, int sck):
    trigger_pin(pin_trigger), mosi_pin(mosi), sck_pin(sck){}


  void init() {
#ifdef DEBUG
    if ((sck_pin == 2 || sck_pin == 3) == false) {
      Serial.println(F("error!: sck_pin must be 2 or 3"));
      while (1) ;
    }
#endif

    pinMode(trigger_pin, OUTPUT);
    portOn(trigger_pin);
    pinMode(mosi_pin, INPUT_PULLUP);
    pinMode(sck_pin, INPUT_PULLUP);
    attachInterrupt(sck_pin - 2, oneclock, RISING);
  }


  void update() {
    pdata = reg_data;
    portOff(trigger_pin);
    delayMicroseconds(1000);

    pos = 0;
    portOn(trigger_pin);
    delayMicroseconds(1000); // この間に割り込みでスティックからのデータを読んでいる

    byte * pd = reg_data+1;
    byte spi;
    for (int i = 0; i < 6; i++) {
      spi = 0;
      for (int j = 0; j < 8; j++, pd++) {
        int b = (*pd & _BV(REG(mosi_pin))) != 0;
        spi |= b << j;
      }
      add_buf(spi);
    }

  }


  void add_buf(byte spdr) {
    sw_data.buf[pos++] = spdr;
  }

  volatile sw_data_t & data() {
    return sw_data;
  }

  bool fire() {
    return sw_data.btn_fire == 0;
  }

  bool top() {
    return sw_data.btn_top == 0;
  }

  bool top_up() {
    return sw_data.btn_top_up == 0;
  }

  bool top_down() {
    return sw_data.btn_top_down == 0;
  }

  bool shift() {
    return sw_data.btn_shift == 0;
  }

  bool a() {
    return sw_data.btn_a == 0;
  }

  bool b() {
    return sw_data.btn_b == 0;
  }

  bool c() {
    return sw_data.btn_c == 0;
  }

  bool d() {
    return sw_data.btn_d == 0;
  }


  int hat_switch() {
    return sw_data.head;
  }

  // 10bits Value (0-1023)
  int x() {
    return sw_data.x;
  }

  // 10bits Value (0-1023)
  int y() {
    return sw_data.y;
  }

  // 7bits Value (0-127)
  int throttle() {
    return sw_data.m;
  }

  // 6bits Value (0-63)
  int rudder() {
    return sw_data.r;
  }

};

#endif
