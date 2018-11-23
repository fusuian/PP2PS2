#include <SPI.h>

extern unsigned long clock_msec;
extern bool read_pp;

byte sw1();
byte sw2();
volatile byte RH = 0x80;
volatile byte RV = 0x80;
volatile byte LH = 0x80;
volatile byte LV = 0x80;

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


void ds2talker_setup()
{
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


inline byte readDataResponse(byte i) {
    const byte DAT[] = {0xFF, 0x41, 0x5A, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    switch (i) {
    case 1: return isConfigMode ? 0xF3 : (isAnalogMode ? 0x73 : 0x41);
    case 3: return sw1();
    case 4: return sw2();
    case 5:
      return RH; //pp->rudder()*4; //RH;
    case 6:
      return RV; //pp->throttle()*2; //RV;
    case 7:
      return LH; //pp->x()/4; // LH
    case 8:
      return LV; //pp->y()/4; // LV
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


void ds2talker_debug()
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
