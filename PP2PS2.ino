// PP2PS2: SideWinder Precision Pro to PlayStation2 Adapter
//
// Based on code by Copyright (c) 2015 Kazumasa ISE
// Released under the MIT license
// http://opensource.org/licenses/mit-license.php
#include <SPI.h>
#include <util/delay.h>
#include "FastRunningMedian.h"
#include "PrecisionPro.h"
#include "PWM.h"
//#define DEBUG
#include "DualShock2Talker.h"

#define DS_SELECT   pp->d()
#define DS_START    pp->a()

#define DS_CROSS  pp->fire()
#define DS_CIRCLE  pp->top()
#define DS_TRIANGLE  pp->top_up()
#define DS_SQUARE  pp->top_down()

#define DS_L1  pp->c()
#define DS_R1  pp->b()
#define DS_L2  0 // pp->d()
#define DS_R2  0 // pp->a()



// Arduino Nano
int sck_pin  = 2;  // 割り込みピン(2 or 3)であること！
int mosi_pin = 4;
int trigger_pin = 5;

const byte left_lite_pin  = A0;
const byte right_lite_pin = A1;


PrecisionPro * pp;
unsigned long clock_msec = 0;
bool read_pp;
const byte buflen = 15;
FastRunningMedian<int, buflen, 0> xMedian;
FastRunningMedian<int, buflen, 0> yMedian;

class PP2DS2Talker : public DualShock2Talker
{
  public:
  PP2DS2Talker(){}
  ~PP2DS2Talker(){}
  
  byte sw1()
  {
    byte hat = pp->hat_switch();
    byte up = (hat == 1) || (hat == 2) || (hat == 8) || up_key.update();
    byte right = (hat >= 2) && (hat <= 4) || right_key.update();
    byte down = (hat >= 4) && (hat <= 6) || down_key.update();
    byte left = (hat >= 6) && (hat <= 8) || left_key.update();
    return ~(DS_SELECT | (DS_START << 3) |
      (up << 4) | (right << 5) | (down << 6) | (left << 7) );
  }


  byte sw2()
  {
    return ~(DS_L2 | (DS_R2 << 1) | (DS_L1 << 2) | (DS_R1 << 3) |
      (DS_TRIANGLE << 4) | (DS_CIRCLE << 5) | (DS_CROSS << 6) | (DS_SQUARE << 7));
  }


  void set_up_key(byte value) { up_key.set_value(value); }
  void set_down_key(byte value) { down_key.set_value(value); }
  void set_left_key(byte value) { left_key.set_value(value); }
  void set_right_key(byte value) { right_key.set_value(value); }

private:
  PWM up_key;
  PWM down_key;
  PWM left_key;
  PWM right_key;


};

PP2DS2Talker * ds2talker;

void oneclock()
{
  portOn(A5);
  *pdata++ = PIN(mosi_pin);
  portOff(A5);
}




void setup() {
    Serial.begin(115200);
    pinMode(A5, OUTPUT);
    pinMode(left_lite_pin, OUTPUT);
    pinMode(right_lite_pin, OUTPUT);

    ds2talker = new PP2DS2Talker();
    ds2talker->setup();
    pp = new PrecisionPro(trigger_pin, mosi_pin, sck_pin);
    pp->init();
}




const int threshold = 16;

void loop() {
  if (clock_msec <= micros() && read_pp == false) {
    pp->update();
    read_pp = true;
    
    Serial.print(pp->x());
    Serial.print(", ");
    Serial.print(pp->y());
    Serial.println("");

    int x = (pp->x() / 4) - 0x80;
//    xMedian.addValue(x);
//    x = xMedian.getMedian();
    if (abs(x) < threshold) { x = 0; }
    if (x == 0) {
        ds2talker->set_right_key(0);
        ds2talker->set_left_key(0);
    } else if (x > 0) {
        ds2talker->set_right_key(x);
    } else {
        ds2talker->set_left_key(-x);
    }

    int y = (pp->y() / 4) - 0x80;
//    yMedian.addValue(y);
//    y = yMedian.getMedian();
    if (abs(y) < threshold) { y = 0; }
    if (y == 0) {
        ds2talker->set_up_key(0);
        ds2talker->set_down_key(0);
    } else if (y > 0) {
        ds2talker->set_down_key(y);
    } else {
        ds2talker->set_up_key(-y);
    }

    portWrite(left_lite_pin, DS_CIRCLE);
    portWrite(right_lite_pin, DS_CROSS);
  }
#if 0
    volatile sw_data_t & sw_data = pp->data();
    for (int i=0; i < 6; i++){
        unsigned char b = sw_data.buf[i];
        if (b < 16) {
          Serial.print("0");
        }
        Serial.print(b, HEX);
        Serial.print(":");
    }

    Serial.print("  btns:");
    Serial.print(sw_data.btn_fire);
    Serial.print(sw_data.btn_top);
    Serial.print(sw_data.btn_top_up);
    Serial.print(sw_data.btn_top_down);
    Serial.print(":");

    Serial.print(sw_data.btn_a);
    Serial.print(sw_data.btn_b);
    Serial.print(sw_data.btn_c);
    Serial.print(sw_data.btn_d);

    Serial.print("; (");
    Serial.print(sw_data.x/4);
    Serial.print(", ");
    Serial.print(sw_data.y/4);
    Serial.print("); rudder=");
    Serial.print(sw_data.r*4);
    Serial.print("; throttle=");
    Serial.print(sw_data.m*2);
    Serial.print("; HAT=");
    Serial.print(sw_data.head);
    Serial.println();
#endif
#ifdef DEBUG
  ds2talker_debug();
#endif
}


#ifdef DEBUG
#define LINE_FEED 0xAA
#define MAX_LOG_SIZE 60
volatile byte cmdLog[MAX_LOG_SIZE] = {0};
volatile byte datLog[MAX_LOG_SIZE] = {0};
volatile int logCount = 0;
#endif


ISR(SPI_STC_vect) {
    static byte ID = 0x41;
    static byte CMD[CMD_BYTES] = {0};
    static byte cmdCount = 0;
    bool continueCom = false;
    CMD[cmdCount] = SPDR;
#ifdef DEBUG
    if (logCount < MAX_LOG_SIZE) {cmdLog[logCount++] = CMD[cmdCount];}
#endif
    const byte numOfCmd = 3+2*(ID & 0x0F);
    // Check CMD
    if (cmdCount == 0) {
        if (CMD[cmdCount] == 0x01) {
            continueCom = true;
        }
    } else if (cmdCount == 1) {
        if (CMD[cmdCount] == 0x01) {
            cmdCount = 0; // Reset count
            continueCom = true;
        } else if ((CMD[cmdCount] & 0x40) == 0x40) {
            continueCom = true;
        }
    } else if (cmdCount == 5) {
        if ((CMD[1] == READ_DATA) && (CMD[cmdCount] == 0x01)) {
            cmdCount = 0; // Reset count
        }
        continueCom = true;
    } else if (cmdCount < numOfCmd-1) {
        continueCom = true;
    }
#ifdef DEBUG
    if (!continueCom && (logCount < MAX_LOG_SIZE)) {cmdLog[logCount++] = LINE_FEED;}
#endif
    // Set next DAT
    cmdCount = continueCom ? cmdCount+1 : 0;
    const byte DAT = ds2talker->dat(CMD, cmdCount);
    if (cmdCount == 1) {ID = DAT;}
    SPDR = DAT;
#ifdef DEBUG
    if (logCount < MAX_LOG_SIZE) {datLog[logCount] = DAT;}
#endif
    if (continueCom) {
      ds2talker->acknowledge();
    } else {
      clock_msec = micros() + 8000;
      read_pp = false;
    }
}
