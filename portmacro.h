#ifndef __PORTMACRO_H__
#define __PORTMACRO_H__

#define PORT(n) ( (n <=  7)? PORTD : \
                ( (n <= 13)? PORTB : \
                ( (n <= 19)? PORTC : PORTB)))

#define PIN(n) ( (n <=  7)? PIND : \
               ( (n <= 13)? PINB : \
               ( (n <= 19)? PINC : PINB)))

#define REG(n)  ( (n <=  7)? (n) : \
                ( (n <= 13)? (n - 8) : \
                ( (n <= 19)? (n - 14) : (13 - 8) )))

#define portOn(p)  ( PORT(p) |=  _BV(REG(p)) )
#define portOff(p) ( PORT(p) &= ~_BV(REG(p)) )
#define portWrite(p, v) ( (v > 0)? portOn(p) : portOff(p) )

#define isPin(p)  ( (PIN(p) & _BV(REG(p))) == 0 )

#endif // __PORTMACRO_H__
