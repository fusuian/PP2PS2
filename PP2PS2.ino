// PP2PS2: SideWinder Precision Pro to PlayStation2 Adapter
//
// Copyright (c) 2019 ASAHI,Michiharu
//
// Based on code by Copyright (c) 2015 Kazumasa ISE
// Released under the MIT license
// http://opensource.org/licenses/mit-license.php

#include <SPI.h>
#include <util/delay.h>
#include "PrecisionPro.h"
#include "PWM.h"
//#define DEBUG
#include "PP2DS2Talker.h"

// the center threshold of analog stick (x and y axis)
const int stick_center_threshold = 16;


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

PP2DS2Talker * ds2talker;

void oneclock() {
    portOn(A5);
    *pdata++ = PIN(mosi_pin);
    portOff(A5);
}




void setup() {
    Serial.begin(115200);
    pinMode(A5, OUTPUT);
    pinMode(left_lite_pin, OUTPUT);
    pinMode(right_lite_pin, OUTPUT);

    pp = new PrecisionPro(trigger_pin, mosi_pin, sck_pin);
    pp->init();
    ds2talker = new PP2DS2Talker(pp);
    ds2talker->setup();
}




void loop() {
    if (clock_msec <= micros() && read_pp == false) {
        pp->update();
        read_pp = true;
#if 0
        Serial.print(pp->x());
        Serial.print(", ");
        Serial.print(pp->y());
        Serial.println("");
#endif

        int x = (pp->x() / 4) - 0x80;
        if (abs(x) < stick_center_threshold) { x = 0; }
        if (x == 0) {
            ds2talker->set_right_key(0);
            ds2talker->set_left_key(0);
        } else if (x > 0) {
            ds2talker->set_right_key(x);
        } else {
            ds2talker->set_left_key(-x);
        }

        int y = (pp->y() / 4) - 0x80;
        if (abs(y) < stick_center_threshold) { y = 0; }
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
