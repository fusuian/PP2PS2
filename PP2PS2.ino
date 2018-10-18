// SPI test PS2: Master, Arduino: Slave (ANALOG MODE)
//
// Copyright (c) 2015 Kazumasa ISE
// Released under the MIT license
// http://opensource.org/licenses/mit-license.php
#include "PrecisionPro.h"

#include <SPI.h>
#include <util/delay.h>
#define DEBUG
#define ACK_WAIT 0.5
#define ACK 9
#define SET_ACK_LOW (PORTB &= ~B00000010)
#define SET_ACK_HIGH (PORTB |= B00000010)

#define SW1 (PIND | B00000111)
#define SW2 ((PINC << 2) | B00001111)

#define READ_DATA 0x42
#define CONFIG_MODE 0x43
#define SET_MODE_AND_LOCK 0x44
#define QUERY_MODEL_AND_MODE 0x45
#define UNKNOWN_COMMAND_46 0x46
#define UNKNOWN_COMMAND_47 0x47
#define UNKNOWN_COMMAND_4C 0x4C
#define VIBRATION_ENABLE 0x4D
#define CMD_BYTES 9
volatile bool isAnalogMode = false;
volatile bool isConfigMode = false;
volatile bool unknownFlag = false;
volatile byte RH = 0x80;
volatile byte RV = 0x80;
volatile byte LH = 0x80;
volatile byte LV = 0x80;

#if 1
// Arduino Nano
int mosi_pin = 3;
int sck_pin  = 2;  // 割り込みピン(2 or 3)であること！
int trigger_pin = 4;
int check_pin = 5;
int clear_pin = 5;
int ss_pin = 5;
#else
// Arduino Pro Mini
int mosi_pin = 9;
int sck_pin  = 2;  // 割り込みピン(2 or 3)であること！
int trigger_pin = 8;
int check_pin = 5;
int clear_pin = 5; // ソフトウェア読み取りでは使わない
int ss_pin = 5; // ソフトウェア読み取りでは使わない
#endif


PrecisionPro * pp;
unsigned long clock_msec = 0;
bool read_pp;

void oneclock()
{
  *pdata++ = PIN(mosi_pin);
}




void setup() {
    // SPI setup
    SPI.setBitOrder(LSBFIRST);
    SPI.setDataMode(SPI_MODE3);
    SPCR &= ~(_BV(MSTR)); // Set as Slave
    SPCR |= _BV(SPE); // Enable SPI
    pinMode(SS, INPUT);
    pinMode(MOSI, INPUT);
    pinMode(MISO, OUTPUT);
    digitalWrite(MISO, HIGH);
    pinMode(SCK, INPUT);
    SPI.attachInterrupt();
    // ACK pin setup
    pinMode(ACK, OUTPUT);
    digitalWrite(ACK, HIGH);
#ifdef DEBUG
    Serial.begin(57600);
#endif

    pp = new PrecisionPro(trigger_pin, clear_pin, mosi_pin, sck_pin, ss_pin);
    pp->init();
//    digitalWrite(PIN_CLEAR, HIGH);
}

#define DS_SELECT   pp->c()
//(pp->shift() & pp->top_up())
#define DS_START    pp->b()
//(pp->shift() & pp->top_down())
#define DS_CROSS  pp->fire()
#define DS_CIRCLE  pp->top()
#define DS_TRIANGLE  pp->top_up()
#define DS_SQUARE  pp->top_down()


#define DS_L1  pp->d()
#define DS_L2  0 //pp->d()
#define DS_R1  pp->a()
#define DS_R2  0 //pp->a()

inline byte sw1()
{
  byte hat = pp->hat_switch();
  byte up = (hat == 1) || (hat == 2) || (hat == 8);
  byte right = (hat >= 2) && (hat <= 4);
  byte down = (hat >= 4) && (hat <= 6);
  byte left = (hat >= 6) && (hat <= 8);
  return ~(DS_SELECT | (DS_START << 3) |
    (up << 4) | (right << 5) | (down << 6) | (left << 7) );
}


inline byte sw2()
{
  return ~(DS_L2 | (DS_R2 << 1) | (DS_L1 << 2) | (DS_R1 << 3) |
    (DS_TRIANGLE << 4) | (DS_CIRCLE << 5) | (DS_CROSS << 6) | (DS_SQUARE << 7));
}


inline byte readDataResponse(byte i) {
    const byte DAT[] = {0xFF, 0x41, 0x5A, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    switch (i) {
    case 1: return isConfigMode ? 0xF3 : (isAnalogMode ? 0x73 : 0x41);
    case 3: return sw1();
    case 4: return sw2();
    case 5: 
      return pp->rudder()*4; //RH;
    case 6: 
      return pp->throttle()*2; //RV;
    case 7: 
      return pp->x()/4; // LH
    case 8: 
      return pp->y()/4; // LV
    default: 
      return DAT[i];
    }
}


inline byte setModeAndLockResponse(byte i) {
    const byte DAT[] = {0xFF, 0xF3, 0x5A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    return DAT[i];
}


inline byte queryModelAndModeResponse(byte i) {
    const byte DAT[] = {0xFF, 0xF3, 0x5A, 0x01, 0x02, 0x00, 0x02, 0x01, 0x00};
    switch (i) {
    case 5: return isAnalogMode ? 0x01 : 0x00;
    default: return DAT[i];
    }
}


inline byte unknownCommand46Response(byte i) {
    const byte DAT[] = {0xFF, 0xF3, 0x5A, 0x00, 0x00, 0x01, 0x02, 0x00, 0x0A};
    switch (i) {
    case 6: return unknownFlag ? 0x01 : 0x02;
    case 7: return unknownFlag ? 0x01 : 0x00;
    case 8: return unknownFlag ? 0x14 : 0x0A;
    default: return DAT[i];
    }
}


inline byte unknownCommand47Response(byte i) {
    const byte DAT[] = {0xFF, 0xF3, 0x5A, 0x00, 0x00, 0x02, 0x00, 0x01,
    0x00};
    return DAT[i];
}


inline byte unknownCommand4CResponse(byte i) {
    const byte DAT[] = {0xFF, 0xF3, 0x5A, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00};
    switch (i) {
    case 6: return unknownFlag ? 0x07 : 0x04;
    default: return DAT[i];
    }
}


inline byte vibrationEnableResponse(byte i) {
    const byte DAT[] = {0xFF, 0xF3, 0x5A, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    return DAT[i];
}


inline byte dat(const byte CMD[], byte i) {
    switch (CMD[1]) {
    case READ_DATA:
        return readDataResponse(i);
    case CONFIG_MODE:
    isConfigMode = (CMD[3] == 0x01);
        return readDataResponse(i);
    case SET_MODE_AND_LOCK:
    isAnalogMode = (CMD[3] == 0x01);
        return setModeAndLockResponse(i);
    case QUERY_MODEL_AND_MODE:
        return queryModelAndModeResponse(i);
    case UNKNOWN_COMMAND_46:
    unknownFlag = (CMD[3] == 0x01);
        return unknownCommand46Response(i);
    case UNKNOWN_COMMAND_47:
        return unknownCommand47Response(i);
    case UNKNOWN_COMMAND_4C:
    unknownFlag = (CMD[3] == 0x01);
        return unknownCommand4CResponse(i);
    case VIBRATION_ENABLE:
        return vibrationEnableResponse(i);
    default:
        return readDataResponse(i);
    }
}


inline void acknowledge() {
    SET_ACK_LOW;
    _delay_us(ACK_WAIT);
    SET_ACK_HIGH;
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
    const byte DAT = dat(CMD, cmdCount);
    if (cmdCount == 1) {ID = DAT;}
    SPDR = DAT;
#ifdef DEBUG
    if (logCount < MAX_LOG_SIZE) {datLog[logCount] = DAT;}
#endif
    if (continueCom) {
      acknowledge();
    } else {
      clock_msec = micros() + 8000;
      read_pp = false;
    }
}


void loop() {
  if (clock_msec <= micros() && read_pp == false) {
    pp->update();
    read_pp = true;
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
    if (logCount == MAX_LOG_SIZE) {
        logCount = 0;
        int lfPos = -1;
        for (int i=0; i<MAX_LOG_SIZE; i++) {
            if (cmdLog[i] == LINE_FEED) {
                Serial.print("DAT: ");
                for (int j=lfPos+1; j<i; j++) {
                    Serial.print(datLog[j], HEX);
                    Serial.print(" ");
                }
                lfPos=i;
                Serial.println();
                Serial.print("CMD: ");
            } else {
                Serial.print(cmdLog[i], HEX);
                Serial.print(" ");
            }
        }
    }
#endif
    //delay(250);
}
