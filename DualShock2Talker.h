
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


class DualShock2Talker
{
  public:
    DualShock2Talker(){}
    ~DualShock2Talker(){}
    void setup();
    void debug();
    virtual byte sw1();
    virtual byte sw2();
    virtual byte rh(){ return 0x80; }
    virtual byte rv(){ return 0x80; }
    virtual byte lh(){ return 0x80; }
    virtual byte lv(){ return 0x80; }

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

  private:
    volatile bool isAnalogMode;// = false;
    volatile bool isConfigMode;// = false;
    volatile bool unknownFlag;// = false;

    inline byte readDataResponse(byte i) {
        const byte DAT[] = {0xFF, 0x41, 0x5A, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
        switch (i) {
        case 1: return isConfigMode ? 0xF3 : (isAnalogMode ? 0x73 : 0x41);
        case 3: return sw1();
        case 4: return sw2();
        case 5:
          return rh(); //pp->rudder()*4; //RH;
        case 6:
          return rv(); //pp->throttle()*2; //RV;
        case 7:
          return lh(); //pp->x()/4; // LH
        case 8:
          return lv(); //pp->y()/4; // LV
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
};
