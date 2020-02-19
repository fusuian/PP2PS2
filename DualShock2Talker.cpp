// DualShock2Talker.cpp: DualShock2 Emulator on Arduino
//
// Copyright (c) 2019-2020 ASAHI,Michiharu
//
// Based on Rolling switch for PS2
// Copyright (c) 2015 Kazumasa ISE
//
// Released under the MIT license
// http://opensource.org/licenses/mit-license.php

#include <SPI.h>
#include "DualShock2Talker.h"



void DualShock2Talker::setup() {
    isAnalogMode = false;
    isConfigMode = false;
    unknownFlag = false;

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
}





void DualShock2Talker::debug() {
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
}



#ifdef DEBUG
#define LINE_FEED 0xAA
#define MAX_LOG_SIZE 60
volatile byte cmdLog[MAX_LOG_SIZE] = {0};
volatile byte datLog[MAX_LOG_SIZE] = {0};
volatile int logCount = 0;
#endif


extern DualShock2Talker * ds2talker;


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
        ds2talker->standby();
    }
}
