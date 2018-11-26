#include <SPI.h>
#include "DualShock2Talker.h"

extern unsigned long clock_msec;
extern bool read_pp;


void DualShock2Talker::setup()
{
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





void DualShock2Talker::debug()
{
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
